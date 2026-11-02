#include "Platform/DX11/DX11Window.h"

#include "Core/Application.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "Core/Util.h"

#include "Platform/DX11/DX11.h"
#include "Platform/DX11/DX11Context.h"
#include "Platform/DX11/DX11InputSystem.h"
#include "Platform/DX11/DX11RendererBasic.h"

#include <exception>
#include <filesystem>

#define UNICODE
#define _UNICODE

DX11Window::DX11Window(const WindowSpecification& windowSpecification)
	: m_Specification(windowSpecification)
{
	Init();
}

DX11Window::~DX11Window()
{
	Release();
}

static DX11Window::MessageHandler s_MessageHandler = nullptr;

void DX11Window::SetMessageHandler(MessageHandler handler)
{
	s_MessageHandler = handler;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (s_MessageHandler && s_MessageHandler(hwnd, msg, wparam, lparam))
	{
		return true;
	}

	switch (msg)
	{
	case WM_CREATE:
	{
		// Event fired when the window is created
		// DX11Window* window = (DX11Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		// window->SetHWND(hwnd);
		// window->OnCreate();
		break;
	}
	case WM_SIZE:
	{
		// Event fired when the window is resized
		DX11Window* window = (DX11Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (window)
		{
			window->OnSize();
		}
		break;
	}
	case WM_SETFOCUS:
	{
		// Event fired when the window is in focus
		DX11Window* window = (DX11Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (window)
		{
			window->OnFocus();
		}
		break;
	}
	case WM_KILLFOCUS:
	{
		// Event fired when the window is in focus
		DX11Window* window = (DX11Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (window)
		{
			window->OnKillFocus();
		}
		break;
	}
	case WM_INPUT:
	{
		// Raw mouse motion (registered in Init), used for camera rotation while the cursor is captured
		RAWINPUT raw = {};
		UINT size = sizeof(raw);
		if (::GetRawInputData((HRAWINPUT)lparam, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) != (UINT)-1 &&
			raw.header.dwType == RIM_TYPEMOUSE && !(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
		{
			DX11InputSystem::Get()->AddRawMouseDelta(raw.data.mouse.lLastX, raw.data.mouse.lLastY);
		}
		return ::DefWindowProcW(hwnd, msg, wparam, lparam); // required for WM_INPUT cleanup
	}
	case WM_DESTROY:
	{
		// Event fired when the window is destroyed
		DX11Window* window = (DX11Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		window->OnDestroy();
		::PostQuitMessage(0);
		break;
	}
	default:
	{
		// The window class is registered with RegisterClassExW, so use the wide default procedure explicitly:
		// the project is built without UNICODE, where DefWindowProc means DefWindowProcA, which reads the
		// UTF-16 window title as ANSI and stops after the first character ("M")
		return ::DefWindowProcW(hwnd, msg, wparam, lparam);
	}
	}

	return NULL;
}

void DX11Window::Init()
{
	m_Data.Title = m_Specification.Title;
	m_Data.Width = m_Specification.Width;
	m_Data.Height = m_Specification.Height;

	Log::GetLogger()->info("[Window] Application working directory: {}", std::filesystem::current_path().string());
	Log::GetLogger()->info("[Window] Creating window - title: '{0}', size: [{1}x{2}]", m_Specification.Title, m_Specification.Width, m_Specification.Height);

	m_IsInitialized = false;

	const wchar_t* className = L"WindowsWindow";
	const wchar_t* menuName = L"";

	std::wstring windowNameWStr = Util::to_wstr(m_Specification.Title);
	LPCWSTR windowName = windowNameWStr.c_str();

	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.lpfnWndProc = &WndProc;
	wc.hInstance = GetModuleHandle(nullptr);
	wc.lpszClassName = className;
	wc.lpszMenuName = menuName;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

	if (!RegisterClassExW(&wc))
		throw std::exception("Window not created successfully.");

	m_HWND = CreateWindowExW(
		WS_EX_OVERLAPPEDWINDOW,
		className,
		windowName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		m_Specification.Width,
		m_Specification.Height,
		nullptr, nullptr,
		GetModuleHandle(nullptr),
		nullptr
	);

	if (!m_HWND)
		throw std::exception("Window not created successfully.");

	ShowWindow(m_HWND, SW_SHOW);
	UpdateWindow(m_HWND);

	// Relative mouse motion for camera rotation (DX11CameraFP), see WM_INPUT in WndProc
	DX11InputSystem::Get()->RegisterRawMouseInput(m_HWND);

	m_IsRunning = true;

	m_RendererContext = H2M::RendererContextH2M::Create(this);
	m_RendererContext->Init();

	RendererBasic::SetRendererContext(m_RendererContext);

	SetVSync(true);

	RendererBasic::EnableDepthTest();
	RendererBasic::SetViewportSize(m_Data.Width, m_Data.Height);
}

std::pair<int, int> DX11Window::GetWindowPos() const
{
	RECT rect = {};
	::GetWindowRect(m_HWND, &rect);
	return { (int)rect.left, (int)rect.top };
}

void DX11Window::ProcessEvents()
{
	Broadcast();
}

void DX11Window::SwapBuffers()
{
	m_RendererContext->SwapBuffers();
}

void DX11Window::OnUpdate()
{
	ProcessEvents();
	SwapBuffers();
}

bool DX11Window::Broadcast()
{
	MSG msg;

	if (!m_IsInitialized)
	{
		SetWindowLongPtr(m_HWND, GWLP_USERDATA, (LONG_PTR)this);
		OnCreate();
		m_IsInitialized = true;
	}

	while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Sleep(1);

	return true;
}

bool DX11Window::Release()
{
	// Destroy the window
	if (!::DestroyWindow(m_HWND))
	{
		return false;
	}
	return true;
}

void DX11Window::OnCreate()
{
}

void DX11Window::OnDestroy()
{
	m_IsRunning = false;
}

void DX11Window::OnFocus()
{
	// DX11InputSystem::Get()->AddListener(this);
	m_InFocus = true;
	// DX11InputSystem::Get()->ShowCursor(false);
}

void DX11Window::OnKillFocus()
{
	// DX11InputSystem::Get()->RemoveListener(this);
	m_InFocus = false;
	// DX11InputSystem::Get()->ShowCursor(true);
}

void DX11Window::OnSize()
{
	// Cooldown
	float currentTimestamp = Timer::Get()->GetCurrentTimestamp();
	if (currentTimestamp - m_ResizeViewport.lastTime < m_ResizeViewport.cooldown) return;
	m_ResizeViewport.lastTime = currentTimestamp;

	RECT windowRECT = GetClientWindowRect();

	m_Data.Width = windowRECT.right - windowRECT.left;
	m_Data.Height = windowRECT.bottom - windowRECT.top;

	// DX11Context::Get()->GetSwapChain()->OnResize(windowRECT.right, windowRECT.bottom);
	DX11Context::Get()->GetSwapChain()->OnResize(m_Data.Width, m_Data.Height);

	// The renderer's images that follow the window's size (the editor's DirectX 11 scene: DX11Renderer)
	Application::Get()->GetClient()->OnWindowResize(m_Data.Width, m_Data.Height);
}

RECT DX11Window::GetClientWindowRect()
{
	RECT rect;
	::GetClientRect(m_HWND, &rect);
	::ClientToScreen(m_HWND, (LPPOINT)&rect.left);
	::ClientToScreen(m_HWND, (LPPOINT)&rect.right);
	return rect;
}

RECT DX11Window::GetSizeScreen()
{
	RECT rect;
	rect.right = ::GetSystemMetrics(SM_CXSCREEN);
	rect.bottom = ::GetSystemMetrics(SM_CYSCREEN);
	return rect;
}
