#include "CsGame.h"

#include "Core/Log.h"
#include "Mono/GameInvoker/CsBind.h"


CsGame::CsGame()
{
    /**** BEGIN Mono ****/

    // Current directory as mono dir
    mono_set_dirs("C:/Program Files/Mono/lib", "C:/Program Files/Mono/etc");

    // Create Mono domain
    m_ptrMonoDomain = mono_jit_init("MoravaEngine");
    if (m_ptrMonoDomain)
    {
        Log::GetLogger()->info("Mono Domain successfully initialized!");

        // Load a Mono assembly HangmanScript.dll
        std::string assemblyFilepath = "Projects/HangmanScript/bin/Debug/netstandard2.0/HangmanScript.dll";
        m_ptrGameAssembly = mono_domain_assembly_open(m_ptrMonoDomain, assemblyFilepath.c_str());
        if (m_ptrGameAssembly)
        {
            Log::GetLogger()->info("Mono Domain Assembly '{0}' successfully opened!", assemblyFilepath);

            // Loading Mono image
            m_ptrGameAssemblyImage = mono_assembly_get_image(m_ptrGameAssembly);
            if (m_ptrGameAssemblyImage)
            {
                Log::GetLogger()->info("Mono Assembly Image successfullu loaded!");

                // Add internal calls
                mono_add_internal_call("CGL.RandomWord::getRandomWord()", &CsBind::CS_RandomWord_getRandomWord);
                mono_add_internal_call("CGL.GameConsole::clearBuffer()", nullptr/*, &CGL::Console::clearBuffer */);
                mono_add_internal_call("CGL.GameConsole::present()", nullptr/*, &CGL::Console::present */);
                mono_add_internal_call("CGL.GameConsole::putChar(uint, uint, char)", nullptr/*, &CGL::Console::putChar */);
                mono_add_internal_call("CGL.GameConsole::putString(uint, uint, string, bool)", nullptr/*, &CsBind::CS_GameConsole_putString */);

                // Find IGame
                MonoClass* ptrIGameClass = mono_class_from_name(m_ptrGameAssemblyImage, "HangmanScript", "IGame");
                MonoClass* ptrMainClass = mono_class_from_name(m_ptrGameAssemblyImage, "HangmanScript", "HangMain");
                if (ptrIGameClass && ptrMainClass)
                {
                    // Describe method
                    MonoMethodDesc* ptrMainMethodDesc = mono_method_desc_new(".HangMain:main()", false);
                    if (ptrMainMethodDesc)
                    {
                        // Find main in mainclass
                        MonoMethod* ptrMainMethod = mono_method_desc_search_in_class(ptrMainMethodDesc, ptrMainClass);
                        if (ptrMainMethod)
                        {
                            // Call main method
                            MonoObject* ptrExObject = nullptr;
                            m_ptrGameObject = mono_runtime_invoke(ptrMainMethod, nullptr, nullptr, &ptrExObject);
                            if (m_ptrGameObject)
                            {
                                // Ref count object on c++ side
                                m_gameObjectGCHandle = mono_gchandle_new(m_ptrGameObject, false);

                                // Get tick function
                                MonoMethodDesc* ptrTickMethodDesc = mono_method_desc_new(".IGame:tick()", false);
                                if (ptrTickMethodDesc)
                                {
                                    // Get real function
                                    MonoMethod* virtualMethod = mono_method_desc_search_in_class(ptrTickMethodDesc, ptrIGameClass);
                                    m_ptrTickMethod = mono_object_get_virtual_method(m_ptrGameObject, virtualMethod);

                                    // Free
                                    mono_method_desc_free(ptrTickMethodDesc);
                                }

                                // Get key event function
                                MonoMethodDesc* ptrKeyMethodDesc = mono_method_desc_new(".IGame:keyEvent(char)", false);
                                if (ptrKeyMethodDesc)
                                {
                                    // Get real function
                                    MonoMethod* virtualMethod = mono_method_desc_search_in_class(ptrKeyMethodDesc, ptrIGameClass);
                                    m_ptrKeyEventMethod = mono_object_get_virtual_method(m_ptrGameObject, virtualMethod);

                                    // Free
                                    mono_method_desc_free(ptrKeyMethodDesc);
                                }
                            }

                            // Report exception
                            if (ptrExObject)
                            {
                                MonoString* exString = mono_object_to_string(ptrExObject, nullptr);
                                const char* exCString = mono_string_to_utf8(exString);
                                MessageBoxA(NULL, exCString, "Mono Invoke issue", MB_OK | MB_ICONERROR);
                            }
                        }

                        // Free desc
                        mono_method_desc_free(ptrMainMethodDesc);
                    }
                }
            }
        }
    }

    /**** END Mono ****/
}

CsGame::~CsGame()
{
    // Release Mono handles
    if (m_gameObjectGCHandle)
    {
        mono_gchandle_free(m_gameObjectGCHandle);
    }

    // Release the domain
    if (m_ptrMonoDomain)
    {
        mono_jit_cleanup(m_ptrMonoDomain);

        Log::GetLogger()->info("Mono Domain cleanup is complete!");
    }
}

void CsGame::keyEvent(char key)
{
    // Only if populated
    if (m_ptrKeyEventMethod)
    {
        // Invoke with exception
        MonoObject* ptrExObject = nullptr;
        // TODO ...
    }
}

bool CsGame::tick()
{

}
