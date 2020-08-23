/* Menu.h
Copyright 2020 Michael Zahniser
*/

#ifndef MENU_H_
#define MENU_H_

#include "Color.h"
#include "Data.h"

#include <SDL2/SDL.h>

#include <string>

using namespace std;



class Menu {
public:
	// Load a menu definition from the given data file.
	static void Add(Data &data);
	// Get the menu with the given name. If there is no menu with that name,
	// this returns null, which is a signal to break out of the menu.
	static const Menu *Get(const string &name);
	
	
public:
	// Draw a menu, with the pointer hovering at the given position.
	void Draw(SDL_Surface *screen, Point hover, bool loaded) const;
	// Handle the given event, returning a string indicating what to do:
	// an empty string means do nothing (don't even redraw the screen).
	// "new" means start a new game.
	// "continue" means close the menu.
	// "quit" means quit the game.
	// Any other string including empty means switch to the menu with that name.
	const string &Handle(const SDL_Event &event) const;
	
	
private:
	class Item {
	public:
		Item() = default;
		// Read an item from a single line of a data file.
		Item(const Data &data, const string &style);
		
		Point center;
		int sprites[4] = {0, 0, 0, 0};
		string text;
		string button;
		string style;
	};
	// Check if the given point is inside a button. If so, return it.
	const Item *Button(Point point) const;
	
	
private:
	vector<Item> items;
	Color background;
	string name;
	bool hasButtons = false;
	// Remember the coordinates of the center of the screen.
	mutable Point center;
};

#endif // MENU_H_
