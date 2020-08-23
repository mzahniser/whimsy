/* Room.cpp
Copyright 2020 Michael Zahniser
*/

#include "Room.h"

#include "Data.h"
#include "Sprite.h"

#include <algorithm>
#include <fstream>

using namespace std;

namespace {
	const Color DEFAULT_BACKGROUND(64, 64, 64);
}



// Read or write a room to a data file.
void Room::Load(Data &data)
{
	Reset();
	name = data.Value();
	while(data.Next())
	{
		// Reading an interaction will advance the data "iterator" to the line
		// after the interaction block. So, we don't want to call Next() after
		// that; instead, go on to processing other room fields.
		while(data.Tag() == "interaction")
			interactions.emplace_back(data);
		if(!data.Size())
			break;
		
		if(data.Tag() == "background")
			background = Color(data[1], data[2], data[3]);
		else
			sprites.emplace_back(data);
	}
}



void Room::Load(const string &path)
{
	Reset();
	for(Data data(path); data; data.Next())
		if(data.Tag() == "room")
			Load(data);
	if(name.empty())
	{
		name = path.substr(path.rfind('/') + 1);
		name = name.substr(0, name.rfind('.'));
	}
}



void Room::Save(const string &path) const
{
	ofstream out(path);
	out << "room " << name << '\n';
	out << "background " << background.r << ' ' << background.g << ' ' << background.b << '\n';
	for(const Entry &entry : sprites)
	{
		out << entry.Index() << ' ' << entry.Center().X() << ',' << entry.Center().Y();
		if(!entry.Name().empty())
			out << ' ' << entry.Name();
		out << '\n';
	}
	for(const Interaction &it : interactions)
		it.Save(out);
}



// Get this room's name.
const string &Room::Name() const
{
	return name;
}



// Add a sprite at the given (x, y), and get the index of the sprite to be
// used if we want to Remove() it.
int Room::Add(int spriteIndex, Point center, const string &name)
{
	// Insert this object after any sprites that it sorts equal to.
	Entry entry(spriteIndex, center, name);
	auto it = upper_bound(sprites.begin(), sprites.end(), entry);
	int index = it - sprites.begin();
	sprites.insert(it, entry);
	return index;
}



// Add an interaction.
void Room::Add(const Interaction &interaction)
{
	interactions.emplace_back(interaction);
}



// Find the top-most sprite that overlaps the given point. If there are no
// overlaps, this returns -1.
int Room::Find(Point center)
{
	for(const Entry &entry : sprites)
		if(entry.Bounds().Contains(center))
			return &entry - &sprites.front();
	
	return -1;
}



// Remove the sprite with the given index.
void Room::Remove(int index)
{
	if(static_cast<size_t>(index) < sprites.size())
		sprites.erase(sprites.begin() + index);
}



// Remove the given interaction.
void Room::Remove(const Interaction *interaction)
{
	interactions.erase(interactions.begin() + (interaction - &interactions.front()));
}



// Remove all sprites and interactions with the give name.
void Room::Remove(const string &name)
{
	// Note: erasing from a vector never invalidates iterators.
	for(vector<Entry>::iterator it = sprites.begin(); it != sprites.end(); )
	{
		if(it->Name() == name)
			sprites.erase(it);
		else
			++it;
	}
	for(vector<Interaction>::iterator it = interactions.begin(); it != interactions.end(); )
	{
		if(it->Name() == name)
			interactions.erase(it);
		else
			++it;
	}
}



// Draw the entire room, in the given surface, with the given (x, y) offset.
void Room::Draw(SDL_Surface *screen, Point offset, Point hover, bool hasFocus) const
{
	// Fill in the background.
	SDL_FillRect(screen, nullptr, background(screen));
	
	// Get the clipping rectangle for the view.
	Rect bounds = Rect(0, 0, screen->w, screen->h) + offset;
	// Draw whatever sprites are within the clipping rectangle.
	for(const Entry &entry : sprites)
		if(entry.Bounds().Overlaps(bounds))
			Sprite::Get(entry.Index()).Draw(screen, entry.Center() - offset);
	
	// Draw interaction icons.
	for(const Interaction &it : interactions)
	{
		// Check if this interaction is currently displaying an icon.
		int index = it.Icon();
		if(!index)
			continue;
		
		// Check if the pointer is hovering over this interaction.
		Point center = it.Position() + it.Offset() - offset;
		if(hasFocus && it.State() == Interaction::ACTIVE && Sprite::Get(index).Bounds().Contains(hover - center))
			index = it.HoverIcon();
		
		// Draw the icon.
		Sprite::Get(index).Draw(screen, center);
	}
}



// Access the raw list of sprites.
const vector<Room::Entry> &Room::Sprites() const
{
	return sprites;
}



vector<Interaction> &Room::Interactions()
{
	return interactions;
}



const vector<Interaction> &Room::Interactions() const
{
	return interactions;
}



// Check if the given point is over an interaction icon. The point should
// be given in world coordinates, not screen coordinates.
const Interaction *Room::Button(Point point) const
{
	const Interaction *match = nullptr;
	for(const Interaction &it : interactions)
		if(it.State() == Interaction::ACTIVE)
		{
			Point center = it.Position() + it.Offset();
			if(Sprite::Get(it.Icon()).Bounds().Contains(point - center))
				match = &it;
		}
	return match;
}



// Constructor.
Room::Entry::Entry(const Data &data)
	: index(data[0]), layer(Sprite::Get(index).Layer()), center(data[1]), name(data.Value(2))
{
}



Room::Entry::Entry(int spriteIndex, Point center, const string &name)
	: index(spriteIndex), layer(Sprite::Get(index).Layer()), center(center), name(name)
{
}



// Comparison operator, defining the sprite ordering.
bool Room::Entry::operator<(const Room::Entry &other) const
{
	return (layer < other.layer || (!layer && !other.layer && center.Y() < other.center.Y()));
}



// Get this sprite's bounding rect in room coordinates.
Rect Room::Entry::Bounds() const
{
	return Sprite::Get(index).Bounds() + center;
}



// Get this sprite's collision mask in room coordinates.
Polygon Room::Entry::Mask() const
{
	return Sprite::Get(index).Mask() + center;
}



int Room::Entry::Index() const
{
	return index;
}



Point Room::Entry::Center() const
{
	return center;
}



const string &Room::Entry::Name() const
{
	return name;
}



// Reset to an "empty" state.
void Room::Reset()
{
	name.clear();
	background = DEFAULT_BACKGROUND;
	sprites.clear();
	interactions.clear();
}
