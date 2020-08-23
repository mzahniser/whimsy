/* Text.h
Copyright 2020 Michael Zahniser
*/

#ifndef TEXT_H_
#define TEXT_H_

#include "Point.h"

#include <SDL2/SDL.h>

#include <string>
#include <vector>

using namespace std;



// This class represents formatted and wrapped text broken up into segments that
// can each be drawn with a single Font call.
class Text {
public:
	// Initialize a Text object, setting the font and the wrap width.
	Text(int width);

	// Wrap the given text. The text will be parsed for style tags, including
	// paragraph break tags.
	void Wrap(const string &text);

	// Get the current wrap width. The text is always wrapped flush left.
	int Width() const;
	void SetWidth(int width);
	// Get the height of the currently wrapped text.
	int Height() const;

	// Get or set the line height.
	int LineHeight() const;
	void SetLineHeight(int height);

	// Get or set paragraph spacing. This is in addition to the line spacing.
	int ParagraphSpacing() const;
	void SetParagraphSpacing(int spacing);

	// Draw this text on the given surface, with its top left corner offset by
	// the given amount relative.
	void Draw(SDL_Surface *surface, Point corner) const;


private:
	int wrapWidth = 400;
	int lineHeight = 20;
	int paragraphSpacing = 10;

	// Object representing a segment of text, all drawn on the same line and in
	// the same color.
	class Segment {
	public:
		Segment(string text, Point position, const string &style = "")
			: text(text), position(position), style(style) {}
		
		string text;
		Point position;
		string style;
	};
	vector<Segment> segments;
	int height;
};

#endif // TEXT_H_
