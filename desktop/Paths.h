/* Paths.h
Copyright 2020 Michael Zahniser
*/

#ifndef PATHS_H_
#define PATHS_H_

#include "Point.h"
#include "Polygon.h"
#include "Room.h"

#include <utility>
#include <vector>

using namespace std;



class Paths {
public:
	// Initialize pathfinding with the given room and the given starting point
	// for the avatar. Assume that the avatar will never get out of the polygon
	// they start in. (If the room changes because sprites are added or removed,
	// pathfinding must be recalculated from scratch.)
	void Init(const Room &room, Point point);
	// Given that the avatar is trying to move to the given point, get a list of
	// waypoints it must visit along the way. If the given point is outside the
	// avatar's polygon, it will move to the closest vertex of the polygon. If
	// an empty vector is returned, the source point isn't in the polygon.
	vector<Point> Find(Point from, Point to) const;
	
	const Polygon &Passable() const;
	
	
private:
	// The given vertex is a possible pathfinding waypoint. Add it to the list
	// and check sightlines to all the other waypoints.
	void AddWaypoint(Point vertex, Point back, Point forward);
	// Find the closest passable vertex to the given point.
	Point ClosestVertex(Point target) const;
	// Calculate the distance from every waypoint to the target point, and also
	// check which waypoints are in direct sight of the target.
	void CalculateDistances(Point target) const;
	// Check if we can travel along the given sightline.
	bool Visible(Point from, Point to) const;
	
	class Waypoint : public Point {
	public:
		Waypoint(Point p = Point()) : Point(p) {}
		
		// Links to all other "visible" concave vertices.
		vector<pair<int, float>> sightlines;
		// Store the distance to the target node so it only has to be calculated
		// once.
		mutable float distance = 0.f;
		// Remember whether we can go straight to the target from here.
		mutable bool visible = false;
		// Store the shortest way to get to this node, so we can backtrack to
		// find the whole A* path instead of storing the path in every A* entry.
		mutable int backtrack = -1;
		mutable float shortest = 0.f;
	};
	
	
private:
	Polygon passable;
	vector<Waypoint> waypoints;
};



#endif
