/* Interaction.cpp
Copyright 2020 Michael Zahniser
*/

#include "Interaction.h"

#include <cstdint>

using namespace std;

namespace {
	// Check if the given point is within the given radius of the origin.
	// Note: if the radius is (0, 0), this will always return true.
	bool InRange(Point point, Point radius)
	{
		// That is, it's within a circle defined by:
		// (point.x / radius.x)^2 + (point.y / radius.y)^2 < 1
		// (point.x * radius.y)^2 + (point.y * radius.x)^2 < (radius.x * radius.y)^2
		// Use 64-bit math to avoid overflow.
		int64_t px = point.X() * radius.Y();
		int64_t py = point.Y() * radius.X();
		int64_t r = radius.X() * radius.Y();
		return (px * px + py * py <= r * r);
	}
};



// Constructor, to allow an Interaction to be emplaced.
Interaction::Interaction(Data &data)
{
	Load(data);
}



// Load an interaction definition. Usually, this will be nested inside a
// room definition.
void Interaction::Load(Data &data)
{
	name = data.Value();
	while(data.Next() && data.Size())
	{
		if(data.Tag() == "position")
			position = data[1];
		else if(data.Tag() == "offset")
			offset = data[1];
		else if(data.Tag() == "visible")
		{
			radius[VISIBLE] = data[1];
			icon[VISIBLE] = data[2];
		}
		else if(data.Tag() == "active")
		{
			radius[ACTIVE] = data[1];
			icon[ACTIVE] = data[2];
			icon[HOVER] = data[3];
			// If no hover icon is provided, use the active icon.
			if(!icon[HOVER])
				icon[HOVER] = icon[ACTIVE];
		}
		else if(data.Tag() == "enter")
		{
			hasEnter = true;
			enterPosition = data[1];
			enterRoom = data.Value(2);
		}
		else if(data.Tag() == "dialog")
			dialog = data.Value();
		// If the tag is none of the above, we have reached the end of this
		// interaction definition. The next interaction, or some other room
		// data, may begin immediately without a blank line.
		else
			break;
	}
}



// Get the name of this interaction. Names are optional; they are needed
// only for interactions that may be removed via dialog events.
const string &Interaction::Name() const
{
	return name;
}



// Set this interaction's name.
void Interaction::SetName(const string &name)
{
	this->name = name;
}



// Write this interaction to the given output stream.
void Interaction::Save(ostream &out, const string &indent) const
{
	out << indent << "interaction";
	if(!name.empty())
		out << ' ' << name;
	out << '\n';
	// Every interaction ought to have a position; everything else is optional.
	out << indent << "  position " << position.X() << ',' << position.Y() << '\n';
	if(offset)
		out << indent << "  offset " << offset.X() << ',' << offset.Y() << '\n';
	if(icon[VISIBLE])
		out << indent << "  visible " << radius[VISIBLE].X() << ',' << radius[VISIBLE].Y() << ' ' << icon[VISIBLE] << '\n';
	if(radius[ACTIVE] || icon[ACTIVE])
	{
		out << indent << "  active " << radius[ACTIVE].X() << ',' << radius[ACTIVE].Y();
		if(icon[ACTIVE])
			out << ' ' << icon[ACTIVE];
		if(icon[HOVER])
			out << ' ' << icon[HOVER];
		out << '\n';
	}
	if(hasEnter)
	{
		out << indent << "  enter " << enterPosition.X() << ',' << enterPosition.Y();
		if(!enterRoom.empty())
			out << ' ' << enterRoom;
		out << '\n';
	}
	if(!dialog.empty())
		out << indent << "  dialog " << dialog << '\n';
}



// Get the state of this interaction given the avatar's position.
int Interaction::SetState(Point avatar)
{
	// If no active radius is specified, the interaction activates whenever the
	// avatar enters this room. If no active icon is specified, the interaction
	// triggers immediately (but does not trigger again until it ceases to be 
	// active and then becomes active again).
	avatar -= position;
	if(InRange(avatar, radius[ACTIVE]))
	{
		if(state != ACTIVE)
		{
			state = ACTIVE;
			if(!icon[ACTIVE])
				return IMMEDIATE;
		}
	}
	else if(icon[VISIBLE] && InRange(avatar, radius[VISIBLE]))
		state = VISIBLE;
	else
		state = INACTIVE;
	
	return state;
}



// Manually set this interaction's state.
void Interaction::SetState(int state)
{
	this->state = state;
}



// Clear the state of this interaction. This must be done when leaving the
// room to reset any active immediate interactions.
void Interaction::ClearState()
{
	state = INACTIVE;
}



// Get the most recently set state.
int Interaction::State() const
{
	return state;
}



// Get the icon for this interaction given whatever state is set.
int Interaction::Icon() const
{
	// Check if the icon should be shown.
	if(state != VISIBLE && state != ACTIVE)
		return 0;
	
	return icon[state];
}



// Get the icon to be used when the active icon is hovered.
int Interaction::HoverIcon() const
{
	return icon[HOVER];
}



// Get the "floor" position of this interaction.
Point Interaction::Position() const
{
	return position;
}



// Get the offset of the icon relative to the sprite's floor position.
Point Interaction::Offset() const
{
	return offset;
}



// Set the interaction's position and offset.
void Interaction::Place(Point position, Point offset)
{
	this->position = position;
	this->offset = offset;
}



// Check if this interaction causes an "enter" event.
bool Interaction::HasEnter() const
{
	return hasEnter;
}



const string &Interaction::EnterRoom() const
{
	return enterRoom;
}



Point Interaction::EnterPosition() const
{
	return enterPosition;
}



// Check if this interaction causes a dialog to run.
bool Interaction::HasDialog() const
{
	return !dialog.empty();
}



const string &Interaction::DialogName() const
{
	return dialog;
}


	
// Get the radius for the given state.
const Point &Interaction::Radius(int state) const
{
	return radius[state];
}
