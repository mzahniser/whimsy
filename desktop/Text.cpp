/* Text.cpp
Copyright 2020 Michael Zahniser
*/

#include "Text.h"

#include "Font.h"

#include <algorithm>

using namespace std;



// Initialize a Text object, setting the font and the wrap width.
Text::Text(int width)
	: wrapWidth(width)
{
}



// Wrap the given text. The text will be parsed for style tags, including
// paragraph break tags.
void Text::Wrap(const string &text)
{
	segments.clear();
	
	// Begin with the default font.
	string style;
	const Font *font = &Font::Get(style);
	// The first line of text starts out at offset (0, 0).
	Point anchor;
	// Iterate through the given text. Break it up into sections separated by
	// {style} tags. If a section does not fit on a single line, break it up.
	size_t first = 0;
	while(first < text.size())
	{
		// Find the next style tag.
		size_t last = min(text.find('{', first), text.size());
		// Find the right place to break this line, if necessary.
		// TODO: Find a more efficient way to do this.
		while(first != last)
		{
			// First, see if the whole text will fit on one line.
			string segment = text.substr(first, last - first);
			if(anchor.X() && !segments.empty())
				anchor.X() += font->Kern(segments.back().text, segment);
			int wholeWidth = font->Width(segment);
			if(anchor.X() + wholeWidth <= wrapWidth)
			{
				segments.emplace_back(segment, anchor, style);
				anchor.X() += wholeWidth;
				break;
			}
			// Scan forward to see where we need to break this string to fit it
			// within the current line width.
			size_t wrapPos = first;
			for(size_t i = first; i < last; ++i)
				if(text[i] <= ' ')
				{
					segment = text.substr(first, i - first);
					int width = font->Width(segment);
					if(anchor.X() + width <= wrapWidth)
						wrapPos = i;
					else
						break;
				}
			// If there was no whitespace before the line reaches the wrap
			// width, we may need to extend beyond the end of the line.
			if(wrapPos == first)
			{
				if(anchor.X())
				{
					// We weren't at the start of a line, so start a new line
					// and try again to see if wrapping now works.
					anchor.X() = 0;
					anchor.Y() += lineHeight;
					continue;
				}
				else
					wrapPos = min(last, text.find(' ', first));
			}
			// Insert the segment, then move to the next line.
			segment.resize(wrapPos - first);
			segments.emplace_back(segment, anchor, style);
			anchor.X() = 0;
			anchor.Y() += lineHeight;
			// Now, find the end of the whitespace that the wrap started at.
			first = wrapPos;
			while(first < last && text[first] <= ' ')
				++first;
		}
		// Now, find the end of the style tag.
		first = last;
		last = text.find('}', first) + 1;
		// If the style tag doesn't close, bail out here.
		if(!last)
			break;
		// Check if this is a {br} tag.
		if(!text.compare(first, last - first, "{br}"))
		{
			if(anchor.X())
			{
				anchor.X() = 0;
				anchor.Y() += lineHeight;
			}
			anchor.Y() += paragraphSpacing;
		}
		else if(last - first >= 2)
		{
			string newStyle = text.substr(first + 1, last - first - 2);
			// If a style appears twice it's toggling between that style and the
			// default one.
			if(newStyle == style)
				style.clear();
			else
				style.swap(newStyle);
			font = &Font::Get(style);
		}
		first = last;
	}
	if(anchor.X())
		anchor.Y() += lineHeight;
	height = anchor.Y();
}



// Get the current wrap width. The text is always wrapped flush left.
int Text::Width() const
{
	return wrapWidth;
}



void Text::SetWidth(int width)
{
	wrapWidth = width;
}



// Get the height of the currently wrapped text.
int Text::Height() const
{
	return height;
}



// Get or set the line height.
int Text::LineHeight() const
{
	return lineHeight;
}



void Text::SetLineHeight(int height)
{
	lineHeight = height;
}



// Get or set paragraph spacing. This is in addition to the line spacing.
int Text::ParagraphSpacing() const
{
	return paragraphSpacing;
}



void Text::SetParagraphSpacing(int spacing)
{
	paragraphSpacing = spacing;
}



// Draw this text on the given surface, with its top left corner offset by
// the given amount relative.
void Text::Draw(SDL_Surface *surface, Point corner) const
{
	for(const Segment &segment : segments)
	{
		const Font &font = Font::Get(segment.style);
		font.Draw(segment.text, corner + segment.position, surface);
	}
}
