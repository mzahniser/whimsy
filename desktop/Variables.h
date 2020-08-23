/* Variables.h
Copyright 2020 Michael Zahniser
*/

#ifndef VARIABLES_H_
#define VARIABLES_H_

#include <ostream>
#include <string>

using namespace std;



class Variables {
public:
	// Evaluate an expression. (Note: it is assumed that this is the contents of
	// an "if", so it cannot assign new values to any variables.)
	static int Eval(const string &line);
	// Evaluate a "set" command, changing the value of a single variable. If no
	// assignment operator is found, an implied " = 1" is appended.
	static int Set(const string &line);
	// Clear all variable definitions.
	static void Clear();
	// Write the current variable values to a saved game file.
	static void Save(ostream &out);
};



#endif
