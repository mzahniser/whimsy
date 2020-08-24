/* whimsy.cpp
Copyright 2020 Michael Zahniser

C++ engine for "Whimsy" games.
*/

#include "Data.h"
#include "Font.h"
#include "Menu.h"
#include "Point.h"
#include "Sprite.h"
#include "World.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <fstream>
#include <iostream>
#include <string>

using namespace std;

namespace {
	// The SDL window.
	const Point MIN_WINDOW_SIZE(640, 500);
	SDL_Window *window = nullptr;
	SDL_Surface *screen = nullptr;
	
	// Persistent preferences options:
	string preferencesPath;
	Point windowSize = MIN_WINDOW_SIZE;
	bool fullscreen = false;
	
	// Frame rate control.
	SDL_TimerID frameTimer = 0;
	
	// The two main UI layers: the menu, and the world view (which includes a
	// dialog overlay).
	const Menu *menu = nullptr;
	World world;
}

// Handle events, and return true unless it's time to quit.
bool HandleEvents();
uint32_t TimerFunction(uint32_t interval, void *);
bool Init(char *argv[]);
void Free();
void ReadPreferences();
void SavePreferences();



int main(int argc, char *argv[])
{
	if(!Init(argv))
	{
		Free();
		return 1;
	}
	
	while(true)
	{
		// The frame timer has to control the time between draw events. If the
		// timer restarted as soon as it elapsed, when the game has been idle
		// waiting for input it will jump through two frames immediately as soon
		// as input is available.
		int x, y;
		SDL_GetMouseState(&x, &y);
		Point hover(x, y);
		
		if(menu)
			menu->Draw(screen, hover, world);
		else
			world.Draw(screen, hover);
		SDL_UpdateWindowSurface(window);
		
		if(!HandleEvents())
			break;
	}
	
	world.Save();
	SavePreferences();
	Free();
	
	return 0;
}



bool HandleEvents()
{
	// Keep track of whether the screen needs updating.
	bool mustRedraw = false;
	while(true)
	{
		// If we know the screen needs to be redrawn, bail out of the event
		// loop as soon as there are no more pending events. Otherwise, stay
		// in the event loop until something triggers a redraw. This reduces
		// CPU usage when the game is "idle."
		SDL_Event event;
		if(!mustRedraw)
			SDL_WaitEvent(&event);
		else if(!SDL_PollEvent(&event))
			return true;
		
		// Regardless of what UI layer is active, check quit and timer events:
		if(event.type == SDL_QUIT || (event.type == SDL_KEYDOWN
				&& event.key.keysym.sym == 'q' && event.key.keysym.mod & (KMOD_CTRL | KMOD_GUI)))
			return false;
		else if(event.type == SDL_WINDOWEVENT)
		{
			if(event.window.event == SDL_WINDOWEVENT_RESIZED)
			{
				mustRedraw = true;
				screen = SDL_GetWindowSurface(window);
				if(!fullscreen)
					windowSize = Point(screen->w, screen->h);
			}
		}
		else if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_F11)
		{
			fullscreen = !fullscreen;
			SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
			if(!fullscreen)
			{
				// If leaving fullscreen mode, the "screen" surface is
				// invalidated but no WINDOWEVENT_RESIZED is produced.
				mustRedraw = true;
				screen = SDL_GetWindowSurface(window);
			}
		}
		else if(event.type == SDL_USEREVENT)
		{
			Sprite::Step();
			// Don't move the avatar if the menu is open.
			if(!menu)
				world.Step();
			mustRedraw = true;
		}
		else if(menu)
		{
			// If the menu is active, it's the only thing receiving events.
			const string &result = menu->Handle(event);
			if(result.empty())
				continue;
			else if(result == "new")
			{
				// If for some reason it's not possible to initialize the
				// world based on the given data, stay in the menu.
				if(world.New())
					menu = nullptr;
			}
			else if(result == "continue")
			{
				// The "continue" button only works if a game is loaded.
				if(world)
					menu = nullptr;
			}
			else if(result == "quit")
				return false;
			else
			{
				// Try to go to the named menu page, but if it does not
				// exist go to the main menu instead.
				menu = Menu::Get(result);
				if(!menu)
					menu = Menu::Get("main");
			}
			mustRedraw = true;
		}
		else if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
		{
			menu = Menu::Get("main");
			mustRedraw = true;
		}
		// If we're here, the menu is not open, so give the event to World.
		else
			mustRedraw |= world.Handle(event);
	}
}



uint32_t TimerFunction(uint32_t interval, void *)
{
	SDL_Event event;
	event.type = SDL_USEREVENT;
	SDL_PushEvent(&event);
	
	return interval;
}



bool Init(char *argv[])
{
	string dataPath = argv[1] ? argv[1] : "data.txt";
	// Convert backward slashes to forward slashes, if on windows.
#ifdef _WIN32
	for(char &c : dataPath)
		if(c == '\\')
			c = '/';
#endif
	string directory = dataPath.substr(0, dataPath.rfind('/') + 1);
	Font::SetDirectory(directory + "fonts/");
	
	// Load the data file, and read the first line to get the game title.
	Data data(dataPath);
	string title = World::LoadConfig(data);
	if(title.empty())
	{
		cerr << "Unable to load the game data." << endl;
		return false;
	}
	// Also read the global whimsy preferences.
	ReadPreferences();
	
	// Create the SDL window.
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER);
	IMG_Init(IMG_INIT_PNG);
	uint32_t flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
	if(fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	window = SDL_CreateWindow(
		title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		windowSize.X(), windowSize.Y(), flags);
	SDL_SetWindowMinimumSize(window, MIN_WINDOW_SIZE.X(), MIN_WINDOW_SIZE.Y());
	screen = SDL_GetWindowSurface(window);
	if(!window || !screen)
	{
		cerr << "Unable to initialize SDL." << endl;
		return false;
	}
	
	// Load all the game data.
	World::Load(data);
	// Make sure that at least a default font was loaded.
	if(!Font::IsLoaded())
	{
		cerr << "Unable to load the font." << endl;
		return false;
	}
	// Attempt to load a saved game.
	world.Init();
	// Always show the menu on startup. If there is no "main" menu defined, go
	// straight into the world instead (the saved game if it was loaded, and
	// otherwise a new one).
	menu = Menu::Get("main");
	if(!menu && !world && !world.New())
	{
		cerr << "Unable to load the world data." << endl;
		return false;
	}
	
	// Start the animation timer. It's conceivable that certain games might not
	// define a frame timer - for example, a game entirely driven by clicking on
	// interaction icons, where the avatar does not actually move.
	if(World::FrameRate() > 0)
		frameTimer = SDL_AddTimer(1000 / World::FrameRate(), TimerFunction, nullptr);
	
	// Report success.
	return true;
}



void Free()
{
	// Stop the frame timer.
	if(frameTimer)
		SDL_RemoveTimer(frameTimer);
	if(window)
	{
		// Free the sprites.
		Sprite::FreeAll();
		
		// Free the fonts.
		Font::FreeAll();
		
		// Clean up the SDL resources.
		SDL_DestroyWindow(window);
		SDL_Quit();
	}
}



void ReadPreferences()
{
	// Figure out the path to the saved game folder:
	char *path = SDL_GetPrefPath("whimsy", "");
	preferencesPath = path;
	SDL_free(path);
	// Fix Windows-style directory separators.
	for(char &c : preferencesPath)
		if(c == '\\')
			c = '/';
	// Note that SDL guarantees the path ends with a directory separator.
	preferencesPath += "config.txt";
	
	for(Data data(preferencesPath); data; data.Next())
	{
		if(data.Tag() == "window" && data.Size() >= 2)
			windowSize = data[1];
		else if(data.Tag() == "fullscreen")
			fullscreen = true;
	}
}



void SavePreferences()
{
	ofstream out(preferencesPath);
	out << "window " << windowSize.X() << "," << windowSize.Y() << '\n';
	if(fullscreen)
		out << "fullscreen" << '\n';
}
