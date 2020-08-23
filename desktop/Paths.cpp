/* Paths.cpp
Copyright 2020 Michael Zahniser
*/

#include "Paths.h"

#include "Sprite.h"

#include <cmath>
#include <limits>
#include <queue>

using namespace std;

namespace {
	const int INTERNAL_SCALE = 4;
	
	// The state of a partially completed path.
	class Node {
	public:
		Node() = default;
		Node(float length, int index, int previous = -1)
			: length(length), index(index), previous(previous) {}
		
		// Sort function for the priority queue. Priority queues return the
		// "greatest" value first, so we need to invert the comparison here so
		// being less actually means being longer (i.e. less optimal).
		bool operator<(const Node &p) const { return length > p.length; }
		
		// Total length of this path, including the heuristic.
		float length = 0.f;
		// Current waypoint index.
		int index = -1;
		// Previous waypoint index;
		int previous = -1;
	};
}



// Initialize pathfinding with the given room and the given starting point
// for the avatar. Assume that the avatar will never get out of the polygon
// they start in. (If the room changes because sprites are added or removed,
// pathfinding must be recalculated from scratch.)
void Paths::Init(const Room &room, Point point)
{
	// Clear any previous pathfinding data.
	passable.clear();
	waypoints.clear();
	
	// Generate the collision mask for this room.
	for(const Room::Entry &entry : room.Sprites())
		if(!entry.Mask().empty())
		{
			Polygon mask = entry.Mask() * INTERNAL_SCALE;
			for(const Ring &ring : mask)
				passable.Add(ring);
		}
	// Prune out everything but the polygon the avatar is moving around in.
	passable.FloodFill(point * INTERNAL_SCALE);
	
	// Now, calculate all the sightlines in the room.
	for(const Ring &part : passable)
	{
		// Skip degenerate polygons. (Just in case, although there should be none.)
		if(part.size() < 3)
			continue;
		
		Point prev = part[part.size() - 2];
		Point here = part.back();
		for(const Point &next : part)
		{
			Point back = prev - here;
			Point forward = next - here;
			if(back.Cross(forward) >= 0)
				AddWaypoint(here, back, forward);
			prev = here;
			here = next;
		}
	}
}



// Given that the avatar is trying to move to the given point, get a list of
// waypoints it must visit along the way. If the given point is outside the
// avatar's polygon, it will move to the closest vertex of the polygon.
vector<Point> Paths::Find(Point from, Point to) const
{
	// If for some reason we have no mask, bail out.
	if(passable.empty())
		return vector<Point>();
	
	// First, convert into internal coordinates.
	from *= INTERNAL_SCALE;
	to *= INTERNAL_SCALE;
	
	// Check that the target point is in the mask. If it's not, replace it with
	// the closest vertex that is.
	if(!passable.Contains(to))
		to = ClosestVertex(to);
	
	// If there's a direct path between the two, return it.
	if(Visible(from, to))
		return vector<Point>(1, to / INTERNAL_SCALE);
	
	// Calculate the distance from every waypoint to the target point, and also
	// check which waypoints are in direct sight of the target.
	CalculateDistances(to);
	
	// Now, we know there's no direct point to the target, but an indirect path
	// must exist. Use A* search to find it.
	priority_queue<Node> queue;
	
	// Start by adding the sightlines from the start node.
	for(size_t i = 0; i < waypoints.size(); ++i)
		if(Visible(from, waypoints[i]))
		{
			float distance = from.Distance(waypoints[i]);
			queue.emplace(distance + waypoints[i].distance, i);
			// Remember that we can get directly back to the beginning from here.
			waypoints[i].backtrack = -1;
			waypoints[i].shortest = distance;
		}
	
	// Now, iterate until we've found the best path.
	float bestDistance = numeric_limits<float>::infinity();
	Node best;
	while(!queue.empty() && queue.top().length < bestDistance)
	{
		Node node = queue.top();
		queue.pop();
		
		// Check if there's a direct line to the end from here. If so, the best
		// path from here is to go straight to it.
		if(waypoints[node.index].visible)
		{
			bestDistance = node.length;
			best = node;
			continue;
		}
		
		// Otherwise, queue up paths through all the waypoints visible from
		// here. Remove the heuristic segment from the path length first.
		node.length -= waypoints[node.index].distance;
		for(const pair<int, float> &it : waypoints[node.index].sightlines)
		{
			const Waypoint &next = waypoints[it.first];
			
			// Check if this is the shortest path we've found to this node.
			float length = node.length + it.second;
			if(length >= next.shortest)
				continue;
			
			next.shortest = length;
			next.backtrack = node.index;
			queue.emplace(length + next.distance, it.first, node.index);
		}
	}
	
	// If we were unable to find a path, return an empty vector.
	if(best.index < 0)
		return vector<Point>();
	
	// Backtrack to find the path.
	vector<Point> path;
	path.push_back(to / INTERNAL_SCALE);
	int i = best.index;
	while(i != -1)
	{
		path.push_back(waypoints[i] / INTERNAL_SCALE);
		i = waypoints[i].backtrack;
	}
	// Note: the path is returned in stack format, i.e. with the first waypoint
	// at the end because it is the first one we will want to pop.
	return path;
}



const Polygon &Paths::Passable() const
{
	return passable;
}



// The given vertex is a possible pathfinding waypoint. Add it to the list
// and check sightlines to all the other waypoints.
void Paths::AddWaypoint(Point vertex, Point back, Point forward)
{
	// Add this vertex, and remember its index.
	int end = waypoints.size();
	waypoints.emplace_back(vertex);
	
	// For each other waypoint, check the sightline to this one.
	for(int i = 0; i < end; ++i)
	{
		Point angle = waypoints[i] - vertex;
		// We know that this is a concave vertex, i.e. the sightlines from it
		// spread across more than a 180 degree angle. So, the easiest way to
		// check if a vector goes into the polygon from here is to check if it
		// is not going into the smaller angle outside.
		if((back.Cross(angle) <= 0 || angle.Cross(forward) <= 0) && !passable.Intersects(waypoints[i], vertex))
		{
			float distance = angle.Length();
			waypoints[i].sightlines.emplace_back(end, distance);
			waypoints[end].sightlines.emplace_back(i, distance);
		}
	}
}



// Find the closest passable vertex to the given point.
Point Paths::ClosestVertex(Point target) const
{
	int bestDistance = numeric_limits<int>::max();
	Point bestPoint;
	
	for(const Ring &part : passable)
		for(const Point &point : part)
		{
			int distance = target.DistanceSquared(point);
			if(distance < bestDistance)
			{
				bestDistance = distance;
				bestPoint = point;
			}
		}
	
	return bestPoint;
}



// Calculate the distance from every waypoint to the target point, and also
// check which waypoints are in direct sight of the target.
void Paths::CalculateDistances(Point target) const
{
	for(const Waypoint &waypoint : waypoints)
	{
		waypoint.distance = target.Distance(waypoint);
		waypoint.visible = Visible(waypoint, target);
		waypoint.backtrack = -1;
		waypoint.shortest = numeric_limits<float>::infinity();
	}
}



// Check if we can travel along the given sightline.
bool Paths::Visible(Point from, Point to) const
{
	// It's possible that both endpoints coincide with vertices of the mask.
	// If so, we need to know that the path between them goes through the
	// polygon, not through a hole in the polygon.
	// TODO: Polygon::Intersects() could be modified to check this directly,
	// which might be more efficient.
	return !passable.Intersects(from, to) && passable.Contains((from + to) / 2);
}
