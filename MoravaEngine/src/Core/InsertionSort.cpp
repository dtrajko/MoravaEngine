#include "Core/InsertionSort.h"


void InsertionSort::SortHighToLow(std::vector<Particle*>* elements)
{
	for (int i = 1; i < elements->size(); i++) {
		Particle* item = elements->at(i);
		if (item->GetDistance() > elements->at(i - 1)->GetDistance()) {
			SortUpHighToLow(elements, i);
		}
	}
}

void InsertionSort::SortUpHighToLow(std::vector<Particle*>* elements, int i)
{
	Particle* item = elements->at(i);
	int attemptPos = i - 1;
	while (attemptPos != 0 && elements->at(attemptPos - 1)->GetDistance() < item->GetDistance()) {
		attemptPos--;
	}
	elements->at(i) = item;
}
