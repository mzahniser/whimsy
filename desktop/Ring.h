/* Ring.h
Copyright 2020 Michael Zahniser
*/

#ifndef RING_H_
#define RING_H_

#include "Point.h"

#include <utility>
#include <vector>

using namespace std;



// A Ring is a simple polygon, i.e. a set of vertices connected in a loop with
// no edges that intersect each other. The last vertex is assumed to connect to
// the first one; there is no need for a copy of the first vertex to "close" it.
class Ring : public vector<Point> {
public:
	// Use all the standard constructors, etc.
	using vector::vector;
	
	// Translate a ring by the given vector.
	Ring operator+(Point offset) const;
	Ring &operator+=(Point offset);
	Ring operator-(Point offset) const;
	Ring &operator-=(Point offset);
	// Scale a ring by the given amount.
	Ring operator*(int scale) const;
	Ring &operator*=(int scale);
	friend Ring operator*(int scale, const Ring &ring);
	Ring operator/(int scale) const;
	Ring &operator/=(int scale);
	
	// Check if this Ring (including its boundary) contains the given point.
	bool Contains(Point point) const;
	
	// Get the area of this Ring. If the area is negative, this is a hole, i.e.
	// a ring that subtracts from a polygon instead of adding to it.
	float Area() const;
	// Check if this Ring is a hole, i.e. a counter-clockwise polygon.
	bool IsHole() const;
	
	// Reverse this ring, in-place.
	void Reverse();
	
	// Get all the points on this ring that are concave, i.e. that might be a
	// pathfinding waypoint.
	vector<Point> ConcavePoints() const;
	
	
private:
	friend class Polygon;
	
	// Assuming that this Ring does not intersect the given ring, as defined in
	// Polygon::Add(), check if the given ring is entirely contained in this.
	// This function is private so that only Polygon can use it.
	bool Contains(const Ring &ring) const;
	// Winding-number algorithm for internal use: calculates a winding number to
	// check if a point is in a polygon, and also checks if the point is on the
	// boundary of the polygon.
	pair<int, int> Winding(Point point) const;
};



#endif
