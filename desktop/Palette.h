/* Palette.h
Copyright 2020 Michael Zahniser
*/

#ifndef PALETTE_H_
#define PALETTE_H_

#include <SDL2/SDL.h>

#include <string>
#include <vector>

using namespace std;



class Palette {
public:
	// Assume that the given data file contains nothing but sprite definitions,
	// i.e. "index" and "sheet" commands and "sprite" objects. Parse the entire
	// file, loading the sprites and remembering which sheet each goes with.
	void Load(const string &path);
	
	// Find out how many sprite sheets are available.
	int Sheets() const;
	// Change which palette is active.
	void Select(int sheet);
	// Get the index of the selected sheet.
	int Selected() const;
	
	// Get the name of the selected palette.
	const string &Name() const;
	// Get the name of the palette with the given index.
	const string &Name(int sheet) const;
	
	// Draw the selected palette at the bottom of the given surface, taking up
	// as much vertical space is necessary (but no more than the height of the
	// tallest sprite, and no more than the maximum height).
	void Draw(SDL_Surface *surface) const;
	
	// Get the Y position of the top of the palette.
	int Top() const;
	// Get the slot index of the sprite at the given X position.
	int Slot(int x) const;
	// Get the sprite in the given slot.
	int Index(int slot) const;
	// Get the total number of slots.
	int Slots() const;
	
	
private:
	class Sheet {
	public:
		Sheet(string name) : name(name) {}
		
		string name;
		vector<int> sprites;
	};
	
	
private:
	// Store a collection of sprite sheets.
	vector<Sheet> sheets;
	int selected = 0;
	// Remember where the palette was drawn.
	mutable int top;
	mutable vector<int> positions;
};



#endif
