/* Avatar.cpp
Copyright 2020 Michael Zahniser
*/

#include "Avatar.h"

#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace {
	// List of sprites to use for facing in any given direction.
	vector<pair<Point, int>> facings;
	// Movement speed, in pixels per frame.
	int speed = 50;
	// The initial facing direction, unless it is changed by a face command.
	const int DEFAULT_FACING = 180;
	
	// Get a vector pointing in the given direction.
	Point Vector(int degrees)
	{
		double radians = degrees * M_PI / 180.;
		// Generate a vector in the specified direction - compass angle, in
		// screen coordinates. So, 0 degrees is (0, -1000) and the x
		// coordinates go positive from there as angle increases.
		const double MAGNITUDE = 1000.;
		return Point(
			round(MAGNITUDE * sin(radians)),
			round(-MAGNITUDE * cos(radians)));
	}
	
	// Get the sprite index for the given facing vector. (By default, return the
	// index for facing straight towards the bottom of the screen.)
	int Facing(Point v = Point(0, 1))
	{
		int bestDot = numeric_limits<int>::min();
		int best = 0;
		for(const pair<Point, int> &it : facings)
		{
			int dot = v.Dot(it.first);
			if(dot > bestDot)
			{
				bestDot = dot;
				best = it.second;
			}
		}
		return best;
	}
};



// Load the avatar definition from a data file.
void Avatar::Load(Data &data)
{
	while(data.Next() && data.Size())
	{
		if(data.Tag() == "sprite" && data.Size() >= 2)
			facings.emplace_back(Vector(data[2]), data[1]);
		else if(data.Tag() == "speed" && data.Size() >= 2)
			speed = data[1];
	}
}



// Get the room the avatar is in.
Room *Avatar::Location() const
{
	return location;
}



// Get the avatar's current position.
Point Avatar::Position() const
{
	return position;
}



// Get the sprite to use, given the direction the avatar is facing in.
int Avatar::SpriteIndex() const
{
	return spriteIndex;
}



// Get the avatar's movement speed.
int Avatar::Speed() const
{
	return speed;
}



// Set the location.
void Avatar::Enter(Point point, Room *room)
{
	// If the avatar has not been placed yet, set its facing angle too.
	if(!spriteIndex)
		Face(DEFAULT_FACING);
	
	if(room)
		location = room;
	position = point;
}



// Move to the given location, and turn to face the direction you moved in.
void Avatar::Move(Point point)
{
	Point v = point - position;
	position = point;
	spriteIndex = Facing(v);
}



// Face in the given direction.
void Avatar::Face(int degrees)
{
	spriteIndex = Facing(Vector(degrees));
}
