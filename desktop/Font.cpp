/* Font.cpp
Copyright 2020 Michael Zahniser
*/

#include "Font.h"

#include <SDL2/SDL_image.h>

#include <algorithm>
#include <cstring>
#include <map>

using namespace std;

namespace {
	// Directory where the font images are stored.
	string directory = "fonts/";
	
	// Object storing a style definition.
	class Style {
	public:
		// Get the path to the image file for this style.
		string Path() const;
		
		string family;
		int size = 16;
		string weight;
		string style;
		Color color;
	};
	Style defaultStyle;
	map<string, Font::Metrics> baseMetrics;
	map<string, Font> fonts;
	
	// Minimum spacing to add between glyphs in addition to the advance.
	const int KERN = 2;
	// Map a character to a glyph index.
	int Glyph(char c, bool isAfterSpace);
}



// Set the directory where font images are stored.
void Font::SetDirectory(const string &path)
{
	directory = path;
}



// Load a new font style based on the given data block. The Data object will
// be advanced to the end of the block.
void Font::Add(Data &data)
{
	string name = data.Value();
	Style style = defaultStyle;
	// Then, read any overrides for those defaults.
	while(data.Next() && data.Size())
	{
		if(data.Tag() == "family")
			style.family = data.Value();
		if(data.Tag() == "size")
			style.size = data[1];
		else if(data.Tag() == "bold")
			style.weight = "bold";
		else if(data.Tag() == "italic")
			style.style = "italic";
		else if(data.Tag() == "normal")
		{
			style.weight.clear();
			style.style.clear();
		}
		else if(data.Tag() == "color")
			style.color = Color(data[1], data[2], data[3]);
	}
	// If this style is unnamed, it is the default style.
	if(name.empty())
		defaultStyle = style;
	
	// Now, generate a new font object based on this style.
	string path = style.Path();
	fonts.try_emplace(name, path, baseMetrics[path], style.color);
}



// Check if the named font is loaded. Leave the argument blank to check for
// the default font.
bool Font::IsLoaded(const string &name)
{
	return fonts.count(name);
}



// Get the font for the named style. If no such style is defined or if the
// style is an empty string, this returns the default font.
const Font &Font::Get(const string &name)
{
	map<string, Font>::const_iterator it = fonts.find(name);
	if(it == fonts.end())
		it = fonts.find("");
	return it->second;
}



// Free all the glyph sheets.
void Font::FreeAll()
{
	fonts.clear();
	baseMetrics.clear();
}



// Destructor.
Font::~Font()
{
	if(glyphs)
		SDL_FreeSurface(glyphs);
}



// Draw the given string, with its top left corner at the given (x, y).
void Font::Draw(const string &text, Point point, SDL_Surface *surface) const
{
	bool isAfterSpace = true;
	
	// The bounding box of the output glyph:
	Rect rect(point);
	// Keep track of the previous non-space character, for kerning:
	int prev = 0;
	for(const char *it = text.data(); *it; ++it)
	{
		// Select curly quotes based on whether the most recent non-quote
		// character was a space.
		int next = Glyph(*it, isAfterSpace);
		if(*it != '"' && *it != '\'')
			isAfterSpace = !next;
		
		if(!next)
			rect.x += metrics.space;
		else
		{
			rect.x += metrics.Advance(prev, next);
			SDL_BlitSurface(glyphs, &metrics.box[next], surface, &rect);
			prev = next;
		}
	}
}



// Get the width of the given string.
int Font::Width(const string &text) const
{
	bool isAfterSpace = true;
	int width = 0;
	int prev = 0;
	for(const char *it = text.data(); *it; ++it)
	{
		int next = Glyph(*it, isAfterSpace);
		if(*it != '"' && *it != '\'')
			isAfterSpace = !next;
		
		if(!next)
			width += metrics.space;
		else
		{
			width += metrics.Advance(prev, next);
			prev = next;
		}
	}
	// Include the advance to get to the end of the last character.
	return width + metrics.Advance(prev);
}



// Get the kerning adjustment for when the given first string is followed by
// the given second string.
int Font::Kern(const string &first, const string &second) const
{
	// Find the last non-space character of the first string and the first non-
	// space character of the second.
	size_t fit = first.size();
	while(fit && isspace(first[--fit]))
		continue;
	size_t sit = 0;
	while(sit < second.size() && isspace(second[sit]))
		++sit;
	
	// Check whether we succeeded in finding non-space characters in each.
	if(!fit || sit == second.size())
		return 0;
	// When selecting curly quotes, rather than figuring out for sure just
	// assume that there's a space in between the strings.
	int prev = Glyph(first[fit], false);
	int next = Glyph(second[sit], true);
	return metrics.Advance(prev, next) - metrics.Advance(prev);
}



// Initialize this Font by calculating the advances between glyphs.
SDL_Surface *Font::Metrics::Init(const string &path)
{
	// Load the glyphs surface.
	glyphs = IMG_Load(path.data());
	if(!glyphs)
		return nullptr;
	
	// Lock the surface for pixel access.
	SDL_LockSurface(glyphs);
	
	// Get the format and size of the surface.
	int width = glyphs->w / GLYPHS;
	int height = glyphs->h;
	uint32_t mask = glyphs->format->Amask;
	uint32_t half = 0x40404040 & mask;
	int pitch = glyphs->pitch / glyphs->format->BytesPerPixel;
	uint32_t *pixels = reinterpret_cast<uint32_t *>(glyphs->pixels);
	
	// advance[previous * GLYPHS + next] is the x advance for each glyph pair.
	// There is no advance if the previous value is 0, i.e. we are at the very
	// start of a string.
	memset(advance, 0, GLYPHS * sizeof(advance[0]));
	for(int prev = 1; prev < GLYPHS; ++prev)
		for(int next = 0; next < GLYPHS; ++next)
		{
			int maxD = 0;
			int glyphWidth = 0;
			uint32_t *begin = pixels;
			for(int y = 0; y < height; ++y)
			{
				// Find the last non-empty pixel in the previous glyph.
				uint32_t *pend = begin + prev * width;
				uint32_t *pit = pend + width;
				while(pit != pend && (*--pit & mask) < half) {}
				int distance = (pit - pend) + 1;
				glyphWidth = max(distance, glyphWidth);
				
				// Special case: if "next" is zero (i.e. end of line of text),
				// calculate the full width of this character. Otherwise:
				if(next)
				{
					// Find the first non-empty pixel in this glyph.
					uint32_t *nit = begin + next * width;
					uint32_t *nend = nit + width;
					while(nit != nend && (*nit++ & mask) < half) {}
					
					// How far apart do you want these glyphs drawn? If drawn at
					// an advance of "width", there would be:
					// pend + width - pit   <- pixels after the previous glyph.
					// nit - (nend - width) <- pixels before the next glyph.
					// So for zero kerning distance, you would want:
					distance += 1 - (nit - (nend - width));
				}
				maxD = max(maxD, distance);
				
				// Update the pointer to point to the beginning of the next row.
				begin += pitch;
			}
			// This is a fudge factor to avoid over-kerning, especially for the
			// underscore and for glyph combinations like AV.
			advance[prev * GLYPHS + next] = KERN + max(maxD, glyphWidth - 4);
		}
	SDL_UnlockSurface(glyphs);
	
	// Set the space size based on the character width.
	space = (width + 3) / 6 + 1;
	
	// Set the bounding box of each glyph.
	for(int i = 0; i < GLYPHS; ++i)
		box[i] = Rect(i * width, 0, width, height);
	
	return glyphs;
}



// Get the advance between the given two glyph indices.
int Font::Metrics::Advance(int prev, int next) const
{
	return advance[prev * GLYPHS + next];
}



// Construct a font with the given metrics and color:
Font::Font(const string &path, Metrics &metrics, const Color &color)
	: metrics(metrics)
{
	// The first font to use a given source image does not need to make a copy
	// of the glyphs surface; it can just recolor it in place.
	if(metrics.glyphs)
		glyphs = SDL_ConvertSurface(metrics.glyphs, metrics.glyphs->format, 0);
	else
		glyphs = metrics.Init(path);
	
	// Color in the glyphs.
	uint32_t mask = glyphs->format->Amask;
	uint32_t rgb = color(glyphs) & ~mask;
	
	// Lock the surface for pixel access.
	SDL_LockSurface(glyphs);
	
	int pitch = glyphs->pitch / glyphs->format->BytesPerPixel;
	uint32_t *pixels = reinterpret_cast<uint32_t *>(glyphs->pixels);
	
	for(int y = 0; y < glyphs->h; ++y)
	{
		uint32_t *it = pixels + y * pitch;
		for(uint32_t *end = it + glyphs->w; it != end; ++it)
			*it = (*it & mask) | rgb;
	}
	
	// Unlock the surface.
	SDL_UnlockSurface(glyphs);
}



namespace {
	// Get the path to the image for a font with this style.
	string Style::Path() const
	{
		string path = directory + family + '-' + to_string(size);
		if(!weight.empty())
			path += '-' + weight;
		if(!style.empty())
			path += '-' + style;
		path += ".png";
		return path;
	}
	
	// Map a character to a glyph index.
	int Glyph(char c, bool isAfterSpace)
	{
		// Curly quotes.
		if(c == '\'' && isAfterSpace)
			return 96;
		if(c == '"' && isAfterSpace)
			return 97;
		
		return max(0, min(Font::Metrics::GLYPHS - 3, c - 32));
	}
}
