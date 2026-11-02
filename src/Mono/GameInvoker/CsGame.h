#pragma once

// Include MONO
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>


class CsGame
{
public:
	CsGame();
	~CsGame();

	void keyEvent(char key);
	bool tick();

private:
	// Mono generic stuff - https://www.youtube.com/watch?v=ps9EW_nzs34
	MonoDomain* m_ptrMonoDomain = nullptr;
	MonoAssembly* m_ptrGameAssembly = nullptr;
	MonoImage* m_ptrGameAssemblyImage = nullptr;
	MonoObject* m_ptrGameObject = nullptr;
	uint32_t m_gameObjectGCHandle = 0;
	MonoMethod* m_ptrTickMethod = nullptr;
	MonoMethod* m_ptrKeyEventMethod = nullptr;

};
