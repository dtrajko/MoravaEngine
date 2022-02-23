#include "Platform/Windows/WindowsWindow.h"

#include "H2M/Core/Events/KeyEventH2M.h"
#include "H2M/Core/Events/MouseEventH2M.h"
#include "H2M/Core/Events/ApplicationEventH2M.h"
#include "H2M/Platform/Vulkan/VulkanContextH2M.h"

#include "Core/Application.h"
#include "Core/Log.h"

#include "Platform/DX11/DX11.h"
#include "Platform/DX11/DX11Context.h"
#include "Platform/DX11/DX11InputSystem.h"
#include "Platform/DX11/DX11RendererBasic.h"
#include "Platform/DX11/DX11Renderer.h"

#include <cmath>
#include <exception>


Window* Window::Create(const WindowSpecification& windowSpecification)
{
	return new WindowsWindow(windowSpecification);
}

WindowsWindow::WindowsWindow(const WindowSpecification& windowSpecification)
	: m_Specification(windowSpecification)
{
	m_EventLoggingEnabled = false;

	Init();
}

WindowsWindow::~WindowsWindow()
{
	Shutdown();
}

/**** BEGIN Hazel properties and methods (GLFW) ****/

static void GLFWErrorCallback(int error, const char* description)
{
	Log::GetLogger()->error("GLFW Error ({0}) : {1}", error, description);
}

static bool s_GLFWInitialized = false;

void WindowsWindow::Init()
{
	m_Data.Title = m_Specification.Title;
	m_Data.Width = m_Specification.Width;
	m_Data.Height = m_Specification.Height;

	xChange = 0.0f;
	yChange = 0.0f;
	xChangeReset = 0.0f;
	yChangeReset = 0.0f;
	m_CursorIgnoreLimit = 5.0f;

	for (size_t i = 0; i < 1024; i++) {
		keys[i] = false;
		keys_prev[i] = false;
	}

	for (size_t i = 0; i < 32; i++) {
		buttons[i] = false;
		buttons_prev[i] = false;
	}

	mouseFirstMoved = true;
	mouseCursorAboveWindow = false;

	Log::GetLogger()->info("Creating window - title: '{0}', size: [{1}x{2}]", m_Specification.Title, m_Specification.Width, m_Specification.Height);

	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::OpenGL:
	case H2M::RendererAPITypeH2M::Vulkan:
		InitGLFW(m_Specification);
		break;
	case H2M::RendererAPITypeH2M::DX11:
		InitDX11(m_Specification);
		break;
	}

	m_RendererContext = H2M::RendererContextH2M::Create(this);
	m_RendererContext->Init();

	H2M::RefH2M<H2M::VulkanContextH2M> context = m_RendererContext.As<H2M::VulkanContextH2M>();

	// m_SwapChain.Init(H2M::VulkanContextH2M::GetInstance(), context->GetDevice());
	// m_SwapChain.InitSurface(m_GLFW_Window);

	uint32_t width = m_Data.Width, height = m_Data.Height;
	// m_SwapChain.Create(&width, &height, m_Data.VSync);

	// RendererBasic::SetRendererContext(m_RendererContext);

	SetVSync(true);

	RendererBasic::EnableDepthTest();
	RendererBasic::SetViewportSize(m_Data.Width, m_Data.Height);

	// The old MoravaEngine method of handling events
	// not working with the new Hazel GLFW callbacks

	// SetCallbacks();
}

void WindowsWindow::InitGLFW(const WindowSpecification& props)
{
	// Initialize GLFW
	if (!s_GLFWInitialized)
	{
		// TODO: glfwTerminate on system shutdown

		// Initialize GLFW
		if (!glfwInit())
		{
			glfwTerminate();
			std::string msg = "WindowWindow: GLFW initialization failed!";
			Log::GetCoreLogger()->error(msg);
			throw std::runtime_error(msg);
		}

		glfwSetErrorCallback(GLFWErrorCallback);
		s_GLFWInitialized = true;

		Log::GetLogger()->info("GLFW initialized.");
	}

	// Setup GLFW window properties
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	// OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	// Core profile = No backwards compatibility
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Allow forward compatibility
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::Vulkan)
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	}

	// Enable MSAA (multisample anti-aliasing)
	glfwWindowHint(GLFW_SAMPLES, 4);
	glEnable(GL_MULTISAMPLE);

	if (m_Specification.Fullscreen)
	{
		GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
		m_GLFW_Window = glfwCreateWindow(mode->width, mode->height, m_Data.Title.c_str(), primaryMonitor, nullptr);
		m_GLFW_Window = glfwCreateWindow((int)m_Data.Width, (int)m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
	}
	else
	{
		m_GLFW_Window = glfwCreateWindow((int)m_Data.Width, (int)m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
	}

	if (!m_GLFW_Window)
	{
		glfwTerminate();
		throw std::runtime_error("GLFW Window creation failed!");
	}

	// Set context for GLEW to use
	glfwSetWindowUserPointer(m_GLFW_Window, &m_Data);

	// GLFW event callbacks
	SetCallbacksHazelDev();
}

inline std::pair<float, float> WindowsWindow::GetWindowPos() const
{
	int x, y;
	glfwGetWindowPos(m_GLFW_Window, &x, &y);
	return { x, y };
}

void WindowsWindow::ProcessEvents()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::OpenGL:
	case H2M::RendererAPITypeH2M::Vulkan:
	{
		glfwPollEvents();

		//ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
		//glfwSetCursor(m_GLFW_Window, m_ImGuiMouseCursors[imgui_cursor] ? m_ImGuiMouseCursors[imgui_cursor] : m_ImGuiMouseCursors[ImGuiMouseCursor_Arrow]);
		glfwSetInputMode(m_GLFW_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
	break;
	case H2M::RendererAPITypeH2M::DX11:
		Broadcast();
		break;
	}
}

void WindowsWindow::SwapBuffers()
{
	m_RendererContext->SwapBuffers();

	// m_SwapChain.Present();
}

void WindowsWindow::SetVSync(bool enabled)
{
	if (H2M::RendererAPI_H2M::Current() == H2M::RendererAPITypeH2M::OpenGL)
	{
		if (enabled) {
			glfwSwapInterval(1);
		}
		else {
			glfwSwapInterval(0);
		}
	}

	m_Data.VSync = enabled;
}

bool WindowsWindow::IsVSync() const
{
	return m_Data.VSync;
}

void WindowsWindow::Maximize()
{
	glfwMaximizeWindow(m_GLFW_Window);
}

void WindowsWindow::SetTitle(std::string title)
{
	m_Data.Title = title;
	glfwSetWindowTitle(m_GLFW_Window, m_Data.Title.c_str());
}

void WindowsWindow::SetEventCallback(const EventCallbackFn& callback)
{
	m_Data.EventCallback = callback;
}

/**** END Hazel properties and methods ****/

// A DirectX 11 methods
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return true;
	}

	switch (msg)
	{
	case WM_CREATE:
	{
		// Event fired when the window is created
		// Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		// window->SetHWND(hwnd);
		// window->OnCreate();
		break;
	}
	case WM_SIZE:
	{
		// Event fired when the window is resized
		Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (window)
		{
			window->OnSize();
		}
		break;
	}
	case WM_SETFOCUS:
	{
		// Event fired when the window is in focus
		Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (window)
		{
			window->OnFocus();
		}
		break;
	}
	case WM_KILLFOCUS:
	{
		// Event fired when the window is in focus
		Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (window)
		{
			window->OnKillFocus();
		}
		break;
	}
	case WM_DESTROY:
	{
		// Event fired when the window is destroyed
		Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		window->OnDestroy();
		::PostQuitMessage(0);
		break;
	}
	default:
	{
		return ::DefWindowProc(hwnd, msg, wparam, lparam);
	}
	}

	return NULL;
}

// A DirectX 11 method
void WindowsWindow::SetHWND(HWND hwnd)
{
	m_HWND = hwnd;
}

// A DirectX 11 method
void WindowsWindow::InitDX11(const WindowSpecification& windowSpecification)
{
	m_IsInitialized = false;

#if defined UNICODE

	LPCWSTR className = L"WindowsWindow";
	LPCWSTR menuName = L"";
	std::wstring windowNameWStr = Util::to_wstr(windowSpecification.Title.c_str());
	const wchar_t* windowNameWChar = windowNameWStr.c_str();
	LPCWSTR windowName = (LPCWSTR)windowNameWChar;

	WNDCLASSEX wc;
	wc.cbClsExtra = NULL;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.cbWndExtra = NULL;
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wc.hInstance = NULL;
	wc.lpszClassName = className;
	wc.lpszMenuName = menuName;
	wc.style = NULL;
	wc.lpfnWndProc = &WndProc;

	if (!::RegisterClassEx(&wc)) // If the registration of class fails, the function returns false
	{
		throw std::exception("Window not created successfully.");
	}

	m_HWND = ::CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, className, windowName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, windowSpecification.Width, windowSpecification.Height,
		NULL, NULL, NULL, NULL);

#else

	LPCSTR className = "WindowsWindow";
	LPCSTR menuName = "";
	std::string windowNameWStr = props.Title.c_str();
	const char* windowNameWChar = windowNameWStr.c_str();
	LPCSTR windowName = (LPCSTR)windowNameWChar;

	WNDCLASSEX wc;
	wc.cbClsExtra = NULL;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.cbWndExtra = NULL;
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wc.hInstance = NULL;
	wc.lpszClassName = className;
	wc.lpszMenuName = menuName;
	wc.style = NULL;
	wc.lpfnWndProc = &WndProc;

	if (!::RegisterClassEx(&wc)) // If the registration of class fails, the function returns false
	{
		throw std::exception("Window not created successfully.");
	}

	m_HWND = ::CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, className, windowName, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, props.Width, props.Height,
		NULL, NULL, NULL, NULL);

#endif

	if (!m_HWND)
	{
		throw std::exception("Window not created successfully.");
	}

	// Show up the window
	::ShowWindow(m_HWND, SW_SHOW);
	::UpdateWindow(m_HWND);

	// Set this flag to true to indicate that the window is initialized and running
	m_IsRunning = true;
}

// Used only for DirectX 11
bool WindowsWindow::Broadcast()
{
	if (H2M::RendererAPI_H2M::Current() != H2M::RendererAPITypeH2M::DX11) return true;

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

// A DirectX 11 method
bool WindowsWindow::Release()
{
	// Destroy the window
	if (!::DestroyWindow(m_HWND))
	{
		return false;
	}
	return true;
}

// A DirectX 11 method
void WindowsWindow::OnCreate()
{
}

// A DirectX 11 method
void WindowsWindow::OnDestroy()
{
	m_IsRunning = false;
}

// A DirectX 11 method
void WindowsWindow::OnFocus()
{
	// DX11InputSystem::Get()->AddListener(this);
	m_InFocus = true;
	// DX11InputSystem::Get()->ShowCursor(false);
}

// A DirectX 11 method
void WindowsWindow::OnKillFocus()
{
	// DX11InputSystem::Get()->RemoveListener(this);
	m_InFocus = false;
	// DX11InputSystem::Get()->ShowCursor(true);
}

// A DirectX 11 method
void WindowsWindow::OnSize()
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

	// TODO: use observer / event listeners to remove this unwanted and unnecessary dependency
	DX11Renderer::OnResize(m_Data.Width, m_Data.Height);
}

// A DirectX 11 method
void WindowsWindow::OnKeyDown(int key)
{
}

// A DirectX 11 method
void WindowsWindow::OnKeyUp(int key)
{
}

// A DirectX 11 method
void WindowsWindow::OnMouseMove(const glm::vec2& mousePosDelta, const glm::vec2& mousePosAbs)
{
}

// A DirectX 11 method
void WindowsWindow::OnLeftMouseDown(const glm::vec2& mousePos)
{
}

// A DirectX 11 method
void WindowsWindow::OnRightMouseDown(const glm::vec2& mousePos)
{
}

// A DirectX 11 method
void WindowsWindow::OnLeftMouseUp(const glm::vec2& mousePos)
{
}

// A DirectX 11 method
void WindowsWindow::OnRightMouseUp(const glm::vec2& mousePos)
{
}

// A DirectX 11 method
bool WindowsWindow::IsRunning()
{
	return m_IsRunning;
}

// A DirectX 11 method
RECT WindowsWindow::GetClientWindowRect()
{
	RECT rect;
	::GetClientRect(m_HWND, &rect);
	::ClientToScreen(m_HWND, (LPPOINT)&rect.left);
	::ClientToScreen(m_HWND, (LPPOINT)&rect.right);
	return rect;
}

// A DirectX 11 method
RECT WindowsWindow::GetSizeScreen()
{
	RECT rect;
	rect.right = ::GetSystemMetrics(SM_CXSCREEN);
	rect.bottom = ::GetSystemMetrics(SM_CYSCREEN);
	return rect;
}

void WindowsWindow::Shutdown()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::OpenGL:
	case H2M::RendererAPITypeH2M::Vulkan:
	{
		glfwDestroyWindow(m_GLFW_Window);
		glfwTerminate();
		s_GLFWInitialized = false;
	}
	break;
	case H2M::RendererAPITypeH2M::DX11:
		Release();
		break;
	}
}

int WindowsWindow::m_ActionPrev;

void WindowsWindow::OnUpdate()
{
	// Get and handle user input events
	// glfwPollEvents(); // TODO: move to WindowsWindow::ProcessEvents()
	ProcessEvents();

	// Swap buffers
	// glfwSwapBuffers(m_GLFW_Window); // TODO: move to WindowsWindow::SwapBuffers() / OpenGLContext::SwapBuffers()

	SwapBuffers();

	// Broadcast();
}

GLfloat WindowsWindow::getXChange()
{
	float theChange = 0.0f;
	if (std::abs(xChange) > m_CursorIgnoreLimit) {
		theChange = xChange;
	}
	return theChange;
}

GLfloat WindowsWindow::getYChange()
{
	float theChange = 0.0f;
	if (std::abs(yChange) > m_CursorIgnoreLimit) {
		theChange = yChange;
	}
	return theChange;
}

float WindowsWindow::getXMouseScrollOffset()
{
	float theOffset = xMouseScrollOffset;
	xMouseScrollOffset = 0.0f;
	return theOffset;
}

float WindowsWindow::getYMouseScrollOffset()
{
	float theOffset = yMouseScrollOffset;
	yMouseScrollOffset = 0.0f;
	return theOffset;
}

void WindowsWindow::SetCursorDisabled()
{
	glfwSetInputMode(m_GLFW_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void WindowsWindow::SetCursorNormal()
{
	glfwSetInputMode(m_GLFW_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

bool WindowsWindow::GetShouldClose()
{
	switch (H2M::RendererAPI_H2M::Current())
	{
	case H2M::RendererAPITypeH2M::OpenGL:
	case H2M::RendererAPITypeH2M::Vulkan:
		return glfwWindowShouldClose(m_GLFW_Window);
	case H2M::RendererAPITypeH2M::DX11:
		return !m_IsRunning;
	default:
		return true;
	}
}

bool WindowsWindow::IsMouseButtonClicked(int mouseButton)
{
	bool result = buttons[mouseButton] && !buttons_prev[mouseButton];
	buttons_prev[mouseButton] = buttons[mouseButton];
	return result;
}

bool WindowsWindow::IsMouseButtonReleased(int mouseButton)
{
	bool result = !buttons[mouseButton] && buttons_prev[mouseButton];
	buttons_prev[mouseButton] = buttons[mouseButton];
	return result;
}

void WindowsWindow::SetShouldClose(bool shouldClose)
{
	glfwSetWindowShouldClose(m_GLFW_Window, shouldClose);
}

H2M::VulkanSwapChainH2M& WindowsWindow::GetSwapChain()
{
	return m_SwapChain;
}

void WindowsWindow::SetInputMode(bool cursorEnabled)
{
	if (cursorEnabled)
		glfwSetInputMode(m_GLFW_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	else
		glfwSetInputMode(m_GLFW_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

/****
 * Obsolete after adding GLFW callbacks to Init() method
 *
void WindowsWindow::SetCallbacks()
{
	glfwSetKeyCallback(m_GLFW_Window, KeyCallback);
	glfwSetCharCallback(m_GLFW_Window, CharCallback);
	glfwSetCursorPosCallback(m_GLFW_Window, CursorPosCallback);
	glfwSetMouseButtonCallback(m_GLFW_Window, MouseButtonCallback);
	glfwSetCursorEnterCallback(m_GLFW_Window, CursorEnterCallback);
	glfwSetWindowSizeCallback(m_GLFW_Window, WindowSizeCallback);
	glfwSetWindowCloseCallback(m_GLFW_Window, WindowCloseCallback);
	glfwSetScrollCallback(m_GLFW_Window, ScrollCallback);
}
****/

void WindowsWindow::SetCallbacksHazelDev()
{
	// Set GLFW callbacks (Handle Key and Mouse input)
	glfwSetWindowSizeCallback(m_GLFW_Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;
			H2M::WindowResizeEventH2M event(width, height);
			data.EventCallback(event);

			// Support for the old way of handling events
			WindowSizeCallback(window, width, height);
		});

	glfwSetWindowCloseCallback(m_GLFW_Window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			H2M::WindowCloseEventH2M event;
			data.EventCallback(event);

			// Support for the old way of handling events
			WindowCloseCallback(window);
		});

	glfwSetKeyCallback(m_GLFW_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				H2M::KeyPressedEventH2M event(key, 0);
				data.EventCallback(event);
				break;
			}
			case GLFW_RELEASE:
			{
				H2M::KeyReleasedEventH2M event(key);
				data.EventCallback(event);
				break;
			}
			case GLFW_REPEAT:
			{
				H2M::KeyPressedEventH2M event(key, 1);
				data.EventCallback(event);
				break;
			}
			}

			// Support for the old way of handling events
			KeyCallback(window, key, scancode, action, mods);
		});

	glfwSetCharCallback(m_GLFW_Window, [](GLFWwindow* window, unsigned int codepoint)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			H2M::KeyTypedEventH2M event(codepoint);
			data.EventCallback(event);

			// Support for the old way of handling events
			CharCallback(window, codepoint);
		});

	glfwSetMouseButtonCallback(m_GLFW_Window, [](GLFWwindow* window, int button, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				H2M::MouseButtonPressedEventH2M event(button);
				data.EventCallback(event);
				break;
			}
			case GLFW_RELEASE:
			{
				H2M::MouseButtonReleasedEventH2M event(button);
				data.EventCallback(event);
				break;
			}
			}

			// Support for the old way of handling events
			MouseButtonCallback(window, button, action, mods);
		});

	glfwSetScrollCallback(m_GLFW_Window, [](GLFWwindow* window, double xoffset, double yoffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			H2M::MouseScrolledEventH2M event((float)xoffset, (float)yoffset);
			data.EventCallback(event);

			// Support for the old way of handling events
			ScrollCallback(window, xoffset, yoffset);
		});

	glfwSetCursorPosCallback(m_GLFW_Window, [](GLFWwindow* window, double xpos, double ypos)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			H2M::MouseMovedEventH2M event((float)xpos, (float)ypos);
			data.EventCallback(event);

			// Support for the old way of handling events
			CursorPosCallback(window, xpos, ypos);
		});

	glfwSetCursorEnterCallback(m_GLFW_Window, [](GLFWwindow* window, int entered)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			// TODO

			// Support for the old way of handling events
			CursorEnterCallback(window, entered);
		});
}

void WindowsWindow::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(Application::Get()->GetWindow());

	if (theWindow->m_EventLoggingEnabled) {
		Log::GetLogger()->debug("WindowsWindow::KeyCallback(key {0}, scancode {1}, action {2}, mods {3})", key, scancode, action, mods);
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		// glfwSetWindowShouldClose(window, GL_TRUE);
		theWindow->SetCursorNormal();
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			theWindow->keys_prev[key] = theWindow->keys[key];
			theWindow->keys[key] = true;
			// printf("Key pressed: %d\n", key);
		}
		else if (action == GLFW_RELEASE)
		{
			theWindow->keys_prev[key] = theWindow->keys[key];
			theWindow->keys[key] = false;
		}
	}
}

void WindowsWindow::CharCallback(GLFWwindow* window, unsigned int codepoint)
{
	// not implemented/used in old MoravaEngine method of handling events
}

void WindowsWindow::CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(Application::Get()->GetWindow());

	if (theWindow->m_EventLoggingEnabled) {
		Log::GetLogger()->debug("WindowsWindow::CursorPosCallback(xpos {0}, ypos {1})", xpos, ypos);
	}

	theWindow->m_MouseX = (float)xpos;
	theWindow->m_MouseY = (float)ypos;

	if (theWindow->mouseFirstMoved)
	{
		theWindow->lastX = (GLfloat)xpos;
		theWindow->lastY = (GLfloat)ypos;
		theWindow->mouseFirstMoved = false;
	}

	theWindow->xChange = (GLfloat)xpos - theWindow->lastX;
	theWindow->yChange = theWindow->lastY - (GLfloat)ypos;

	theWindow->xChangeReset = theWindow->xChange;
	theWindow->yChangeReset = theWindow->yChange;

	// printf("theWindow->xChange [%.2f] theWindow->yChange [%.2f]\n", theWindow->xChange, theWindow->yChange);

	theWindow->lastX = (GLfloat)xpos;
	theWindow->lastY = (GLfloat)ypos;

	// printf("x:%.2f, y:%.2f\n", theWindow->xChange, theWindow->yChange);
}

void WindowsWindow::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(Application::Get()->GetWindow());

	if (theWindow->m_EventLoggingEnabled) {
		Log::GetLogger()->debug("WindowsWindow::MouseButtonCallback(button {0}, action {1}, mods {2})", button, action, mods);
	}

	if (button >= 0 && button < 32)
	{
		if (action == GLFW_PRESS)
		{
			if (action != m_ActionPrev) {
				theWindow->buttons_prev[button] = theWindow->buttons[button];
				m_ActionPrev = action;
			}

			theWindow->buttons[button] = true;
			// printf("Mouse button pressed: %d\n", button);
		}
		else if (action == GLFW_RELEASE)
		{
			if (action != m_ActionPrev) {
				theWindow->buttons_prev[button] = theWindow->buttons[button];
				m_ActionPrev = action;
			}

			theWindow->buttons[button] = false;
			// printf("Mouse button released: %d\n", button);
		}
	}

	if (theWindow->buttons[GLFW_MOUSE_BUTTON_MIDDLE] && theWindow->mouseCursorAboveWindow)
	{
		theWindow->SetCursorDisabled();
	}
}

void WindowsWindow::CursorEnterCallback(GLFWwindow* window, int entered)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(Application::Get()->GetWindow());

	if (theWindow->m_EventLoggingEnabled) {
		Log::GetLogger()->debug("WindowsWindow::CursorEnterCallback(entered {0})", entered);
	}

	if (entered)
		theWindow->mouseCursorAboveWindow = true;
	else
		theWindow->mouseCursorAboveWindow = false;
}

void WindowsWindow::WindowSizeCallback(GLFWwindow* window, int width, int height)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(Application::Get()->GetWindow());

	if (theWindow->m_EventLoggingEnabled) {
		Log::GetLogger()->debug("WindowsWindow::WindowSizeCallback(width {0}, height {1})", width, height);
	}

	theWindow->m_Data.Width = width;
	theWindow->m_Data.Height = height;
	glViewport(0, 0, width, height);
}

void WindowsWindow::WindowCloseCallback(GLFWwindow* window)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(Application::Get()->GetWindow());

	// not implemented/used in old MoravaEngine method of handling events
}

void WindowsWindow::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	WindowsWindow* theWindow = static_cast<WindowsWindow*>(Application::Get()->GetWindow());

	theWindow->xMouseScrollOffset = (float)xoffset;
	theWindow->yMouseScrollOffset = (float)yoffset;
}
