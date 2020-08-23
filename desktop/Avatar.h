/* Avatar.h
Copyright 2020 Michael Zahniser
*/

#ifndef AVATAR_H_
#define AVATAR_H_

#include "Data.h"
#include "Point.h"
#include "Room.h"

using namespace std;


// Class representing the avatar.
class Avatar {
public:
	// Load the avatar definition from a data file.
	static void Load(Data &data);
	
	
public:
	// Get the room the avatar is in.
	Room *Location() const;
	// Get the avatar's current position.
	Point Position() const;
	// Get the sprite to use, given the direction the avatar is facing in.
	int SpriteIndex() const;
	// Get the avatar's movement speed.
	int Speed() const;
	
	// Set the location.
	void Enter(Point point, Room *room = nullptr);
	// Move to the given location, and turn to face the direction you moved in.
	void Move(Point point);
	// Face in the given direction.
	void Face(int degrees);
	
	
private:
	// Initial position:
	Room *location = nullptr;
	Point position;
	int spriteIndex = 0;
};



#endif
