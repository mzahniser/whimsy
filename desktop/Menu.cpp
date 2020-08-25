/* Menu.cpp
Copyright 2020 Michael Zahniser
*/

#include "Menu.h"

#include "Font.h"
#include "Sprite.h"

#include <map>

using namespace std;

namespace {
	map<string, Menu> menus;
}



// Load a menu definition from the given data file.
void Menu::Add(Data &data)
{
	// Keep track of the currently specified style.
	string style;
	Menu &menu = menus[data.Value()];
	// Remember the name of this menu.
	menu.name = data.Value();
	while(data.Next() && data.Size())
	{
		if(data.Tag() == "background")
			menu.background = Color(data[1], data[2], data[3]);
		else if(data.Tag() == "style")
			style = data.Value();
		else
			menu.items.emplace_back(data, style);
	}
	// Remember whether this menu has buttons.
	for(const Item &item : menu.items)
		menu.hasButtons |= !item.button.empty();
}



// Get the menu with the given name.
const Menu *Menu::Get(const string &name)
{
	map<string, Menu>::const_iterator it = menus.find(name);
	return (it == menus.end() ? nullptr : &it->second);
}



// Draw a menu, with the pointer hovering at the given position.
void Menu::Draw(SDL_Surface *screen, Point hover, bool loaded) const
{
	// Menu coordinates are relative to the center of the screen, so that the
	// menu will be centered in large screens.
	center = Point(screen->w / 2, screen->h / 2);
	SDL_FillRect(screen, nullptr, background(screen));
	for(const Item &item : items)
	{
		Point pos = center + item.center;
		if(!item.text.empty())
		{
			const Font &font = Font::Get(item.style);
			font.Draw(item.text, pos, screen);
		}
		else
		{
			Rect bounds = Sprite::Get(item.sprites[0]).Bounds() + pos;
			int index = 2 * !loaded + bounds.Contains(hover);
			Sprite::Get(item.sprites[index]).Draw(screen, pos);
		}
	}
}



// Handle the given event, returning a string indicating what to do:
// An empty string means do nothing (don't even redraw the screen).
// "new" means start a new game.
// "continue" means close the menu.
// "quit" means quit the game.
// Any other string means to switch to the menu with that name.
const string &Menu::Handle(const SDL_Event &event) const
{
	static const string MAIN_MENU = "main";
	static const string CONTINUE = "continue";
	static const string IGNORE = "";
	
	if(event.type == SDL_MOUSEBUTTONDOWN)
	{
		Point point(event.button.x, event.button.y);
		// If this menu has no buttons, any click returns us to the main menu.
		if(!hasButtons)
			return MAIN_MENU;
		const Item *clicked = Button(point);
		if(clicked)
			return clicked->button;
	}
	else if(event.type == SDL_MOUSEMOTION && hasButtons)
	{
		Point point(event.motion.x, event.motion.y);
		Point previous = point - Point(event.motion.xrel, event.motion.yrel);
		if(Button(point) != Button(previous))
			return name;
	}
	else if(event.type == SDL_KEYDOWN)
	{
		SDL_Keycode sym = event.key.keysym.sym;
		if(sym == SDLK_SPACE || sym == SDLK_RETURN)
		{
			// If this menu has no buttons, the same buttons that acknowledge a
			// dialog page also work to return to the main menu.
			if(!hasButtons)
				return MAIN_MENU;
		}
		else if(sym == SDLK_ESCAPE)
		{
			// Escape in a sub-menu returns to the main menu.
			if(name != MAIN_MENU)
				return MAIN_MENU;
			else
				return CONTINUE;
		}
	}
	return IGNORE;
}



// Check if the given point is inside a button. If so, return it.
const Menu::Item *Menu::Button(Point point) const
{
	// Translate from screen coordinates to menu coordinates.
	point -= center;
	// Check if the given point is inside of any of the button sprites.
	for(const Item &item : items)
		if(!item.button.empty())
		{
			// Assume all four sprites have the same bounding box.
			Rect bounds = Sprite::Get(item.sprites[0]).Bounds() + item.center;
			if(bounds.Contains(point))
				return &item;
		}
	return nullptr;
}



Menu::Item::Item(const Data &data, const string &style)
	: style(style)
{
	center = data[0];
	// Assume that there are at least two arguments. Check if the first one is
	// an integer. If not, this is a text string.
	if(!data[1].IsInt())
	{
		text = data.Value();
		return;
	}
	
	// Read a sequence of integers giving the sprites for various button states.
	size_t i = 1;
	for( ; i < data.Size() && i <= 4 && data[i].IsInt(); ++i)
		sprites[i - 1] = data[i];
	// Fill in undefined sprites. If the hover sprite was not defined:
	if(i == 2 || i == 4)
		sprites[i - 1] = sprites[i - 2];
	// If the sprites for no game loaded were not defined:
	if(i <= 3)
	{
		sprites[2] = sprites[0];
		sprites[3] = sprites[1];
	}
	
	// The rest of the line starting here is the menu name.
	button = data.Value(i);
}
