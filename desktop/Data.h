/* Data.h
Copyright 2020 Michael Zahniser
*/

#ifndef DATA_H_
#define DATA_H_

#include "Point.h"

#include <ostream>
#include <string>
#include <utility>
#include <vector>

using namespace std;



// A class representing a data file, which is just a collection of lines that
// are broken up into "tokens" by whitespace. Some lines may be stand-alone
// commands, and others may be "blocks" of data terminated by an empty line.
// Comments (lines beginning with #) and file inclusion (include <path>) are
// supported.
class Data {
public:
	// Don't allow copying a Data object, but do allow moving.
	Data();
	Data(const Data &) = delete;
	Data(Data &&) = default;
	Data(const string &path);
	Data(const vector<string> &lines);
	
	Data &operator=(const Data &) = delete;
	Data &operator=(Data &&) = default;
	
	// Read one line of data. Return true unless we're at the end of the file.
	bool Next();
	// Check if we're at the end of the data.
	operator bool() const;
	bool operator!() const;
	
	// Write the portion of this data file that has not yet been processed to
	// the given output stream.
	void Save(ostream &out) const;
	
	// Get the "working directory" of the current line of the data file - that
	// is, the directory containing the file that contributes that line.
	const string &Directory() const;
	
	// Get the number of arguments in this line.
	size_t Size() const;
	
	// Get the entire line as one string, including leading and trailing space.
	const string &Line() const;
	// Get the "tag," i.e. the first word of the line.
	string Tag() const;
	// Get the "value," i.e. all the text after the first word. For some data
	// types this will be a single field, and for others it may contain multiple
	// fields separated by whitespace. Trailing spaces are trimmed out.
	string Value(size_t index = 1) const;
	// Get the current line's indent, in characters. All characters count the
	// same, so a tab is the same as a single space, not multiple spaces.
	int Indent() const;
	// Get the given whitespace-separated argument.
	class Arg;
	Arg operator[](size_t index) const;
	
	
public:
	// Nested class, representing a single argument that can be interpreted in a
	// variety of ways.
	class Arg {
	public:
		Arg(const string &line, size_t start, size_t end);
		
		// Allow an argument to be typecast to any of the following types.
		operator string() const;
		operator bool() const;
		operator int() const;
		operator size_t() const;
		operator Point() const;
		
		// Check what type of argument this is.
		bool IsInt() const;
		
	private:
		const string &line;
		const size_t start;
		const size_t end;
	};
	
	
private:
	// Load lines from the given file, appending to the existing lines rather
	// than replacing them. This allows recursive "include" commands.
	void Load(const string &path);
	// Find the start and end indices of the tokens in the current line.
	bool Tokenize();
	
	
private:
	vector<string> lines;
	vector<string>::const_iterator it;
	vector<string>::const_iterator end;
	vector<size_t> tokens;
	
	size_t lineIndex = 0;
	string directory;
	vector<pair<size_t, string>> directories;
};



#endif
