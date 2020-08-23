/* Palette.cpp
Copyright 2020 Michael Zahniser
*/

#include "Palette.h"

#include "Color.h"
#include "Data.h"
#include "Font.h"
#include "Rect.h"
#include "Sprite.h"

#include <algorithm>

using namespace std;

namespace {
	const double MAX_HEIGHT = 200.;
	const double MAX_ZOOM = 0.5;
	
	Color lineColor(0);
	Color backColor(200);
}



// Assume that the given data file contains nothing but sprite definitions,
// i.e. "index" and "sheet" commands and "sprite" objects. Parse the entire
// file, loading the sprites and remembering which sheet each goes with.
void Palette::Load(const string &path)
{
	bool includeSheet = false;
	for(Data data(path); data; data.Next())
	{
		if(data.Tag() == "index")
		{
			includeSheet = (static_cast<int>(data[1]) >= 1000);
			Sprite::SetIndex(data);
		}
		else if(data.Tag() == "sheet")
		{
			if(includeSheet)
			{
				string name = data.Value();
				name = name.substr(name.rfind('/') + 1);
				name = name.substr(0, name.rfind('.'));
				sheets.emplace_back(name);
			}
			
			Sprite::LoadSheet(data);
		}
		else if(data.Tag() == "sprite")
		{
			int index = Sprite::Add(data);
			if(includeSheet)
				sheets.back().sprites.emplace_back(index);
		}
		else if(data.Tag() == "style")
			Font::Add(data);
		else if(data.Size())
		{
			// This is an unknown data block. Skip everything in it.
			while(data.Next() && data.Size())
				continue;
		}
	}
}



// Find out how many sprite sheets are available.
int Palette::Sheets() const
{
	return sheets.size();
}



// Change which palette is active.
void Palette::Select(int sheet)
{
	selected = sheet;
}



// Get the index of the selected sheet.
int Palette::Selected() const
{
	return selected;
}



// Get the name of this palette.
const string &Palette::Name() const
{
	return Name(selected);
}



// Get the name of the palette with the given index.
const string &Palette::Name(int sheet) const
{
	return sheets[sheet].name;
}



// Draw the palette at the bottom of the given surface, taking up as much
// vertical space is necessary (but no more than the height of the tallest
// sprite, and no more than the maximum height).
void Palette::Draw(SDL_Surface *surface) const
{
	const Sheet &sheet = sheets[selected];
	
	// Figure out the maximum sprite height and the total palette width.
	int maxHeight = 0;
	int totalWidth = 0;
	for(int i : sheet.sprites)
	{
		const Sprite &sprite = Sprite::Get(i);
		totalWidth += sprite.Width();
		maxHeight = max(maxHeight, sprite.Height());
	}
	
	// The zoom must fit within the given surface and the maximum height, and
	// must be no more than the maximum zoom percent.
	double zoom = min(MAX_ZOOM, min(
		static_cast<double>(surface->w) / totalWidth,
		MAX_HEIGHT / maxHeight));
	
	// Fill in the background of the palette box.
	top = floor(surface->h - maxHeight * zoom);
	Rect back(0, top, surface->w, surface->h - top);
	SDL_FillRect(surface, &back, backColor(surface));
	Rect line(0, top - 1, surface->w, 1);
	SDL_FillRect(surface, &line, lineColor(surface));
	
	// Draw the sprites.
	Point corner(0, surface->h);
	positions.clear();
	for(int i : sheet.sprites)
	{
		const Sprite &sprite = Sprite::Get(i);
		corner += Point(sprite.Draw(surface, corner, zoom), 0);
		positions.push_back(corner.X());
	}
}



// Get the Y position of the top of the most recently drawn palette.
int Palette::Top() const
{
	return top;
}



// Get the slot index of the sprite at the given X position.
int Palette::Slot(int x) const
{
	return upper_bound(positions.begin(), positions.end(), x) - positions.begin();
}



// Get the sprite in the given slot.
int Palette::Index(int slot) const
{
	if(static_cast<size_t>(slot) >= sheets[selected].sprites.size())
		return 0;
	
	return sheets[selected].sprites[slot];
}



// Get the total number of slots.
int Palette::Slots() const
{
	return sheets[selected].sprites.size();
}
