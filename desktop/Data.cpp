/* Data.cpp
Copyright 2020 Michael Zahniser
*/

#include "Data.h"

#include <cctype>
#include <cstdlib>
#include <fstream>

using namespace std;

namespace {
	// Extract the directory path from the given file path.
	string DirPath(const string &path)
	{
		// If there is no '/' in the path, it's a file in the current working
		// directory, so the directory path should be empty. Otherwise, return
		// everything up to and including the '/'. Because string::npos = -1,
		// both conditions simplify down to this:
		return path.substr(0, path.rfind('/') + 1);
	}
}



Data::Data()
	: it(lines.begin()), end(lines.end())
{
}



Data::Data(const string &path)
{
	// Read the entire file, and any files it includes.
	Load(path);
	// Tokenize the first line.
	it = lines.begin();
	end = lines.end();
	Tokenize();
}



Data::Data(const vector<string> &lines)
{
	it = lines.begin();
	end = lines.end();
	Tokenize();
}



// Read one line of data. Return true unless we're at the end of the file.
bool Data::Next()
{
	if(it == end)
		return false;
	
	++it;
	Tokenize();
	return true;
}



// Check if we're at the end of the data.
Data::operator bool() const
{
	return !!*this;
}



bool Data::operator!() const
{
	return (it == end);
}



// Write the portion of this data file that has not yet been processed to
// the given output stream.
void Data::Save(ostream &out) const
{
	for(vector<string>::const_iterator rit = it; rit != end; ++rit)
		out << *rit << '\n';
}



// Get the "working directory" of the current line of the data file - that
// is, the directory containing the file that contributes that line.
const string &Data::Directory() const
{
	return directory;
}



// Get the number of arguments in this line.
size_t Data::Size() const
{
	return tokens.size() / 2;
}



// Get the entire line as one string, including leading and trailing space.
const string &Data::Line() const
{
	return *it;
}



// Get the "tag," i.e. the first word of the line.
string Data::Tag() const
{
	if(tokens.empty())
		return string();
	
	return (*this)[0];
}



// Get the "value," i.e. all the text after the first word. For some data
// types this will be a single field, and for others it may contain multiple
// fields separated by whitespace. Trailing spaces are trimmed out.
string Data::Value(size_t index) const
{
	if(tokens.size() < 2 * (index + 1))
		return string();
	
	size_t pos = tokens[2 * index];
	return it->substr(pos, tokens.back() - pos);
}



// Get the current line's indent, in characters. All characters count the
// same, so a tab is the same as a single space, not multiple spaces.
int Data::Indent() const
{
	return tokens.empty() ? 0 : tokens.front();
}



// Get the given whitespace-separated argument.
Data::Arg Data::operator[](size_t index) const
{
	return Arg(*it, tokens[index * 2], tokens[index * 2 + 1]);
}



Data::Arg::Arg(const string &line, size_t start, size_t end)
	: line(line), start(start), end(end)
{
}



// Allow an argument to be typecast to any of the following types.
Data::Arg::operator string() const
{
	return line.substr(start, end - start);
}



Data::Arg::operator bool() const
{
	return !line.compare(start, end - start, "false");
}



Data::Arg::operator int() const
{
	return atoi(line.c_str() + start);
}



Data::Arg::operator size_t() const
{
	return atoi(line.c_str() + start);
}



Data::Arg::operator Point() const
{
	// Look for a comma.
	const char *str = line.c_str();
	for(size_t i = start; i < end; ++i)
		if(line[i] == ',')
			return Point(atoi(str + start), atoi(str + i + 1));
	
	return Point();
}



// Check what type of argument this is.
bool Data::Arg::IsInt() const
{
	for(size_t i = start; i < end; ++i)
		if(line[i] < '0' || line[i] > '9')
			return false;
	return true;
}



// Load lines from the given file, appending to the existing lines rather
// than replacing them. This allows recursive "include" commands.
void Data::Load(const string &path)
{
	string directory = DirPath(path);
	directories.emplace_back(lines.size(), directory);
	
	ifstream in(path);
	string line;
	while(getline(in, line))
	{
		// Find the first non-whitespace character.
		size_t i = 0;
		while(i != line.size() && isspace(line[i]))
			++i;
		if(i != line.size())
		{
			// This line is not empty. Check for some special cases.
			// If the line is a comment, skip it:
			if(line[i] == '#')
				continue;
			static const string INCLUDE = "include";
			if(!line.compare(i, i + INCLUDE.size(), INCLUDE) && isspace(line[i + INCLUDE.size()]))
			{
				// Find the start of the second token.
				i += INCLUDE.size() + 1;
				while(i != line.size() && isspace(line[i]))
					++i;
				if(i < line.size())
				{
					// Leave a blank line before and after the file contents.
					// That will keep lines in separate files from being
					// interpreted as part of the same data block.
					lines.emplace_back();
					Load(directory + line.substr(i));
					lines.emplace_back();
					directories.emplace_back(lines.size(), directory);
				}
				continue;
			}
		}
		// Append this line to the list.
		lines.emplace_back();
		lines.back().swap(line);
	}
}



bool Data::Tokenize()
{
	// Update the directory path if this line is the start of an included file.
	if(!directories.empty() && directories.front().first == lineIndex)
	{
		directory = directories.front().second;
		directories.erase(directories.begin());
	}
	++lineIndex;
	
	tokens.clear();
	if(it == end)
		return false;
	
	// Tokenize the line.
	bool was = true;
	for(size_t i = 0; i < it->size(); ++i)
	{
		bool is = isspace((*it)[i]);
		if(is != was)
			tokens.push_back(i);
		was = is;
	}
	// If the line didn't end in whitespace, mark the end of the last token.
	if(tokens.size() % 2)
		tokens.push_back(it->size());
	
	return true;
}
