#pragma once

#include "WinInclude.h"

#include <memory>


namespace CGL_internal
{
	class RWord
	{
		// Class
		public:
			// Destruct
			~RWord();

			// Get word at
			const char* getWord(unsigned int index);
			// Get word count
			unsigned int getWordCount();

		private:
			// Construct by word fiel
			RWord(const char* file);

			// Memory allocators
			char* m_textAllocator = nullptr;
			char** m_arrayAllocator = nullptr;

			// Number of words
			unsigned int m_wordCount = 0;
			
			// If class is valid after construct
			bool m_valid = false;

		// Singelton
		public:
			// Inti and destroy
			static bool init(const char* file);
			static void destroy();

			// Singelton get
			static RWord* get();

		private:
			// Static instance
			static RWord* s_ptrInstance;
	};
}
