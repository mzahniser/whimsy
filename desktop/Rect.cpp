/* Rect.cpp
Copyright 2020 Michael Zahniser
*/

#include "Rect.h"

#include <algorithm>

using namespace std;



Rect::Rect(int x, int y, int w, int h)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
}



Rect::Rect(Point a, Point b)
	: Rect(a.X(), a.Y(), b.X() - a.X(), b.Y() - a.Y())
{
}



// Functions to shift a rectangle by a given vector.
Rect Rect::operator+(Point point) const
{
	return Rect(x + point.X(), y + point.Y(), w, h);
}



Rect &Rect::operator+=(Point point)
{
	x += point.X();
	y += point.Y();
	return *this;
}



Rect Rect::operator-(Point point) const
{
	return Rect(x - point.X(), y - point.Y(), w, h);
}



Rect &Rect::operator-=(Point point)
{
	x -= point.X();
	y -= point.Y();
	return *this;
}



// Check if a rectangle contains a point or overlaps a rectangle.
bool Rect::Contains(Point point) const
{
	int offX = point.X() - x;
	int offY = point.Y() - y;
	return (offX >= 0 && offX < w && offY >= 0 && offY < h);
}



bool Rect::Overlaps(const Rect &rect) const
{
	return max(x, rect.x) < min(x + w, rect.x + rect.w)
		&& max(y, rect.y) < min(y + h, rect.y + rect.h);
}

// Grow or shrink the rectangle in all directions by the given amount.
void Rect::Grow(int distance)
{
	x -= distance;
	y -= distance;
	w += 2 * distance;
	h += 2 * distance;
}



// Convert the corners to points.
Point Rect::TopLeft() const
{
	return Point(x, y);
}



Point Rect::Size() const
{
	return Point(w, h);
}
