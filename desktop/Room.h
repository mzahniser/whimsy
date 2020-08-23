/* Room.h
Copyright 2020 Michael Zahniser
*/

#ifndef ROOM_H_
#define ROOM_H_

#include "Color.h"
#include "Data.h"
#include "Interaction.h"
#include "Point.h"
#include "Polygon.h"
#include "Rect.h"

#include <SDL2/SDL.h>

#include <string>
#include <vector>

using namespace std;



class Room {
public:
	// Read or write a room to a data file.
	void Load(Data &data);
	void Load(const string &path);
	void Save(const string &path) const;
	
	// Get this room's name.
	const string &Name() const;
	
	// Add a sprite at the given (x, y), and get the index of the sprite to be
	// used if we want to Remove() it. The (x, y) must be in room coordinates.
	int Add(int spriteIndex, Point center, const string &name = "");
	// Add an interaction.
	void Add(const Interaction &interaction);
	// Find the top-most sprite that overlaps the given point. If there are no
	// overlaps, this returns -1.
	int Find(Point center);
	// Remove the sprite with the given index.
	void Remove(int index);
	// Remove the given interaction.
	void Remove(const Interaction *interaction);
	// Remove all sprites and interactions with the give name.
	void Remove(const string &name);
	
	// Draw the entire room, in the given surface, with the given (x, y) offset.
	void Draw(SDL_Surface *screen, Point offset, Point hover, bool hasFocus) const;
	
	// Access the raw list of sprites.
	class Entry;
	const vector<Entry> &Sprites() const;
	
	// Access the list of interactions.
	vector<Interaction> &Interactions();
	const vector<Interaction> &Interactions() const;
	
	// Check if the given point is over an interaction icon. The point should
	// be given in world coordinates, not screen coordinates.
	const Interaction *Button(Point point) const;
	
	
public:
	class Entry {
	public:
		// Constructors.
		Entry(const Data &data);
		Entry(int spriteIndex, Point center, const string &name);
		
		// Comparison operator, defining the sprite ordering.
		bool operator<(const Entry &other) const;
		
		// Get this sprite's bounding rect in room coordinates.
		Rect Bounds() const;
		// Get this sprite's collision mask in room coordinates.
		Polygon Mask() const;
		
		int Index() const;
		Point Center() const;
		const string &Name() const;
		
		
	private:
		int index;
		int layer;
		Point center;
		string name;
	};
	
	
private:
	// Reset to an "empty" state.
	void Reset();
	
	
private:
	string name;
	Color background;
	vector<Entry> sprites;
	vector<Interaction> interactions;
};



#endif
