/* Canvas.h
Copyright 2020 Michael Zahniser
*/

#ifndef CANVAS_H_
#define CANVAS_H_

#include "Color.h"
#include "Point.h"
#include "Polygon.h"
#include "Rect.h"
#include "Ring.h"

#include <SDL2/SDL.h>

using namespace std;



class Canvas {
public:
	// Wrap an SDL surface in a canvas object, locking it for pixel-by-pixel
	// editing. The Canvas object must be destroyed to unlock the surface before
	// using any other draw calls on it.
	Canvas(SDL_Surface *surface);
	// Destructor, which unlocks the canvas.
	~Canvas();
	
	// Set the pen color.
	void SetColor(Color color);
	// Move the "pen" to the given point.
	void MoveTo(Point point);
	// Draw a line to the given point.
	void LineTo(Point point);
	
	// Draw a rectangle.
	void Draw(const Rect &rect);
	// Draw a simple polygon.
	void Draw(const Ring &ring);
	// Draw a polygon.
	void Draw(const Polygon &poly);
	
	
private:
	SDL_Surface *surface = nullptr;
	uint32_t penColor = 0;
	Point source;
};



#endif
