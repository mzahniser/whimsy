/* Edge.cpp
Copyright 2020 Michael Zahniser
*/

#include "Edge.h"

using namespace std;



Edge::Edge(const Ring &ring)
	: start(ring.back()), v(ring.front() - start), it(&ring.front()), end(it + ring.size())
{
}



// Increment the iterators.
void Edge::operator++()
{
	order += v.Dot(v);
	start = *it;
	++it;
	if(it != end)
		v = *it - start;
}



void Edge::operator++(int)
{
	++*this;
}



// Check if we've reached the end of this ring.
Edge::operator bool() const
{
	return (it != end);
}



bool Edge::operator!() const
{
	return (it == end);
}



// Get the current start and end points.
Point Edge::Start() const
{
	return start;
}



Point Edge::End() const
{
	return *it;
}



// Get the edge vector, i.e. End() - Start().
Point Edge::Vector() const
{
	return v;
}



// Get the current "order" value, i.e. the sum of squared edge lengths
// of all edges prior to this one.
int Edge::Order() const
{
	return order;
}
