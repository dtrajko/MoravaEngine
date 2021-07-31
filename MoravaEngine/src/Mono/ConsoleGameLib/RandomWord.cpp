#include "RandomWord.h"
#include "RWord.h"

#include <cstdlib>


const char* CGL::getRandomWord()
{
	// Word is null at begining
	const char* word = nullptr;
	
	// Get random value
	CGL_internal::RWord* ptrWord = CGL_internal::RWord::get();
	if (ptrWord)
	{
		// Compute random value
		unsigned int randomValue = rand() % ptrWord->getWordCount();
		// Load word
		word = ptrWord->getWord(randomValue);
	}

	// Return word
	return word;
}
