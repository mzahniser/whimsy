/* World.h
Copyright 2020 Michael Zahniser
*/

#ifndef WORLD_H_
#define WORLD_H_

#include "Avatar.h"
#include "Data.h"
#include "Dialog.h"
#include "Interaction.h"
#include "Paths.h"
#include "Point.h"

#include <SDL2/SDL.h>

#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;



class World {
public:
	// Parse the game configuration and return the title of the game. If the
	// returned string is empty, parsing the configuration failed.
	static string LoadConfig(Data &data);
	// Configuration options:
	static int FrameRate();
	
	// Load all the data from the given file.
	static void Load(Data &data);
	
	
public:
	// Default constructor.
	World();
	
	// Check if this World object has been initialized yet.
	operator bool() const;
	// Initialize the world, reading any data from the saved game file.
	bool Init();
	// Reset this object to the world's initial state (i.e. a new game).
	bool New();
	// Save the current game state.
	void Save();
	
	// Draw the world. Use the given mouse position to check if any of the
	// interaction icons are hovered over.
	void Draw(SDL_Surface *screen, Point hover) const;
	// Handle an event, and return true if the screen must be redrawn.
	bool Handle(const SDL_Event &event);
	
	// Step forward one frame, moving the avatar and checking interactions.
	void Step();
	
	// These functions are used by Dialog to change the game world in response
	// to certain events.
	void Enter(Point position, const string &room);
	void Face(int degrees);
	// Add a sprite to the given room (or the avatar's current room, if none is
	// specified). The sprite can optionally be given a name.
	void Add(int sprite, Point center, const string &name = "", const string &room = "");
	// Add an interaction to the given room, or the avatar's current room if no
	// room is specified.
	void Add(const Interaction &interaction, const string &room = "");
	// Remove any sprites or interactions with the given name from the given
	// room, or the avatar's current room if no room is given.
	void Remove(const string &name, const string &room = "");
	
	
private:
	// Reset the state of this world to the initial state.
	void Reset();
	// Check whether the avatar's location is valid. If so, initialize
	// pathfinding and return true; otherwise return false;
	bool InitPathfinding();
	// For an Add() or Remove() call, find the room it applies to. If the given
	// string is empty, the avatar's location will be used. Otherwise, the name
	// will be looked up and null is returned if it is not a valid room.
	Room *FindRoom(const string &room);
	// Trigger the given interaction. If the "entering" flag is true, the
	// avatar just moved via an "enter" command so they are 
	void Trigger(const Interaction &interaction, bool entering = false);
	
	
private:
	// State of the active dialog, if any.
	Dialog dialog;
	// Current state of the rooms, including any changes events have made.
	map<string, Room> rooms;
	// Record of changes that have been made to the world, so they can be stored
	// in a saved game and replayed when that game is loaded.
	ostringstream changes;
	
	// Current avatar location.
	Avatar avatar;
	// Offset to use to center the avatar in the view. This is calculated based
	// on the size of the view and the size of the avatar sprite.
	mutable Point viewOffset;
	
	// Pathfinding.
	Paths paths;
	vector<Point> path;
};



#endif
