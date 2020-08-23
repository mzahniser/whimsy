/* Polygon.cpp
Copyright 2020 Michael Zahniser
*/

#include "Polygon.h"

#include "Edge.h"

#include <algorithm>
#include <cstdint>
#include <utility>

using namespace std;

namespace {
	// Get the maximum possible order value for a given ring.
	int MaxOrder(const Ring &ring)
	{
		Edge edge(ring);
		while(edge)
			++edge;
		return edge.Order();
	}
	
	// An Intersection is a point where P and Q intersect. Depending on what
	// algorithm step we're in, the data members are used slightly differently.
	class Intersection {
	public:
		Intersection() = default;
		Intersection(Point point, int order, int index = 0, bool entering = false)
			: point(point), order(order), index(index), entering(entering) {}
		
		// Comparison operator, which will sort intersections by order.
		bool operator<(const Intersection &other) const { return order < other.order; }
		
		// Coordinates of the intersection.
		Point point;
		// A distance value to be used to sort intersections on the same edge.
		int order;
		// The index of the associated vertex: the source vertex, if this is in
		// the first algorithm stage, or the link, in the second stage.
		int index;
		// Whether P is entering Q (as opposed to exiting it) at this point.
		bool entering;
	};
	
	// Sort a list of intersections.
	void Sort(vector<Intersection> &intersections)
	{
		sort(intersections.begin(), intersections.end());
	}
	
	// Any time two intersections occur at the same point, that means a vertex
	// touched but the edge "turned back" and did not pass through the polygon.
	// Those intersections should be erased.
	void RemoveDoubles(vector<Intersection> &intersections)
	{
		size_t j = 0;
		for(size_t i = 0; i < intersections.size(); )
		{
			// As we go, we'll shift the intersections we preserve into the
			// start of the vector.
			intersections[j] = intersections[i];
			// Find all intersections in the list that have the same order (i.e.
			// the same location) as this one. If there are multiples, we will
			// remove all of them if half are entering and half are not, or
			// all but one of them if all or none of them are entering.
			// This gets rid of corners that touch an edge without crossing it.
			int order = intersections[i].order;
			int entering = intersections[i].entering;
			int total = 1;
			for(++i; i < intersections.size() && intersections[i].order == order; ++i)
			{
				entering += intersections[i].entering;
				++total;
			}
			
			// Preserve this intersection if there is only one instance of it,
			// or if all the instances have the same entering flag.
			j += (entering == 0 || entering == total);
		}
		intersections.resize(j);
	}
	
	// A "vertex" is a polygon vertex annotated with (possibly) a link to a
	// different vertex that is equivalent to it.
	class Vertex {
	public:
		Vertex() = default;
		Vertex(Point point, int link = -1) : point(point), link(link) {}
		
		Point point;
		int link = -1;
	};
}



// Translate a ring by the given vector.
Polygon Polygon::operator+(Point offset) const
{
	Polygon result = *this;
	result += offset;
	return result;
}



Polygon &Polygon::operator+=(Point offset)
{
	for(Ring &ring : *this)
		ring += offset;
	return *this;
}



Polygon Polygon::operator-(Point offset) const
{
	return *this + -offset;
}



Polygon &Polygon::operator-=(Point offset)
{
	return (*this += -offset);
}



// Scale a ring by the given amount.
Polygon Polygon::operator*(int scale) const
{
	Polygon result = *this;
	result *= scale;
	return result;
}



Polygon &Polygon::operator*=(int scale)
{
	for(Ring &ring : *this)
		ring *= scale;
	return *this;
}



Polygon operator*(int scale, const Polygon &poly)
{
	Polygon result = poly;
	result *= scale;
	return result;
}



Polygon Polygon::operator/(int scale) const
{
	Polygon result = *this;
	result /= scale;
	return result;
}



Polygon &Polygon::operator/=(int scale)
{
	for(Ring &ring : *this)
		ring /= scale;
	return *this;
}



// A "ring" is a simple polygon. If the points are in clockwise order it is
// a filled polygon; otherwise it is a hole.
void Polygon::Add(const Ring &ring)
{
	// In the comments below, this polygon is called P, and the simple polygon
	// being added is called Q. The output is called R.
	
	// We're going to generate a new output polygon here:
	Polygon result;
	
	// Remember all intersections with all edges on Q, so we can slot them in
	// at the right spots when generating the new Q.
	vector<Intersection> qIntersections;
	int qMaxOrder = MaxOrder(ring);
	
	// This is all the parts in one vector, with intersection vertices added
	// and links between polygons (including joining end to start points).
	vector<Vertex> amplified;
	
	// Remember the smallest part of P that contains Q.
	const Ring *smallestContainer = nullptr;
	
	// Entering and exiting calculations are reversed if Q is a hole.
	bool isHole = ring.IsHole();
	
	
	// Step 1: finding the intersections and generating copies of the P polygons
	// with those intersections inserted:
	
	// Loop through every segment of every part of this polygon, checking for
	// intersections with the given ring. If there are any, generate an
	// "amplified" version of the part with all the intersection vertices added.
	for(const Ring &part : *this)
	{
		// First, find any potential intersections. It's possible that we will
		// generate duplicates here, e.g. if an intersection occurs at a vertex.
		vector<Intersection> pIntersections;
		int pMaxOrder = MaxOrder(part);
		
		for(Edge p(part); p; ++p)
			for(Edge q(ring); q; ++q)
			{
				int cross = p.Vector().Cross(q.Vector());
				if(!cross)
					continue;
				
				// Now, we know the two lines are not parallel, so they must
				// intersect. Calculate where that intersection occurs.
				Point d = q.Start() - p.Start();
				int pT = d.Cross(q.Vector());
				int qT = d.Cross(p.Vector());
				// If the cross product is negative, negate all three so that
				// the comparisons below will work correctly.
				bool entering = (cross > 0) ^ isHole;
				if(cross < 0)
				{
					cross = -cross;
					pT = -pT;
					qT = -qT;
				}
				
				// Check if the intersection occurs within both segments,
				// including their endpoints. If so, record the intersection.
				if(qT >= 0 && qT <= cross && pT >= 0 && pT <= cross)
				{
					// The "order" for this intersection represents how far it
					// is around P. Order values must monotonically increase
					// along each edge and from one edge to the next, and the
					// order of the end of one edge should equal the order of
					// the beginning of the next.
					// 64-bit math is needed for this calculation.
					Point dP(
						static_cast<int64_t>(pT) * p.Vector().X() / cross,
						static_cast<int64_t>(pT) * p.Vector().Y() / cross);
					Point dQ(
						static_cast<int64_t>(qT) * q.Vector().X() / cross,
						static_cast<int64_t>(qT) * q.Vector().Y() / cross);
					pIntersections.emplace_back(
						p.Start() + dP,
						(p.Order() + dP.Dot(dP)) % pMaxOrder,
						(q.Order() + dQ.Dot(dQ)) % qMaxOrder,
						entering);
				}
			}
		// Sort the intersections so we can loop through them in order.
		Sort(pIntersections);
		// Remove any "doubles" in the list. (This also includes quadruples when
		// two polygons cross at their vertices.)
		RemoveDoubles(pIntersections);
		
		// If that removed all the intersections, this part does not intersect
		// the newly added ring.
		if(pIntersections.empty())
		{
			// Parts of P that do not intersect Q should be added to the output
			// as long as they are not entirely contained within Q.
			if(!ring.Contains(part))
			{
				result.emplace_back(part);
				
				// If Q doesn't contain this part of P, check if the opposite is
				// true. If this part contains Q, check if it's contained in any
				// other "container" we've found so far.
				
				// Anything that contains the ring must be either nested in this
				// part or around it; there cannot be any intersection because
				// the rings in P do not intersect.
				if(part.Contains(ring) && (!smallestContainer || smallestContainer->Contains(part)))
					smallestContainer = &part;
			}
			continue;
		}
		// Otherwise, we need to go back around the part to generate the
		// amplified list of vertices.
		
		// Remember the index in the "amplified" vector where this part begins.
		// We'll need that later to construct the closing loop.
		int pIndex = amplified.size();
		
		// If two intersections in a row have the same "entering" flag, ignore
		// the second one.
		bool wasEntering = pIntersections.back().entering;
		// Just to avoid always having to check if we're at the end, insert a
		// sentinel into the end of the candidates list that has a high enough
		// order that we'll never reach it.
		pIntersections.emplace_back(Point(), pMaxOrder);
		vector<Intersection>::const_iterator it = pIntersections.begin();
		for(Edge p(part); p; )
		{
			// Add the start point to the list of amplified vertices. But, don't
			// add it twice if there's an intersection at the start of this segment.
			if(it->order != p.Order())
				amplified.push_back(p.Start());
			// Advance p to the next segment, and then advance the intersection
			// iterator until it reaches the next segment, too.
			for(++p; it->order < p.Order(); ++it)
			{
				// If the edge of one polygon goes from outside to collinear
				// with the other, and then collinear to inside, that generates
				// two "entering" intersections, of which only one should count.
				if(it->entering == wasEntering)
					continue;
				wasEntering = it->entering;
				
				// The "index" field was used to store the Q order.
				qIntersections.emplace_back(it->point, it->index, amplified.size(), it->entering);
				amplified.emplace_back(it->point);
			}
		}
		// Close out the amplified polygon.
		amplified.emplace_back(amplified[pIndex].point, pIndex);
	}
	
	
	// Step 2: generating a copy of Q with the intersections inserted:
	
	// If there were no intersections at all:
	if(qIntersections.empty())
	{
		// Q should be added to P if it has the opposite polarity of the
		// smallest part of P that fully contains Q. (If none, the space outside
		// the polygon counts as having "hole" polarity.)
		bool containerIsHole = (!smallestContainer || smallestContainer->IsHole());
		if(containerIsHole != isHole)
			result.emplace_back(ring);
		
		result.swap(*this);
		return;
	}
	
	// Sort the intersections so we can loop through them in order. This time,
	// we don't need to check for "doubles" since they were removed above.
	Sort(qIntersections);
	
	// Remember the index in the "amplified" vector where this part begins.
	// We'll need that later to construct the closing loop.
	int qIndex = amplified.size();
	
	// Remember what indices in the amplified vector list are possible
	// starting points for a polygon trace.
	vector<int> enteringIndices;
	
	// Just to avoid always having to check if we're at the end, insert a
	// sentinel into the end of the candidates list that has a high enough
	// order that we'll never reach it.
	qIntersections.emplace_back(Point(), qMaxOrder);
	vector<Intersection>::const_iterator it = qIntersections.begin();
	// Loop through the edges of q.
	for(Edge q(ring); q; )
	{
		// Add the start point to the list of amplified vertices. But, don't
		// add it twice if there's an intersection at the start of this segment.
		if(it->order != q.Order())
			amplified.push_back(q.Start());
		// Advance q to the next segment, and then advance the intersection
		// iterator until it reaches the next segment, too.
		for(++q; it->order < q.Order(); ++it)
		{
			// If this is an "entering" intersection, store it as one of the
			// possible places to start a trace.
			if(it->entering)
				enteringIndices.emplace_back(amplified.size());
			// Add links in both directions.
			if(!it->entering)
				amplified[it->index].link = amplified.size();
			amplified.emplace_back(it->point);
			if(it->entering)
				amplified.back().link = it->index;
		}
	}
	// Close out the amplified polygon.
	amplified.emplace_back(amplified[qIndex].point, qIndex);
	
	
	// Step 3: Tracing the output polygons.
	
	// Try each possible starting index.
	for(int index : enteringIndices)
	{
		if(amplified[index].link < 0)
			continue;
		
		// Begin editing a new output polygon.
		result.emplace_back();
		Ring &out = result.back();
		
		int j = index;
		do {
			Vertex &vertex = amplified[j];
			
			// Follow the link, then disconnect it. This is to make sure we
			// only trace each polygon once.
			if(vertex.link >= 0)
			{
				j = vertex.link;
				vertex.link = -2;
			}
			else
			{
				out.emplace_back(vertex.point);
				++j;
			}
		} while(j != index);
		
		// Make sure we didn't create a degenerate polygon.
		if(out.size() < 3)
			result.pop_back();
	}
	result.swap(*this);
}



// Get just the component of this polygon that contains the given point.
// That includes any holes in that polygon.
void Polygon::FloodFill(Point point)
{
	vector<float> area(size(), 0.f);
	vector<bool> keep(size(), false);
	int keepCount = 0;
	for(size_t i = 0; i < size(); ++i)
		area[i] = (*this)[i].Area();
	
	// First, find the smallest positive part that contains the point.
	size_t smallestI = 0;
	for(size_t i = 0; i < size(); ++i)
		if(area[i] > 0.f && (*this)[i].Contains(point) && (!keepCount || area[i] < area[smallestI]))
		{
			keepCount = 1;
			smallestI = i;
		}
	// If the given point is not contained within a polygon, return an empty set.
	if(!keepCount)
	{
		clear();
		return;
	}
	keep[smallestI] = true;
	Ring &smallest = (*this)[smallestI];
	
	// Now, find all holes that are inside that polygon. But, make sure we do
	// not include holes that are nested inside other holes.
	for(size_t i = 0; i < size(); ++i)
		if(area[i] < 0.f && smallest.Contains((*this)[i]))
		{
			bool nested = false;
			// Find all holes larger than this one and see if they contain this
			// one. To contain it, the other hole must be larger, and since
			// holes have negative area that must mean a lower area.
			for(size_t j = 0; j < size(); ++j)
				if(area[j] < area[i] && (*this)[j].Contains((*this)[i].front()))
				{
					nested = true;
					break;
				}
			if(!nested)
				keep[i] = true;
		}
	Polygon result;
	result.reserve(keepCount);
	for(size_t i = 0; i < size(); ++i)
		if(keep[i])
		{
			result.emplace_back();
			result.back().swap((*this)[i]);
		}
	result.swap(*this);
}



// Check if this polygon contains the given point.
bool Polygon::Contains(Point point) const
{
	// Just add up the "winding number" of all the parts of this polygon and
	// check if it is nonzero. As a shortcut, if the point is on any edge of the
	// polygon, it must be inside.
	int winding = 0;
	for(const Ring &part : *this)
	{
		pair<int, int> result = part.Winding(point);
		if(result.second)
			return true;
		winding += result.first;
	}
	return winding;
}



// Check if the given line segment (excluding its endpoints) intersects this
// polygon. Touching a vertex counts only if the edge the vertex is on is
// not exactly collinear with the given line segment.
bool Polygon::Intersects(Point start, Point end) const
{
	Point qV = end - start;
	for(const Ring &part : *this)
		for(Edge p(part); p; ++p)
		{
			int cross = p.Vector().Cross(qV);
			if(!cross)
				continue;
			
			// Now, we know the two lines are not parallel, so they must
			// intersect. Calculate where that intersection occurs.
			Point d = start - p.Start();
			int pT = d.Cross(qV);
			int qT = d.Cross(p.Vector());
			// If the cross product is negative, negate all three so that
			// the comparisons below will work correctly.
			if(cross < 0)
			{
				cross = -cross;
				pT = -pT;
				qT = -qT;
			}
			
			// Check if the intersection occurs within both segments,
			// including their endpoints. If so, record the intersection.
			if(qT > 0 && qT < cross && pT >= 0 && pT <= cross)
				return true;
		}
	return false;
}
