/* Sprite.cpp
Copyright 2020 Michae Zahniser
*/

#include "Sprite.h"

#include <cmath>
#include <iostream>
#include <vector>

#include <SDL2/SDL_image.h>

using namespace std;

namespace {
	// The vector of sprites. Note that sprite 0 is a "null" sprite placeholder.
	vector<Sprite> sprites;
	
	size_t nextIndex = 1;
	vector<SDL_Surface *> sheets;
	
	size_t step = 0;
}



// The given data object is currently at an "index" command. Set the index
// to be used for the next sprite, based on it.
void Sprite::SetIndex(Data &data)
{
	nextIndex = data[1];
}



// The given data object is currently at a "sheet" command. Load the sprite
// sheet image file that it specifies.
void Sprite::LoadSheet(Data &data)
{
	string path = data.Directory() + data.Value();
	sheets.emplace_back(IMG_Load(path.c_str()));
}



// The given data object is currently at the start of a "sprite" object.
// Read the sprite information and advance to the end of that data block.
int Sprite::Add(Data &data)
{
	// Bail out if no sprite sheet has been loaded.
	if(sheets.empty() || !sheets.back())
		return 0;
	// Resize the sprite array if necessary to include this index.
	if(nextIndex >= sprites.size())
		sprites.resize(nextIndex + 1);
	
	Sprite &sprite = sprites[nextIndex];
	sprite.sheet = sheets.back();
	// Loop until we find an empty line, interpreting every line until than as
	// a part of the data for this sprite.
	bool hasBaseline = false;
	while(data.Next() && data.Size())
	{
		if(data.Tag() == "bounds")
		{
			Point a = data[1];
			Point b = data[2];
			if(!sprite.source.empty())
				b = a + Point(sprite.source.front().w, sprite.source.front().h);
			sprite.source.emplace_back(a, b);
		}
		else if(data.Tag() == "baseline" && data.Size() == 2)
		{
			sprite.bounds.y = data[1];
			hasBaseline = true;
		}
		else if(data.Tag() == "layer" && data.Size() == 2)
			sprite.layer = data[1];
		else if(data.Tag() == "mask")
		{
			sprite.mask.emplace_back();
			vector<Point> &part = sprite.mask.back();
			for(size_t i = 1; i < data.Size(); ++i)
				part.push_back(data[i]);
			// TODO: Make sure "allow" masks are inside-out.
		}
		else
			cerr << "Sprite: error:" << data.Line() << endl;
	}
	// Coordinates for the sprite mask, etc.  should be relative to this point.
	// If no baseline was given, use the middle of the sprite. Otherwise,
	// convert the baseline to a distance from the top of the sprite.
	Point anchor(
		sprite.source.front().x + sprite.source.front().w / 2,
		hasBaseline ? sprite.bounds.y : sprite.source.front().y + sprite.source.front().h / 2);
	// Shift the bounds and mask coordinates to be relative to the anchor.
	sprite.bounds = sprite.source.front() - anchor;
	sprite.mask -= anchor;
	
	// Return the index of this sprite, then increment to the next one.
	return nextIndex++;
}



// Get the sprite with the given index. If no sprite with that index exists,
// this will return an "empty" sprite.
const Sprite &Sprite::Get(int index)
{
	// If the given sprite is undefined, return the null placeholder.
	if(static_cast<size_t>(index) >= sprites.size())
		return sprites[0];
	
	return sprites[index];
}



// Free all the sprite sheets.
void Sprite::FreeAll()
{
	for(SDL_Surface *surface : sheets)
		if(surface)
			SDL_FreeSurface(surface);
	sheets.clear();
}



	
// Step the animation forward.
void Sprite::Step()
{
	++step;
}



// This is the rectangle to be used for drawing the sprite, as an offset
// from the "center" point.
const Rect &Sprite::Bounds() const
{
	return bounds;
}



// This is the sprite's collision mask. If it's "inside out" (negative area)
// it creates walkable space instead of blocked space.
const Polygon &Sprite::Mask() const
{
	return mask;
}



// This is what layer of the map the sprite is in. Layer 0 is sprites that
// have a vertical component, and thus are sorted by center Y.
int Sprite::Layer() const
{
	return layer;
}



// Convenience functions to get the sprite dimensions.
int Sprite::Width() const
{
	return source.empty() ? 0 : source.front().w;
}



int Sprite::Height() const
{
	return source.empty() ? 0 : source.front().h;
}



// Draw this sprite with its center baseline at the given position.
void Sprite::Draw(SDL_Surface *surface, Point center) const
{
	// Make sure this sprite actually has an image defined.
	if(source.empty())
		return;
	
	Rect rect = bounds + center;
	SDL_BlitSurface(sheet, &source[step % source.size()], surface, &rect);
}



// Draw this sprite in a palette, at the given zoom.
int Sprite::Draw(SDL_Surface *surface, Point corner, double zoom) const
{
	// Make sure this sprite actually has an image defined.
	if(source.empty())
		return 0;
	
	// The given corner is the lower left. Figure out what the bounding box is.
	int w = round(source.front().w * zoom);
	int h = round(source.front().h * zoom);
	Rect rect(corner.X(), corner.Y() - h, w, h);
	SDL_BlitScaled(sheet, &source.front(), surface, &rect);
	
	return w;
}
