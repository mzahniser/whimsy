/* Polygon.h
Copyright 2020 Michael Zahniser
*/

#ifndef POLYGON_H_
#define POLYGON_H_

#include "Point.h"
#include "Ring.h"

#include <vector>

using namespace std;



class Polygon : public vector<Ring> {
public:
	// Translate a polygon by the given vector.
	Polygon operator+(Point offset) const;
	Polygon &operator+=(Point offset);
	Polygon operator-(Point offset) const;
	Polygon &operator-=(Point offset);
	// Scale a polygon by the given amount.
	Polygon operator*(int scale) const;
	Polygon &operator*=(int scale);
	friend Polygon operator*(int scale, const Polygon &poly);
	Polygon operator/(int scale) const;
	Polygon &operator/=(int scale);
	
	// A "ring" is a simple polygon. If the points are in clockwise order it is
	// a filled polygon; otherwise it is a hole.
	void Add(const Ring &ring);
	
	// Get just the component of this polygon that contains the given point.
	// That includes any holes in that polygon.
	void FloodFill(Point point);
	
	// Check if this polygon contains the given point.
	bool Contains(Point point) const;
	// Check if the given line segment (excluding its endpoints) intersects this
	// polygon. Touching a vertex counts only if the edge the vertex is on is
	// not exactly collinear with the given line segment.
	bool Intersects(Point start, Point end) const;
};



#endif
