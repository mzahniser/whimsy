/* Ring.cpp
Copyright 2020 Michael Zahniser
*/

#include "Ring.h"

#include "Edge.h"

#include <algorithm>
#include <cstdint>

using namespace std;



// Translate a ring by the given vector.
Ring Ring::operator+(Point offset) const
{
	Ring result = *this;
	result += offset;
	return result;
}



Ring &Ring::operator+=(Point offset)
{
	for(Point &point : *this)
		point += offset;
	return *this;
}



Ring Ring::operator-(Point offset) const
{
	return *this + -offset;
}



Ring &Ring::operator-=(Point offset)
{
	return (*this += -offset);
}



// Scale a ring by the given amount.
Ring Ring::operator*(int scale) const
{
	Ring result = *this;
	result *= scale;
	return result;
}



Ring &Ring::operator*=(int scale)
{
	for(Point &point : *this)
		point *= scale;
	return *this;
}



Ring operator*(int scale, const Ring &ring)
{
	Ring result = ring;
	result *= scale;
	return result;
}



Ring Ring::operator/(int scale) const
{
	Ring result = *this;
	result /= scale;
	return result;
}



Ring &Ring::operator/=(int scale)
{
	for(Point &point : *this)
		point /= scale;
	return *this;
}



// Check if this Ring contains the given point.
bool Ring::Contains(Point point) const
{
	// Use the "winding number" method, which is more robust than ray casting.
	return Winding(point).first;
}



// Get the area of this Ring. If the area is negative, this is a hole, i.e.
// a ring that subtracts from a polygon instead of adding to it.
float Ring::Area() const
{
	// Break the polygon up into triangles and add up their areas. The triangles
	// are each formed using an edge of this ring as one edge, and connecting
	// them all to the same arbitrary point as the third vertex. That vertex
	// does not even need to be inside the polygon, so simplify the math by just
	// using the origin as the third point.
	
	// The area of a triangle is half the cross product of the two vectors
	// coming out of any of the vertices. Note that the area of a polygon with
	// very many vertices may overflow the size of a 32-bit int.
	int64_t area = 0;
	for(Edge edge(*this); edge; ++edge)
		area += edge.Start().Cross(edge.End());
	return 0.5f * area;
}



// Check if this Ring is a hole, i.e. a counter-clockwise polygon.
bool Ring::IsHole() const
{
	return (Area() < 0);
}



// Reverse this ring, in-place.
void Ring::Reverse()
{
	reverse(begin(), end());
}



// Get all the points on this ring that are concave, i.e. that might be a
// pathfinding waypoint.
vector<Point> Ring::ConcavePoints() const
{
	// If this is a degenerate polygon, every point is an obstacle.
	if(size() < 3)
		return *this;
	
	vector<Point> result;
	Point prev = (*this)[size() - 2];
	Point here = back();
	for(const Point &next : *this)
	{
		if((here - prev).Cross(next - prev) < 0)
			result.push_back(here);
		prev = here;
		here = next;
	}
	return result;
}



// Assuming that this Ring does not intersect the given ring, as defined in
// Polygon::Add(), check if the given ring is entirely contained in this.
bool Ring::Contains(const Ring &ring) const
{
	// The given ring is contained in this one if any of its vertices are
	// inside this one according to winding number. If the tested vertex was on
	// the border of this ring, we also have to check whether this ring is the
	// bigger of the two.
	pair<int, int> result = Winding(ring.front());
	return result.first && (!result.second || Area() > ring.Area());
}



// Winding-number algorithm for internal use: calculates a winding number to
// check if a point is in a polygon, and also checks if the point is on the
// boundary of the polygon.
pair<int, int> Ring::Winding(Point point) const
{
	int winding = 0;
	int border = 0;
	
	for(Edge edge(*this); edge; ++edge)
	{
		bool startsBelow = (edge.Start().Y() <= point.Y());
		bool endsBelow = (edge.End().Y() <= point.Y());
		// Check if a horizontal line through the point intersects this edge. It
		// does so if one end of the edge is below it and the other is above.
		if(startsBelow == endsBelow)
			continue;
		
		// Update the winding number and the border count based on this
		// cross product, which shows what side of the edge the point is on.
		int cross = edge.Vector().Cross(point - edge.Start());
		winding += (!endsBelow && cross > 0);
		winding -= (endsBelow && cross <= 0);
		border += !cross;
	}
	return make_pair(winding, border);
}
