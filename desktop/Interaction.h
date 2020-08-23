/* Interaction.h
Copyright 2020 Michael Zahniser
*/

#ifndef INTERACTION_H_
#define INTERACTION_H_

#include "Data.h"
#include "Point.h"

#include <ostream>
#include <string>

using namespace std;



class Interaction {
public:
	Interaction() = default;
	Interaction(Data &data);
	
	// Load an interaction definition. Usually, this will be nested inside a
	// room definition.
	void Load(Data &data);
	// Write this interaction to the given output stream.
	void Save(ostream &out, const string &indent = "") const;
	
	// Get the name of this interaction. Names are optional; they are needed
	// only for interactions that may be removed via dialog events.
	const string &Name() const;
	// Set this interaction's name.
	void SetName(const string &name);
	
	// Note: the functions below use the following "state" definitions:
	static const int INACTIVE = -1;
	static const int VISIBLE = 0;
	static const int ACTIVE = 1;
	static const int HOVER = 2;
	static const int IMMEDIATE = 3;
	
	// Set the state of this interaction given the avatar's position.
	int SetState(Point avatar);
	// Manually set this interaction's state.
	void SetState(int state);
	// Clear the state of this interaction. This must be done when leaving the
	// room to reset any active immediate interactions.
	void ClearState();
	// Get the most recently set state.
	int State() const;
	// Get the icon for this interaction given whatever state is set.
	int Icon() const;
	// Get the icon to be used when the active icon is hovered.
	int HoverIcon() const;
	
	// Get the "floor" position of this interaction.
	Point Position() const;
	// Get the offset of the icon relative to the sprite's floor position.
	Point Offset() const;
	// Set the interaction's position and offset.
	void Place(Point position, Point offset = Point());
	
	// Check if this interaction causes an "enter" event.
	bool HasEnter() const;
	const string &EnterRoom() const;
	Point EnterPosition() const;
	
	// Check if this interaction causes a dialog to run.
	bool HasDialog() const;
	const string &DialogName() const;
	
	// Get the radius for the given state.
	const Point &Radius(int state) const;
	
	
private:
	string name;
	
	Point position;
	Point offset;
	Point radius[2];
	
	int state = -1;
	int icon[3] = {0, 0, 0};
	
	bool hasEnter = false;
	Point enterPosition;
	string enterRoom;
	string dialog;
};



#endif
