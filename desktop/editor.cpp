/* editor.cpp
Copyright 2020 Michael Zahniser

Room editor for Whimsy.
*/

#include "Canvas.h"
#include "Color.h"
#include "Data.h"
#include "Font.h"
#include "Interaction.h"
#include "Palette.h"
#include "Point.h"
#include "Polygon.h"
#include "Rect.h"
#include "Ring.h"
#include "Room.h"
#include "Sprite.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <cmath>
#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

namespace {
	SDL_Window *window = nullptr;
	SDL_Surface *screen = nullptr;
	
	Room room;
	Polygon passable;
	
	const int SCROLL = 400;
	Point scroll;
	
	bool isHovering = false;
	Point hover;
	
	bool showMask = true;
	
	int slot = 0;
	int selected = 0;
	Palette palette;
	
	const Interaction *interaction = nullptr;
	bool isPlacingInteraction = false;
	
	const int INTERNAL_SCALE = 4;
	
	const Color background(64, 64, 64);
	const Color maskColor(255, 0, 0);
	const Color newColor(255, 128, 0);
	const Color lineColor(0);
	const Color backColor(200);
	const Color selectedColor(180);
	
	const Color radiusColor[2] = {Color(0, 200, 255), Color(0, 60, 255)};
	
	const int LIST_WIDTH = 100;
	const int LINE_HEIGHT = 20;
	const int INTERACTION_WIDTH = 200;
	vector<Interaction> interactions;
	
	enum ScreenZone {MAIN, PALETTE_LIST, INTERACTION_LIST, PALETTE};
}

// Check if the given file exists and is readable.
bool FileExists(const string &path);

// Figure out what zone of the screen the given (x, y) point is in.
ScreenZone Zone(int x, int y);
// Update where the mouse is hovering. Returns true if the tile changed.
bool Hover(int x, int y);
// Update the collision mask.
void UpdateMask();

// Draw the entire screen.
void Draw();
// Draw the sprites (and the background).
void DrawSprites();
// Draw the currently selected palette.
void DrawPalette();
// Draw the interactions list.
void DrawInteractions();



int main(int argc, char *argv[])
{
	// Create the SDL window.
	SDL_Init(SDL_INIT_VIDEO);
	IMG_Init(IMG_INIT_PNG);
	window = SDL_CreateWindow(
		"Whimsy Editor", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		1200, 640, SDL_WINDOW_SHOWN);
	screen = SDL_GetWindowSurface(window);
	if(!window || !screen)
	{
		cerr << "Unable to intialize SDL." << endl;
		return 1;
	}
	
	// Starting with the room file we're editing, assume that there is a
	// "data.txt" file defining the sprites, etc. either in the same directory
	// or in one of its parent directories. Find that directory.
	string roomPath = (argc >= 2 ? argv[1] : "room.txt");
	// Convert backward slashes to forward slashes, if on windows.
#ifdef _WIN32
	for(char &c : roomPath)
		if(c == '\\')
			c = '/';
#endif
	string directory = roomPath.substr(0, roomPath.rfind('/') + 1);
	// Only look three directories up.
	for(int i = 0; i < 3 && !FileExists(directory + "data.txt"); i++)
		directory += "../";
	
	// Set the location of the font images. Assume the "fonts" directory is in
	// the same place as the "data.txt" file.
	Font::SetDirectory(directory + "fonts/");
	// Load the data file.
	palette.Load(directory + "data.txt");
	if(!Font::IsLoaded())
	{
		cerr << "Unable to load the font." << endl;
		return 1;
	}
	// Load the interaction prototypes.
	for(Data data(directory + "interactions.txt"); data; data.Next())
	{
		if(data.Tag() == "interaction")
		{
			interactions.emplace_back(data);
			interactions.back().SetState(Interaction::ACTIVE);
		}
	}
	
	// Bail out if we didn't load any palettes.
	bool done = !palette.Sheets();
	
	room.Load(roomPath);
	UpdateMask();
	// Display the active icon for all interactions.
	for(Interaction &interaction : room.Interactions())
		interaction.SetState(Interaction::ACTIVE);
	
	while(!done)
	{
		Draw();
		
		// Receive events, redrawing the screen only when one is received that
		// changes the map.
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			// Check where the cursor is right now, and whether the left button is down.
			int x;
			int y;
			uint32_t state = SDL_GetMouseState(&x, &y);
			
			if(event.type == SDL_QUIT)
			{
				done = true;
				break;
			}
			else if(event.type == SDL_KEYDOWN)
			{
				SDL_Keycode key = event.key.keysym.sym;
				if(key == 'q')
				{
					done = true;
					break;
				}
				
				int dx = (key == SDLK_RIGHT) - (key == SDLK_LEFT);
				int dy = (key == SDLK_DOWN) - (key == SDLK_UP);
				if(dx || dy)
				{
					Point d(SCROLL * dx, SCROLL * dy);
					scroll += d;
					passable -= d;
				}
				else if(key == SDLK_PAGEUP || key == SDLK_PAGEDOWN)
				{
					int d = (key == SDLK_PAGEDOWN ? 1 : palette.Sheets() - 1);
					palette.Select((palette.Selected() + d) % palette.Sheets());
				}
				else if(key == SDLK_SPACE)
					showMask = !showMask;
			}
			else if(event.type == SDL_MOUSEWHEEL && interaction)
			{
				if(event.wheel.y > 0 && interaction != &interactions.front())
					--interaction;
				else if(event.wheel.y < 0 && interaction != &interactions.back())
					++interaction;
			}
			else if(event.type == SDL_MOUSEWHEEL)
			{
				slot += (event.wheel.y < 0) - (event.wheel.y > 0);
				slot = max(0, min(palette.Slots() - 1, slot));
				selected = palette.Index(slot);
			}
			else if(event.type == SDL_MOUSEBUTTONDOWN)
			{
				ScreenZone zone = Zone(x, y);
				if(zone == PALETTE)
				{
					slot = palette.Slot(x);
					selected = palette.Index(slot);
					interaction = nullptr;
				}
				else if(zone == PALETTE_LIST)
					palette.Select(y / LINE_HEIGHT);
				else if(zone == INTERACTION_LIST)
				{
					slot = 0;
					selected = 0;
					interaction = &interactions[y / LINE_HEIGHT];
				}
				else if(zone == MAIN && selected)
				{
					room.Add(selected, Point(x, y) + scroll);
					if(!Sprite::Get(selected).Mask().empty())
						UpdateMask();
				}
				else if(zone == MAIN && interaction)
				{
					room.Add(*interaction);
					room.Interactions().back().Place(hover);
					isPlacingInteraction = true;
				}
			}
			else if(event.type == SDL_MOUSEBUTTONUP)
				isPlacingInteraction = false;
			else if(event.type == SDL_MOUSEMOTION)
			{
				// Check whether a hovering sprite needs to be drawn.
				isHovering = !state && Hover(x, y);
				if(isPlacingInteraction)
				{
					Interaction &it = room.Interactions().back();
					Point position = Point(x, y) + scroll;
					it.Place(position, hover - position);
				}
			}
		}
	}
	
	// Save the room.
	room.Save(roomPath);
	
	// Free the sprites.
	Sprite::FreeAll();
	
	// Clean up the SDL resources.
	SDL_DestroyWindow(window);
	SDL_Quit();
	
	return 0;
}



bool FileExists(const string &path)
{
	return ifstream(path).good();
}



// Figure out what zone of the screen the given (x, y) point is in.
ScreenZone Zone(int x, int y)
{
	if(x <= LIST_WIDTH && y <= LINE_HEIGHT * palette.Sheets())
		return PALETTE_LIST;
	if(x >= screen->w - LIST_WIDTH && y <= static_cast<int>(LINE_HEIGHT * interactions.size()))
		return INTERACTION_LIST;
	if(y >= palette.Top())
		return PALETTE;
	return MAIN;
}



// Update where the mouse is hovering. Returns true if the tile changed.
bool Hover(int x, int y)
{
	if(Zone(x, y) != MAIN)
		return false;
	
	hover = Point(x, y) + scroll;
	
	return true;
}



// Update the collision mask.
void UpdateMask()
{
	passable.clear();
	for(const Room::Entry &entry : room.Sprites())
	{
		Polygon mask = entry.Mask() * INTERNAL_SCALE;
		for(const Ring &ring : mask)
			passable.Add(ring);
	}
	passable /= INTERNAL_SCALE;
	passable -= scroll;
}



// Draw the entire screen.
void Draw()
{
	DrawSprites();
	DrawPalette();
	DrawInteractions();
	
	SDL_UpdateWindowSurface(window);
}



// Draw the sprites (and the background).
void DrawSprites()
{
	// Fill the background with the default grass color.
	SDL_FillRect(screen, nullptr, background(screen));
	
	int index = 0;
	if(selected && isHovering)
		index = room.Add(selected, hover);
	room.Draw(screen, scroll, hover, false);
	if(selected && isHovering)
		room.Remove(index);
	
	if(interaction && isHovering)
	{
		int index = interaction->Icon();
		if(index)
			Sprite::Get(index).Draw(screen, hover - scroll);
	}
	
	// Generate the collision mask for the visible sprites.
	if(showMask)
	{
		Canvas canvas(screen);
		canvas.SetColor(maskColor);
		canvas.Draw(passable);
		
		if(isHovering)
		{
			canvas.SetColor(newColor);
			Polygon mask = Sprite::Get(selected).Mask();
			mask += hover - scroll;
			canvas.Draw(mask);
		}
		if(isPlacingInteraction)
		{
			const Interaction &it = room.Interactions().back();
			for(int state = Interaction::VISIBLE; state <= Interaction::ACTIVE; ++state)
			{
				Point radius = it.Radius(state);
				
				Ring ring;
				for(int degrees = 0; degrees < 360; degrees += 30)
				{
					double radians = degrees * M_PI / 180.;
					ring.emplace_back(
						it.Position().X() + round(cos(radians) * radius.X()),
						it.Position().Y() + round(sin(radians) * radius.Y()));
				}
				canvas.SetColor(radiusColor[state]);
				canvas.Draw(ring);
			}
		}
	}
}



// Draw the currently selected palette.
void DrawPalette()
{
	palette.Draw(screen);
	
	Rect line(0, 0, LIST_WIDTH + 1, LINE_HEIGHT * palette.Sheets() + 1);
	SDL_FillRect(screen, &line, lineColor(screen));
	Rect back(0, 0, LIST_WIDTH, LINE_HEIGHT * palette.Sheets());
	SDL_FillRect(screen, &back, backColor(screen));
	
	const Font &font = Font::Get();
	Point corner(5, 2);
	for(int i = 0; i < palette.Sheets(); ++i)
	{
		if(i == palette.Selected())
		{
			Rect fill(0, i * LINE_HEIGHT, LIST_WIDTH, LINE_HEIGHT);
			SDL_FillRect(screen, &fill, selectedColor(screen));
		}
		font.Draw(palette.Name(i), corner, screen);
		corner.Y() += LINE_HEIGHT;
	}
	
	// Also draw the pointer coordinates.
	corner.Y() += LINE_HEIGHT;
	line = Rect(
		0, LINE_HEIGHT * (palette.Sheets() + 1) - 1,
		LIST_WIDTH + 1, LINE_HEIGHT + 2);
	SDL_FillRect(screen, &line, lineColor(screen));
	back = Rect(
		0, LINE_HEIGHT * (palette.Sheets() + 1),
		LIST_WIDTH, LINE_HEIGHT);
	SDL_FillRect(screen, &back, backColor(screen));
	
	string coords = "(" + to_string(hover.X()) + ", " + to_string(hover.Y()) + ")";
	font.Draw(coords, corner, screen);
}



// Draw the interactions list.
void DrawInteractions()
{
	Rect line(screen->w - LIST_WIDTH - 1, 0, screen->w, LINE_HEIGHT * interactions.size() + 1);
	SDL_FillRect(screen, &line, lineColor(screen));
	Rect back(screen->w - LIST_WIDTH, 0, screen->w, LINE_HEIGHT * interactions.size());
	SDL_FillRect(screen, &back, backColor(screen));
	
	const Font &font = Font::Get();
	Point corner(screen->w - LIST_WIDTH + 5, 2);
	for(size_t i = 0; i < interactions.size(); ++i)
	{
		if(interaction == &interactions[i])
		{
			Rect fill(screen->w - LIST_WIDTH, i * LINE_HEIGHT, LIST_WIDTH, LINE_HEIGHT);
			SDL_FillRect(screen, &fill, selectedColor(screen));
		}
		font.Draw(interactions[i].Name(), corner, screen);
		corner.Y() += LINE_HEIGHT;
	}
}
