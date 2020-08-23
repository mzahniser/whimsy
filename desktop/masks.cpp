/* masks.cpp
Copyright 2020 Michael Zahniser

Program to edit the collision masks for a sprite sheet. Assume that the text
file that goes with a sprite sheet contains nothing but sprite definitions, so
we don't have to parse and preserve any other data in it.
*/

#include "Canvas.h"
#include "Color.h"
#include "Data.h"
#include "Point.h"
#include "Polygon.h"
#include "Rect.h"
#include "Ring.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <cmath>
#include <iostream>
#include <vector>

using namespace std;

namespace {
	SDL_Window *window = nullptr;
	SDL_Surface *screen = nullptr;
	SDL_Surface *sheet = nullptr;
	
	// Circular object mask. This is conter-clockwise in screen coordinates, so
	// it defines a "block" object.
	vector<Point> CIRCLE = {
		Point(6, 1), Point(6, -1), Point(2, -3), Point(-2, -3),
		Point(-6, -1), Point(-6, 1), Point(-2, 3), Point(2, 3)};
	
	const int RADIUS_STEP = 1;
	const int AVATAR_RADIUS = 4;
	int radius = AVATAR_RADIUS;
	Point hover;
	
	class Sprite {
	public:
		Rect bounds;
		int baseline = 0;
		int layer = 0;
		Polygon mask;
	};
	vector<Sprite> sprites;
	
	Color background(64, 64, 64);
	Color boundsColor(0, 64, 255);
	Color baseColor(0, 192, 255);
	Color blockColor(255, 0, 0);
	Color allowColor(255, 192, 0);
	Color hoverColor(255, 128, 0);
}

void Load(string path);
void Draw();
Polygon Circle(int radius);



int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		cerr << "Please specify a sprite sheet to load." << endl;
		return 1;
	}
	
	// Initialize SDL first, so we can load the image before creating the window.
	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init(IMG_INIT_PNG);
	
	// Strip the file extension from the path. The text file and the PNG ought
	// to have the same name aside from the extension.
	string path = argv[1];
	path = path.substr(0, path.rfind('.'));
	Load(path);
	
	// Create the SDL window.
	window = SDL_CreateWindow(
		"Mask Editor", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		sheet->w, sheet->h, SDL_WINDOW_SHOWN);
	screen = SDL_GetWindowSurface(window);
	if(!window || !screen)
	{
		cerr << "Unable to intialize SDL." << endl;
		return 1;
	}
	
	// Begin the event loop.
	while(true)
	{
		Draw();
		SDL_UpdateWindowSurface(window);
		
		// Receive events, redrawing the screen only when one is received that
		// changes the map.
		SDL_Event event;
		SDL_WaitEvent(&event);
		// Check where the cursor is right now, and whether the left button is down.
		int x;
		int y;
		SDL_GetMouseState(&x, &y);
		hover = Point(x, y);
		
		if(event.type == SDL_QUIT)
			break;
		else if(event.type == SDL_KEYDOWN)
		{
			SDL_Keycode key = event.key.keysym.sym;
			if(key == 'q')
				break;
			if(key >= '0' && key <= '9')
				radius = RADIUS_STEP * (key - '0') + AVATAR_RADIUS;
		}
		else if(event.type == SDL_MOUSEWHEEL)
			radius = max(AVATAR_RADIUS, radius + RADIUS_STEP * ((event.wheel.y > 0) - (event.wheel.y < 0)));
		else if(event.type == SDL_MOUSEBUTTONDOWN)
		{
			// Check which sprite was clicked.
			for(Sprite &sprite : sprites)
				if(sprite.bounds.Contains(hover))
				{
					sprite.mask = Circle(radius) + hover;
					break;
				}
		}
	}
	
	// Clean up the SDL resources.
	SDL_FreeSurface(sheet);
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	// Write the new sprite definitions to the terminal.
	for(const Sprite &sprite : sprites)
	{
		cout << "sprite" << endl;
		cout << "bounds " << sprite.bounds.x << "," << sprite.bounds.y
			<< " " << sprite.bounds.x + sprite.bounds.w
			<< "," << sprite.bounds.y + sprite.bounds.h << endl;
		if(sprite.layer)
			cout << "layer " << sprite.layer << endl;
		else
			cout << "baseline " << sprite.baseline << endl;
		for(const vector<Point> &part : sprite.mask)
		{
			cout << "mask";
			for(const Point &point : part)
				cout << " " << point.X() << "," << point.Y();
			cout << endl;
		}
		cout << endl;
	}
	
	return 0;
}



void Load(string path)
{
	sheet = IMG_Load((path + ".png").c_str());
	
	for(Data data(path + ".txt"); data; data.Next())
	{
		if(data.Tag() == "sprite")
		{
			sprites.emplace_back();
			Sprite &sprite = sprites.back();
			
			while(data.Next() && data.Size())
			{
				if(data.Tag() == "bounds" && data.Size() == 3)
				{
					Point a = data[1];
					Point b = data[2];
					sprite.bounds = Rect(a, b);
				}
				else if(data.Tag() == "baseline" && data.Size() == 2)
					sprite.baseline = data[1];
				else if(data.Tag() == "layer" && data.Size() == 2)
					sprite.layer = data[1];
				else if(data.Tag() == "mask")
				{
					sprite.mask.emplace_back();
					vector<Point> &part = sprite.mask.back();
					for(size_t i = 1; i < data.Size(); ++i)
						part.push_back(data[i]);
				}
				else
					cerr << "Sprite: error:" << data.Line() << endl;
			}
		}
	}
}



void Draw()
{
	// Fill the background with the default grass color.
	SDL_FillRect(screen, nullptr, background(screen));
	SDL_BlitSurface(sheet, nullptr, screen, nullptr);
	
	Canvas canvas(screen);
	
	// Draw the baselines.
	canvas.SetColor(baseColor);
	for(const Sprite &sprite : sprites)
		if(sprite.layer == 0)
		{
			canvas.MoveTo(Point(sprite.bounds.x, sprite.baseline));
			canvas.LineTo(Point(sprite.bounds.x + sprite.bounds.w, sprite.baseline));
		}
	
	// Draw the bounds.
	canvas.SetColor(boundsColor);
	for(const Sprite &sprite : sprites)
		canvas.Draw(sprite.bounds);
	
	// Draw the masks that have been set.
	for(const Sprite &sprite : sprites)
		for(const Ring &ring : sprite.mask)
		{
			canvas.SetColor(ring.IsHole() ? blockColor : allowColor);
			canvas.Draw(ring);
		}
	
	// Draw the "hovering" mask.
	Polygon mask = Circle(radius) + hover;
	canvas.SetColor(hoverColor);
	canvas.Draw(mask);
	// Draw guidelines showing how far the edge of the mask should be from the
	// edge of the object to account for the avatar's radius.
	int rx = CIRCLE.front().X() * (radius - AVATAR_RADIUS);
	canvas.MoveTo(hover + Point(-rx, -5));
	canvas.LineTo(hover + Point(-rx, 5));
	canvas.MoveTo(hover + Point(rx, -5));
	canvas.LineTo(hover + Point(rx, 5));
}



Polygon Circle(int radius)
{
	Polygon circle;
	circle.emplace_back();
	vector<Point> &part = circle.back();
	
	for(const Point &point : CIRCLE)
		part.emplace_back(point * radius);
	
	return circle;
}
