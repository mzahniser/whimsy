/* Edge.h
Copyright 2020 Michael Zahniser
*/

#ifndef EDGE_H_
#define EDGE_H_

#include "Point.h"
#include "Ring.h"

using namespace std;



// This class represents an iterator over the edges of a part of a polygon.
class Edge {
public:
	Edge() = default;
	// Make sure the ring is not empty before calling this constructor.
	Edge(const Ring &ring);
	
	// Increment the iterators.
	void operator++();
	void operator++(int);
	// Check if we've reached the end of this ring.
	operator bool() const;
	bool operator!() const;
	
	// Get the current start and end points.
	Point Start() const;
	Point End() const;
	// Get the edge vector, i.e. End() - Start().
	Point Vector() const;
	// Get the current "order" value, i.e. the sum of sqared edge lengths
	// of all edges prior to this one.
	int Order() const;
	
	
private:
	Point start;
	Point v;
	const Point *it = nullptr;
	const Point *end = nullptr;
	int order = 0.f;
};



#endif
