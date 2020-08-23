/* Font.h
Copyright 2020 Michael Zahniser
*/

#ifndef FONT_H_
#define FONT_H_

#include "Color.h"
#include "Data.h"
#include "Point.h"
#include "Rect.h"

#include <SDL2/SDL.h>

#include <string>

using namespace std;



class Font {
public:
	// Load a new font style based on the given data block. The Data object will
	// be advanced to the end of the block.
	static void Add(Data &data);
	// Check if the named font is loaded. Leave the argument blank to check for
	// the default font.
	static bool IsLoaded(const string &name = "");
	// Get the font for the named style. If no such style is defined or if the
	// style is an empty string, this returns the default font.
	static const Font &Get(const string &name = "");
	// Free all the glyph sheets.
	static void FreeAll();
	
	
public:
	// Destructor.
	~Font();
	// Draw the given string, with its top left corner at the given (x, y).
	void Draw(const string &text, Point point, SDL_Surface *surface) const;
	
	// Get the width of the given string.
	int Width(const string &text) const;
	// Get the kerning adjustment for when the given first string is followed by
	// the given second string.
	int Kern(const string &first, const string &second) const;
	
	
public:
	// Object storing the metrics for a given font. This is public only so it
	// can be used in helper functions in the implementation code.
	// TODO: find a less messy way of accomplishing that.
	class Metrics {
	public:
		// Load the glyph sheet for fonts with these metrics.
		SDL_Surface *Init(const string &path);
		
		// Get the advance between the given two glyph indices.
		int Advance(int prev, int next = 0) const;
		
		// Pointer to the first glyph sheet for a font with these metrics.
		// Multiple fonts may use copies of this sheet with different colors.
		SDL_Surface *glyphs = nullptr;
		// Skip characters 0..31 and characters 127...255.
		// That leaves 128 - 32 - 1 = 95 characters, plus one more
		// glyph for any characters outside that range. Add two more glyphs for
		// right-side single and double quotes.
		static const int GLYPHS = 98;
		// Kerning between any two glyphs: advance[a + b * GLYPHS] is the advance
		// for glyph a followed by glyph b.
		int advance[GLYPHS * GLYPHS];
		// Bounding box of each glyph.
		Rect box[GLYPHS];
		// This value will be adjusted based on the character height.
		int space;
	};
	
	
public:
	// Construct a font with the given metrics and color:
	Font(const string &path, Metrics &metrics, const Color &color);
	
	
private:
	// Each Font object has its own color, but all different colors of a given
	// font share the same metrics.
	const Metrics &metrics;
	SDL_Surface *glyphs = nullptr;
};



#endif
