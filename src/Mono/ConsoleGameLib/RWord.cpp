#include "RWord.h"

// Instance
CGL_internal::RWord* CGL_internal::RWord::s_ptrInstance = nullptr;

CGL_internal::RWord::~RWord() {
	// Free allocators
	if (m_textAllocator) free(m_textAllocator);
	if (m_arrayAllocator) free(m_arrayAllocator);
}

const char* CGL_internal::RWord::getWord(unsigned int index) {
	// Word initaliy null
	const char* word = nullptr;

	// Check bounds
	if (index < m_wordCount) {
		word = m_arrayAllocator[index];
	}

	// Return word
	return word;
}

unsigned int CGL_internal::RWord::getWordCount() {
	return m_wordCount;
}


CGL_internal::RWord::RWord(const char* file) {
	// Open file
	HANDLE hFile = CreateFileA(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	bool readOk = false;
	if (hFile != INVALID_HANDLE_VALUE) {
		// Get file size
		DWORD fileSize = GetFileSize(hFile, NULL);

		// Allocate strig allocator
		m_textAllocator = (char*)malloc(fileSize + 1);
		if (m_textAllocator) {
			// Force terminator
			m_textAllocator[fileSize] = '\0';

			// Read file
			if (ReadFile(hFile, m_textAllocator, fileSize, NULL, NULL)) {
				readOk = true;
			}
		}

		// Close file
		CloseHandle(hFile);
	}

	// Allocate buffer
	unsigned int arraySize = 0;
	if (readOk) {
		// Allocate buffer
		m_arrayAllocator = (char**)malloc(4096 * sizeof(char*));
		if (m_arrayAllocator) {
			arraySize = 4096;
		}
	}

	// Process file
	if (arraySize) {
		// Current string begin
		char* stringBegin = m_textAllocator;
		bool memoryOk = true;

		// For each char
		for (char* cPtr = m_textAllocator; *cPtr && memoryOk; cPtr++) {
			// Check if is controle char
			if (*cPtr == '\r' || *cPtr == '\n') {
				// Only on new line
				if (*cPtr == '\n') {
					// Allocate more array memory
					if (m_wordCount >= arraySize) {
						// Allocate new meory
						void* newMemory = malloc(arraySize * 2 * sizeof(char*));
						if (newMemory) {
							// Copy and and store
							memcpy(newMemory, m_arrayAllocator, arraySize * sizeof(char*));
							free(m_arrayAllocator);
							m_arrayAllocator = (char**)newMemory;

							// Store new size
							arraySize *= 2;
						}
						else {
							memoryOk = false;
						}
					}

					// Only if memory is ok
					if (memoryOk) {
						// Set word
						m_arrayAllocator[m_wordCount++] = stringBegin;
						// Move stringbegin
						stringBegin = &cPtr[1];
					}
				}
				
				// Make it a terminator
				*cPtr = '\0';
			}
			else {
				// Convert to uppercase
				*cPtr = toupper(*cPtr);
			}
		}

		// Check integrety
		if (memoryOk) {
			m_valid = true;
		}
	}
}

bool CGL_internal::RWord::init(const char* file) {
	// Result bool
	bool result = false;
	
	// Only if not existing
	if (!s_ptrInstance) {
		// Create instance
		s_ptrInstance = new RWord(file);
		if (s_ptrInstance) {
			// Check
			if (s_ptrInstance->m_valid) {
				// Init succeeded
				result = true;
			}
			else {
				// Delete
				delete s_ptrInstance;
				s_ptrInstance = nullptr;
			}
		}
	}

	// Return result
	return result;
}

void CGL_internal::RWord::destroy() {
	// If in existance delete and unset
	if (s_ptrInstance) {
		delete s_ptrInstance;
		s_ptrInstance = nullptr;
	}
}

CGL_internal::RWord* CGL_internal::RWord::get() {
	return s_ptrInstance;
}
