/* Canvas.cpp
Copyright 2020 Michael Zahniser
*/

#include "Canvas.h"

#include <algorithm>

#include <iostream>

using namespace std;

namespace {
	// Cohen-Sutherland line clipping.
	int Code(const Point &p, int width, int height)
	{
		return (p.X() < 0) |
			((p.X() > width) << 1) |
			((p.Y() < 0) << 2) |
			((p.Y() > height) << 3);
	}
	
	void Shift(Point &s, const Point &e, int code, int width, int height)
	{
		// The bit order is: left, right, up, down.
		if(code & 3)
		{
			int x = (code & 1) ? 0 : width;
			s = Point(
				x,
				s.Y() + (e.Y() - s.Y()) * (x - s.X()) / (e.X() - s.X()));
		}
		else
		{
			int y = (code & 4) ? 0 : height;
			s = Point(
				s.X() + (e.X() - s.X()) * (y - s.Y()) / (e.Y() - s.Y()),
				y);
		}
	}
	
	bool Clip(Point &s, Point &e, int width, int height)
	{
		while(true)
		{
			int sc = Code(s, width, height);
			int ec = Code(e, width, height);
			
			// If the points are entirely within the viewport, draw the line.
			if(!(sc | ec))
				return true;
			// If the points are both outside it on the same side, the line does
			// not need to be drawn because it is not visible.
			if(sc & ec)
				return false;
			
			// If either point is outside the clip window, shift it to one of
			// the edges of the window. If a point is outside the window in both
			// x and y, we may need to loop through again to correct the other
			// axis.
			if(sc)
				Shift(s, e, sc, width, height);
			else if(ec)
				Shift(e, s, ec, width, height);
		}
	}
}



// Wrap an SDL surface in a canvas object, locking it for pixel-by-pixel
// editing. The Canvas object must be destroyed to unlock the surface before
// using any other draw calls on it.
Canvas::Canvas(SDL_Surface *surface)
	: surface(surface)
{
	SDL_LockSurface(surface);
}



// Destructor, which unlocks the canvas.
Canvas::~Canvas()
{
	SDL_UnlockSurface(surface);
}



// Set the pen color.
void Canvas::SetColor(Color color)
{
	penColor = color(surface);
}



// Move the "pen" to the given point.
void Canvas::MoveTo(Point point)
{
	source = point;
}



// Draw a line to the given point.
void Canvas::LineTo(Point point)
{
	// Copy the start and end points, then clip the copies to the window.
	Point s = source;
	Point e = point;
	source = point;
	// Clip the line to the view and bail out if it's not visible. The clip
	// function must be given the maximum allowable x and y, which is one less
	// than the width and the height of the surface.
	if(!Clip(s, e, surface->w - 1, surface->h - 1))
		return;
	
	// Get the slope of the line.
	int dx = e.X() - s.X();
	int dy = e.Y() - s.Y();
	
	int pitch = surface->pitch / surface->format->BytesPerPixel;
	int wholePitch = 1;
	int fractionPitch = pitch;
	
	if(dx < 0)
	{
		wholePitch = -wholePitch;
		dx = -dx;
	}
	if(dy < 0)
	{
		fractionPitch = -fractionPitch;
		dy = -dy;
	}
	if(dx < dy)
	{
		swap(wholePitch, fractionPitch);
		swap(dx, dy);
	}
	
	// Start out with the pen logically in the center of the pixel instead of
	// the edge, so a line looks similar when drawn in either direction.
	int step = dy * 2;
	int fraction = dx;
	int whole = dx * 2;
	
	uint32_t *pixels = reinterpret_cast<uint32_t *>(surface->pixels);
	Uint32 *it = pixels + s.X() + s.Y() * pitch;
	Uint32 *end = pixels + e.X() + e.Y() * pitch;
	
	// Bresenham's line-drawing algorithm.
	for( ; it != end; it += wholePitch)
	{
		*it = penColor;
		fraction += step;
		if(fraction >= whole)
		{
			fraction -= whole;
			it += fractionPitch;
		}
	}
	*it = penColor;
}



// Draw a rectangle.
void Canvas::Draw(const Rect &rect)
{
	// Ideally this would be optimized given the knowledge that the lines are
	// axis-aligned, but speed isn't important here.
	MoveTo(Point(rect.x, rect.y));
	LineTo(Point(rect.x + rect.w, rect.y));
	LineTo(Point(rect.x + rect.w, rect.y + rect.h));
	LineTo(Point(rect.x, rect.y + rect.h));
	LineTo(Point(rect.x, rect.y));
}



// Draw a simple polygon.
void Canvas::Draw(const Ring &ring)
{
	MoveTo(ring.back());
	for(const Point &point : ring)
		LineTo(point);
}



// Draw a polygon.
void Canvas::Draw(const Polygon &poly)
{
	for(const Ring &ring : poly)
		Draw(ring);
}
