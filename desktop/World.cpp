/* World.cpp
Copyright 2020 Michael Zahniser
*/

#include "World.h"

#include "Color.h"
#include "Font.h"
#include "Menu.h"
#include "Room.h"
#include "Sprite.h"
#include "Variables.h"

#include <cmath>
#include <fstream>
#include <map>

using namespace std;

namespace {
	// Path to the saved game file.
	string savePath;
	// Configuration options:
	int frameRate = 8;
	
	// Store two vectors of rooms: one with their initial states, and one with
	// any changes that have occurred due to in-game events.
	map<string, Room> roomInit;
}



// Parse the game configuration and return the title of the game. If the
// returned string is empty, parsing the configuration failed.
string World::LoadConfig(Data &data)
{
	// When this function is called, the Data object must be at the start of
	// the data file, at a "game" line.
	if(data.Tag() != "game")
		return "";
	string title = data.Value();
	
	// Figure out the path to the saved game folder:
	char *path = SDL_GetPrefPath("whimsy", "");
	savePath = path;
	SDL_free(path);
	// Fix Windows-style directory separators.
	for(char &c : savePath)
		if(c == '\\')
			c = '/';
	// Note that SDL guarantees the path ends with a directory separator.
	savePath += title + ".txt";
	
	while(data.Next() && data.Size())
	{
		if(data.Tag() == "fps")
			frameRate = data[1];
	}
	return title;
}



// Configuration options:
int World::FrameRate()
{
	return frameRate;
}



// Load all the data from the given file.
void World::Load(Data &data)
{
	// Load the sprite sheets.
	for( ; data; data.Next())
	{
		if(data.Tag() == "index")
			Sprite::SetIndex(data);
		else if(data.Tag() == "sheet")
			Sprite::LoadSheet(data);
		else if(data.Tag() == "sprite")
			Sprite::Add(data);
		else if(data.Tag() == "style")
			Font::Add(data);
		else if(data.Tag() == "menu")
			Menu::Add(data);
		else if(data.Tag() == "avatar")
			Avatar::Load(data);
		else if(data.Tag() == "init")
			Dialog::Load(data);
		else if(data.Tag() == "dialog")
			Dialog::Load(data);
		else if(data.Tag() == "room")
			roomInit[data.Value()].Load(data);
	}
}



// Default constructor.
World::World()
	: dialog(*this)
{
}



// Check if this World object has been initialized yet.
World::operator bool() const
{
	// If the avatar's location is set, a game is loaded.
	return avatar.Location();
}



// Initialize the world, reading any data from the saved game file.
bool World::Init()
{
	Reset();
	
	// Load any changes in the saved game file.
	ifstream in(savePath);
	if(!in)
		return false;
	
	string line;
	static vector<string> lines;
	while(getline(in, line))
		lines.push_back(line);
	
	// Apply those changes. Note that in the process of applying them they will
	// get added to the "changes" record all over again.
	dialog.Begin(lines);
	
	// If the avatar was placed successfully, pathfinding will now work.
	return InitPathfinding();
}



// Reset this object to the world's initial state (i.e. a new game).
bool World::New()
{
	Reset();
	// Run the "init" dialog. It is responsible for placing the avatar.
	dialog.Begin("");
	
	// If the avatar was placed successfully, pathfinding will now work.
	return InitPathfinding();
}



// Save the current game state.
void World::Save()
{
	// We can't save the state if the world is not initialized.
	if(!avatar.Location())
		return;
	
	ofstream out(savePath);
	out << changes.str();
	
	Variables::Save(out);
	dialog.Save(out);
	
	out << "enter " << avatar.Position().X() << "," << avatar.Position().Y()
		<< " " << avatar.Location()->Name() << '\n';
}



// Draw the world. Use the given mouse position to check if any of the
// interaction icons are hovered over.
void World::Draw(SDL_Surface *screen, Point hover) const
{
	// Note: this function will never be called unless the world is "loaded,"
	// meaning that the avatar is in a valid room.
	Room &room = *avatar.Location();
	
	// Insert the avatar into the room, then draw it, then remove the avatar.
	int sprite = avatar.SpriteIndex();
	viewOffset = Point(
		screen->w, screen->h - Sprite::Get(sprite).Bounds().y) / -2;
	int index = room.Add(sprite, avatar.Position());
	room.Draw(screen, avatar.Position() + viewOffset, hover, !dialog.IsOpen());
	room.Remove(index);
	
	// Draw the dialog overlay.
	if(dialog.IsOpen())
		dialog.Draw(screen, hover);
}



// Handle an event, and return true if the screen must be redrawn.
bool World::Handle(const SDL_Event &event)
{
	// If the dialog is open, it "traps" all events.
	if(dialog.IsOpen())
		return dialog.Handle(event);
	
	// Note: this function will never be called unless the world is "loaded,"
	// meaning that the avatar is in a valid room.
	Room &room = *avatar.Location();
	
	if(event.type == SDL_MOUSEBUTTONDOWN)
	{
		Point target = Point(event.button.x, event.button.y) + avatar.Position() + viewOffset;
		const Interaction *interaction = room.Button(target);
		if(interaction)
		{
			// Assume that triggering this interaction will require redrawing.
			// It's not a big deal if it doesn't, because the user is not
			// clicking on interactions that frequently.
			Trigger(*interaction);
			return true;
		}
		else
			path = paths.Find(avatar.Position(), target);
	}
	else if(event.type == SDL_MOUSEMOTION)
	{
		// Check if the hover state of any interaction icon should change.
		Point target = Point(event.motion.x, event.motion.y) + avatar.Position() + viewOffset;
		Point previous = target - Point(event.motion.xrel, event.motion.yrel);
		
		if(room.Button(target) != room.Button(previous))
			return true;
	}
	
	return false;
}



// Step forward one frame, moving the avatar and checking interactions.
void World::Step()
{
	// If the dialog is open, the world is "paused."
	if(dialog.IsOpen())
		return;
	
	Point position = avatar.Position();
	
	// If we're here, the path must not be empty. But, we do need to check
	// if the avatar has reached one of the waypoints.
	float step = avatar.Speed();
	while(!path.empty() && step > 0.f)
	{
		Point d = path.back() - position;
		float length = d.Length();
		if(length < step)
		{
			position = path.back();
			path.pop_back();
			step -= length;
		}
		else
		{
			float scale = step / length;
			position += Point(round(d.X() * scale), round(d.Y() * scale));
			break;
		}
	}
	// Move the avatar to the calculated position.
	avatar.Move(position);
	
	// Check what interaction zone states have changed. If any change, nothing
	// needs to be done here unless they're "immediate" interactions.
	Room &room = *avatar.Location();
	for(Interaction &it : room.Interactions())
		if(it.SetState(position) == Interaction::IMMEDIATE)
			Trigger(it);
}



// These functions are used by Dialog to change the game world in response
// to certain events.
void World::Enter(Point position, const string &room)
{
	// If we're moving to a new room, clear interaction states in the old one.
	if(!room.empty() && avatar.Location())
		for(Interaction &it : avatar.Location()->Interactions())
			it.ClearState();
	
	// Move the avatar to the new room, if one is given.
	map<string, Room>::iterator it = rooms.find(room);
	avatar.Enter(position, it == rooms.end() ? nullptr : &it->second);
	
	// An enter event should always interrupt movement and redo pathfinding.
	// Even if we're in the same room, we may be in a different, disjoint
	// section of the room.
	InitPathfinding();
	
	// Entering a room may trigger interactions, but if so those interactions
	// are not allowed to directly trigger another "entering" action. (Dialogs
	// run by the interaction may still use an "entering" command, so it is
	// still possible, but less likely, to cause an infinite loop.)
	if(avatar.Location())
		for(Interaction &it : avatar.Location()->Interactions())
			if(it.SetState(position) == Interaction::IMMEDIATE)
				Trigger(it, true);
}



void World::Face(int degrees)
{
	avatar.Face(degrees);
}



// Add a sprite to the given room (or the avatar's current room, if none is
// specified). The sprite can optionally be given a name.
void World::Add(int sprite, Point center, const string &name, const string &room)
{
	Room *location = FindRoom(room);
	if(!location)
		return;
	
	location->Add(sprite, center, name);
	changes << "add " << location->Name() << '\n' << "  " << sprite << " " << center;
	if(!name.empty())
		changes << " " << name;
	changes << '\n';
}



// Add an interaction to the given room, or the avatar's current room if no
// room is specified.
void World::Add(const Interaction &interaction, const string &room)
{
	Room *location = FindRoom(room);
	if(!location)
		return;
	
	location->Add(interaction);
	changes << "add " << location->Name() << '\n';
	interaction.Save(changes, "  ");
}



// Remove any sprites or interactions with the given name from the given
// room, or the avatar's current room if no room is given.
void World::Remove(const string &name, const string &room)
{
	Room *location = FindRoom(room);
	if(!location)
		return;
	
	location->Remove(name);
	changes << "remove " << location->Name() << '\n' << "  " << name << '\n';
}



// Reset the state of this world to the initial state.
void World::Reset()
{
	rooms = roomInit;
	avatar = Avatar();
	Variables::Clear();
	changes.clear();
	path.clear();
	dialog.Close();
}



// Check whether the avatar's location is valid. If so, initialize
// pathfinding and return true; otherwise return false;
bool World::InitPathfinding()
{
	if(!avatar.Location())
		return false;
	
	// Reset the pathfinding state.
	path.clear();
	paths.Init(*avatar.Location(), avatar.Position());
	return true;
}



// For an Add() or Remove() call, find the room it applies to. If the given
// string is empty, the avatar's location will be used. Otherwise, the name
// will be looked up and null is returned if it is not a valid room.
Room *World::FindRoom(const string &room)
{
	if(room.empty())
		return avatar.Location();
	map<string, Room>::iterator it = rooms.find(room);
	return (it == rooms.end() ? nullptr : &it->second);
}



// Trigger the given interaction.
void World::Trigger(const Interaction &interaction, bool entering)
{
	if(interaction.HasDialog())
		dialog.Begin(interaction.DialogName());
	if(interaction.HasEnter() && !entering)
		Enter(interaction.EnterPosition(), interaction.EnterRoom());
}
