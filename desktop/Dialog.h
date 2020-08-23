/* Dialog.h
Copyright 2020 Michael Zahniser
*/

#ifndef DIALOG_H_
#define DIALOG_H_

#include "Data.h"
#include "Point.h"
#include "Rect.h"

#include <SDL2/SDL.h>

#include <ostream>
#include <set>
#include <string>
#include <vector>

class World;

using namespace std;



class Dialog {
public:
	// Load a dialog definition from the given data file.
	static void Load(Data &data);
	
	
public:
	Dialog(World &world);
	
	// Begin the given dialog.
	void Begin(const string &name);
	void Begin(const vector<string> &changes);
	// Write the dialog's current state to a saved game file.
	void Save(ostream &out) const;
	// Reset the dialog state, e.g. to begin a new game.
	void Close();
	
	// Check if the dialog needs to be drawn.
	bool IsOpen() const;
	
	// Draw the dialog. If the given mouse position is on one of the options,
	// highlight it.
	void Draw(SDL_Surface *screen, Point hover) const;
	// Handle an event, and return true if the screen needs to be redrawn.
	bool Handle(const SDL_Event &event);
	
	
private:
	// Select the given option, advancing the dialog to the next page.
	void Choose(int option);
	// If the dialog is just waiting for acknowledgment, instead of for a
	// specific choice, go on to the next page.
	void Acknowledge();
	
	// Step forward to the next "say" block.
	void Step();
	void ClearOptions();
	// Check if the given point is inside one of the options. If not, this
	// returns the size of the options vector.
	size_t Button(Point p);
	
	
private:
	// Reference to the world, so events in this dialog can modify it.
	World &world;
	
	// The dialog data that we're "iterating" through right now.
	Data data;
	
	// Things we show to the player.
	string text;
	int icon = 0;
	int scene = 0;
	vector<string> optionText;
	
	// Information stored behind the scenes.
	vector<string> options;
	mutable vector<Rect> optionRects;
	string exitText;
	set<string> visited;
};


#endif
