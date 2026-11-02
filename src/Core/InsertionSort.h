#pragma once

#include "Particle/Particle.h"

#include <vector>

/**
 * A simple implementation of an insertion sort. I implemented this very quickly
 * the other day so it may not be perfect or the most efficient! Feel free to
 * implement your own sorter instead.
 *
 * @author Karl
 *
 */
class InsertionSort
{
public:
	/**
	 * Sorts a list of particles so that the particles with the highest distance
	 * from the camera are first, and the particles with the shortest distance
	 * are last.
	 *
	 * @param list
	 *            - the list of particles needing sorting.
	 */
	static void SortHighToLow(std::vector<Particle*>* elements);
	static void SortUpHighToLow(std::vector<Particle*>* elements, int i);

};
