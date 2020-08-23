/* Rect.h
Copyright 2019 Michael Zahniser
*/

#ifndef RECT_H_
#define RECT_H_

#include "Point.h"

#include <SDL2/SDL.h>

using namespace std;



// This class is just an SDL_Rect wrapper that adds a constructor.
class Rect : public SDL_Rect {
public:
	Rect(int x = 0, int y = 0, int w = 0, int h = 0);
	Rect(Point a, Point b = Point(0, 0));
	
	// Functions to shift a rectangle by a given vector.
	Rect operator+(Point point) const;
	Rect &operator+=(Point point);
	Rect operator-(Point point) const;
	Rect &operator-=(Point point);
	
	// Check if a rectangle contains a point or overlaps a rectangle.
	bool Contains(Point point) const;
	bool Overlaps(const Rect &rect) const;
	
	// Grow or shrink the rectangle in all directions by the given amount.
	void Grow(int distance);
	// Convert the corner and dimensions to points.
	Point TopLeft() const;
	Point Size() const;
};



#endif
