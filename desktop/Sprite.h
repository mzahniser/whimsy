/* Sprite.h
Copyright 2020 Michael Zahniser
*/

#ifndef SPRITE_H_
#define SPRITE_H_

#include "Data.h"
#include "Point.h"
#include "Polygon.h"
#include "Rect.h"

#include <SDL2/SDL.h>

#include <vector>

using namespace std;



class Sprite {
public:
	// The given data object is currently at an "index" command. Set the index
	// to be used for the next sprite, based on it.
	static void SetIndex(Data &data);
	// The given data object is currently at a "sheet" command. Load the sprite
	// sheet image file that it specifies.
	static void LoadSheet(Data &data);
	// The given data object is currently at the start of a "sprite" object.
	// Read the sprite information and advance to the end of that data block.
	// Returns the index of the sprite that was added.
	static int Add(Data &data);
	// Get the sprite with the given index. If no sprite with that index exists,
	// this will return an "empty" sprite.
	static const Sprite &Get(int index);
	// Free all the sprite sheets.
	static void FreeAll();
	
	// Step the animation forward.
	static void Step();
	
	
public:
	// This is the rectangle to be used for drawing the sprite, as an offset
	// from the "center" point.
	const Rect &Bounds() const;
	// This is what layer of the map the sprite is in. Layer 0 is sprites that
	// have a vertical component, and thus are sorted by center Y.
	int Layer() const;
	// This is the sprite's collision mask. If it's "inside out" (negative area)
	// it creates walkable space instead of blocked space.
	const Polygon &Mask() const;
	// Convenience functions to get the sprite dimensions.
	int Width() const;
	int Height() const;
	
	// Draw this sprite with its center baseline at the given position.
	void Draw(SDL_Surface *surface, Point center) const;
	// Draw this sprite in a palette, at the given zoom. Return the draw width.
	int Draw(SDL_Surface *surface, Point corner, double zoom) const;
	
	
private:
	// The sprite sheet containing this sprite.
	SDL_Surface *sheet = nullptr;
	// Bounding box of the sprite within the spritesheet.
	vector<Rect> source;
	// Bounding box to use when drawing the sprite.
	Rect bounds;
	// Collision mask.
	Polygon mask;
	// Layer 0 contains the objects that have baselines (and thus, vertical
	// dimensions) and must be sorted by their Y coordinates. Layers less than
	// 0 are under that, and greater than 0 are over.
	int layer = 0;
};


#endif
