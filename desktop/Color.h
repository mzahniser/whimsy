/* Color.h
Copyright 2019 Michael Zahniser
*/

#ifndef COLOR_H_
#define COLOR_H_

#include <SDL2/SDL.h>

#include <cstdint>

using namespace std;



class Color {
public:
	Color(int v = 0) : r(v), g(v), b(v) {}
	Color(int r, int g, int b) : r(r), g(g), b(b) {}
	
	uint32_t operator()(const SDL_Surface *surface) const
	{
		return SDL_MapRGB(surface->format, r, g, b);
	}
	
	int r;
	int g;
	int b;
};



#endif
