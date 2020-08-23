/* Dialog.cpp
Copyright 2020 Michael Zahniser
*/

#include "Dialog.h"

#include "Color.h"
#include "Sprite.h"
#include "Text.h"
#include "Variables.h"
#include "World.h"

#include <map>

using namespace std;

namespace {
	// This class stores a single "node" of the dialog, i.e. a section of the
	// conversation that you can skip to via a goto or an option.
	class Node : public vector<string> {
	public:
		string ask;
	};
	map<string, Node> nodes;
	
	// Parameters for drawing dialogs:
	const Color DIALOG_COLOR(180, 180, 180);
	const Color OPTION_COLOR(200, 200, 200);
	const Color HOVER_COLOR(150, 170, 200);
	const Color LINE_COLOR(0, 0, 0);
	const int WRAP_WIDTH = 400;
	const Point BOX_PAD(10, 10);
	const int OPTION_PAD = 10 + 2 * BOX_PAD.Y();
	const int DIALOG_Y = 40 + BOX_PAD.Y();
	const Point TEXT_OFFSET(0, 2);
	
	// Skip an indented block in a data file.
	void SkipBlock(Data &data)
	{
		// Skip the indented block after this line. Note that data.Indent()
		// returns 0 if we reach a blank line, so there is no need to check
		// data.Size().
		int indent = data.Indent();
		while(data.Next() && data.Indent() > indent)
			continue;
	}
	// Draw a rectangle with a frame around it.
	void FrameRect(SDL_Surface *surface, Rect rect, const Color &color)
	{
		SDL_FillRect(surface, &rect, LINE_COLOR(surface));
		rect.Grow(-1);
		SDL_FillRect(surface, &rect, color(surface));
	}
}



// Load a dialog definition from the given data file.
void Dialog::Load(Data &data)
{
	Node &node = nodes[data.Value()];
	while(data.Next() && data.Size())
	{
		if(data.Tag() == "ask")
			node.ask = data.Value();
		else
			node.push_back(data.Line());
	}
}



Dialog::Dialog(World &world)
	: world(world)
{
}



// Begin the given dialog.
void Dialog::Begin(const string &name)
{
	data = Data(nodes[name]);
	Step();
}



void Dialog::Begin(const vector<string> &changes)
{
	data = Data(changes);
	Step();
}



// Write the dialog's current state to a saved game file.
void Dialog::Save(ostream &out) const
{
	// Write the current state that the dialog has accumulated.
	if(!text.empty())
		out << "say " << text << '\n';
	if(icon)
		out << "icon " << icon << '\n';
	if(scene)
		out << "scene " << scene << '\n';
	for(const string &option : options)
		out << "option " << option << '\n';
	if(!exitText.empty())
		out << "exit " << exitText << '\n';
	// Also write the remainder of this node that it has not yet executed.
	data.Save(out);
}



void Dialog::Close()
{
	ClearOptions();
	icon = 0;
	scene = 0;
}



// Check if the dialog needs to be drawn.
bool Dialog::IsOpen() const
{
	return !text.empty() || icon || scene || !optionText.empty();
}



// Draw the dialog. If the given mouse position is on one of the options,
// highlight it.
void Dialog::Draw(SDL_Surface *screen, Point hover) const
{
	Text wrap(WRAP_WIDTH);
	wrap.Wrap(text);

	// The dialog box is laid out as follows:
	// First, the scene (if any), centered horizontally and occupying the whole
	// width of the box. (If necessary, the box expands to fit it.)
	// Then, the icon and the text side by side, with BOX_PAD between them. If
	// there is extra width (because of a wide scene) they are centered.
	// TODO: Is there any way to simplify this math?
	Point dialogSize(wrap.Width(), wrap.Height());
	if(icon)
	{
		const Sprite &sprite = Sprite::Get(icon);
		dialogSize.X() += Sprite::Get(icon).Width() + BOX_PAD.X();
		dialogSize.Y() = max(dialogSize.Y(), sprite.Height());
	}
	if(scene)
	{
		const Sprite &sprite = Sprite::Get(scene);
		dialogSize.X() = max(dialogSize.X(), sprite.Width());
		dialogSize.Y() += sprite.Height() + BOX_PAD.Y();
	}
	// Center the dialog in X, and put it near the top of the screen in Y.
	Point corner((screen->w - dialogSize.X()) / 2, DIALOG_Y);
	Rect rect(corner - BOX_PAD, corner + dialogSize + BOX_PAD);
	FrameRect(screen, rect, DIALOG_COLOR);
	
	// Draw the scene, icon, and text.
	if(scene)
	{
		const Sprite &sprite = Sprite::Get(scene);
		// The top left corner of the sprite should be at the top of the box and
		// the left and right should have equal padding.
		int hPad = (dialogSize.X() - sprite.Width()) / 2;
		sprite.Draw(screen, corner - sprite.Bounds().TopLeft() + Point(hPad, 0));
		int height = BOX_PAD.Y() + sprite.Height();
		corner.Y() += height;
		dialogSize.Y() -= height;
	}
	int width = wrap.Width();
	if(icon)
	{
		const Sprite &sprite = Sprite::Get(icon);
		width += sprite.Width() + BOX_PAD.X();
		int hPad = (dialogSize.X() - width) / 2;
		sprite.Draw(screen, corner - sprite.Bounds().TopLeft() + Point(hPad, 0));
	}
	// Given the width of this entire lower section, calculate the text offset.
	corner.X() += (dialogSize.X() + width) / 2 - wrap.Width();
	wrap.Draw(screen, corner + TEXT_OFFSET);
	// Options should only be as wide as the text.
	dialogSize.X() = wrap.Width();
	
	// Show the options, or else a prompt to continue.
	static const vector<string> PROMPT = {"(Click anywhere to continue.)"};
	string number = "0: ";
	optionRects.clear();
	for(const string &option : optionText.empty() ? PROMPT : optionText)
	{
		corner.Y() += dialogSize.Y() + OPTION_PAD;
		++number[0];
		wrap.Wrap(number + option);
		dialogSize.Y() = wrap.Height();
		rect = Rect(corner - BOX_PAD, corner + dialogSize + BOX_PAD);
		FrameRect(screen, rect, rect.Contains(hover) ? HOVER_COLOR : OPTION_COLOR);
		wrap.Draw(screen, corner + TEXT_OFFSET);
		optionRects.push_back(rect);
	}
}



// Handle an event, and return true if the screen needs to be redrawn.
bool Dialog::Handle(const SDL_Event &event)
{
	if(event.type == SDL_MOUSEBUTTONDOWN)
	{
		if(optionText.empty())
		{
			Acknowledge();
			return true;
		}
		size_t clicked = Button(Point(event.button.x, event.button.y));
		if(clicked != optionRects.size())
		{
			Choose(clicked);
			return true;
		}
	}
	else if(event.type == SDL_MOUSEMOTION)
	{
		Point point(event.motion.x, event.motion.y);
		Point previous = point - Point(event.motion.xrel, event.motion.yrel);
		if(Button(point) != Button(previous))
			return true;
	}
	else if(event.type == SDL_KEYDOWN)
	{
		SDL_Keycode sym = event.key.keysym.sym;
		if(!optionText.empty() && sym >= '1' && sym <= '9')
		{
			size_t choice = sym - '1';
			if(choice < optionText.size())
			{
				Choose(choice);
				return true;
			}
		}
		else if(optionText.empty() && (sym == '1' || sym == SDLK_SPACE || sym == SDLK_RETURN))
		{
			Acknowledge();
			return true;
		}
	}
	return false;
}



// Select the given option, advancing the dialog to the next page.
void Dialog::Choose(int option)
{
	// Check if the chosen option was to exit the conversation.
	if(static_cast<size_t>(option) < options.size())
	{
		visited.insert(options[option]);
		data = Data(nodes[options[option]]);
		ClearOptions();
		Step();
	}
	else
		Close();
}



// If the dialog is just waiting for acknowledgment, instead of for a
// specific choice, go on to the next page.
void Dialog::Acknowledge()
{
	if(!data)
		Close();
	else
		Step();
}



void Dialog::Step()
{
	// The scene resets every page. The icon persists.
	scene = 0;
	bool spoke = false;
	while(data)
	{
		// Special case: a "goto" node moves us to new data immediately.
		if(data.Tag() == "goto")
		{
			data = Data(nodes[data.Value()]);
			// Process this line instead of advancing to the next one.
			continue;
		}
		else if(data.Tag() == "if")
		{
			if(!Variables::Eval(data.Value()))
			{
				SkipBlock(data);
				// We are now at the first line after the end of the "if" block
				// that we skipped. If that line is an "else" we can skip that
				// line and move on into the contents of the else. Otherwise, we
				// need to "continue" to avoid advancing past this line.
				if(data.Tag() != "else")
					continue;
			}
		}
		else if(data.Tag() == "else")
		{
			// If we got to this line, it means we didn't skip the previous "if"
			// clause, so we need to skip this one.
			SkipBlock(data);
			// We don't need to call data.Next() because it is already at the
			// first line after the block.
			continue;
		}
		else if(data.Tag() == "option")
		{
			if(!visited.count(data.Value()))
				options.push_back(data.Value());
		}
		else if(data.Tag() == "exit")
			exitText = data.Size() > 1 ? data.Value() : "(End conversation.)";
		else if(data.Tag() == "icon")
			icon = data[1];
		else if(data.Tag() == "scene")
			scene = data[1];
		else if(data.Tag() == "add")
		{
			string room = data.Value();
			int indent = data.Indent();
			data.Next();
			while(data.Indent() > indent)
			{
				if(data.Tag() == "interaction")
					world.Add(Interaction(data), room);
				else
				{
					world.Add(data[0], data[1], data.Value(2), room);
					data.Next();
				}
			}
			// Return to the top of the loop without calling data.Next() again.
			continue;
		}
		else if(data.Tag() == "remove")
		{
			string room = data.Value();
			int indent = data.Indent();
			while(data.Next() && data.Indent() > indent)
				world.Remove(data.Value(0), room);
			// Return to the top of the loop without calling data.Next() again.
			continue;
		}
		else if(data.Tag() == "enter")
			world.Enter(data[1], data[2]);
		else if(data.Tag() == "face")
			world.Face(data[1]);
		else if(data.Tag() == "set")
			Variables::Set(data.Value());
		else if(data.Tag() == "say")
		{
			// If we've reached another "say" line, that means the current
			// "paragraph" should be shown immediately, and we don't need to
			// show options because we're not yet at the end of the stream.
			if(spoke)
				return;
			spoke = true;
			text = data.Value();
		}
		// If we didn't "continue" up above, advance to the next line.
		data.Next();
	}
	
	// We've reached the end of the stream. Show any options we accumulated.
	for(const string &option : options)
		optionText.push_back(nodes[option].ask);
	if(!exitText.empty())
		optionText.push_back(exitText);
}



void Dialog::ClearOptions()
{
	// Begin accumulating options from scratch.
	optionText.clear();
	options.clear();
	exitText.clear();
	// The text disappears, but the icon will remain until cleared manually or
	// until the conversation ends.
	text.clear();
}



// Check if the given point is inside one of the options. If not, this
// returns the size of the options vector.
size_t Dialog::Button(Point p)
{
	for(size_t i = 0; i < optionRects.size(); ++i)
		if(optionRects[i].Contains(p))
			return i;
	return optionRects.size();
}
