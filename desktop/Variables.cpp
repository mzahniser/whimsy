/* Variables.cpp
Copyright 2020 Michael Zahniser
*/

#include "Variables.h"

#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

using namespace std;

namespace {
	// Variables:
	map<string, int> variables;
	
	// Struct representing a unary or binary operator.
	struct Op {
		string token;
		int precedence = 0;
		// Number of arguments must be 1 (unary) or 2 (binary).
		int args = 2;
		// Function object. For unary operators, the first argument is ignored.
		function<int(int, int)> fun;
	};
	// Operators may only contain these characters, and variable names may not
	// use these characters.
	const string OP_CHARS = "()!*/%+-<=>&|^?";
	// Operators, listed in greedy parsing order. That is, if parsing a group of
	// operators you can go with the first match found here. Binary operators
	// should be matched any time the previous token was a variable or value:
	const vector<Op> BINARY_OPS = {
		{"**", 8, 2, [](int a, int b) -> int { return pow(a, b); }},
		{"<=", 4, 2, [](int a, int b) -> int { return a <= b; }},
		{">=", 4, 2, [](int a, int b) -> int { return a >= b; }},
		{"==", 3, 2, [](int a, int b) -> int { return a == b; }},
		{"!=", 3, 2, [](int a, int b) -> int { return a != b; }},
		{"&&", 2, 2, [](int a, int b) -> int { return a && b; }},
		{"||", 1, 2, [](int a, int b) -> int { return a || b; }},
		{ "*", 6, 2, [](int a, int b) -> int { return a * b; }},
		{ "/", 6, 2, [](int a, int b) -> int { return a / b; }},
		{ "%", 6, 2, [](int a, int b) -> int { return a % b; }},
		{ "+", 5, 2, [](int a, int b) -> int { return a + b; }},
		{ "-", 5, 2, [](int a, int b) -> int { return a - b; }},
		{ "<", 4, 2, [](int a, int b) -> int { return a < b; }},
		{ ">", 4, 2, [](int a, int b) -> int { return a > b; }}
	};
	// Unary operators should be matched whenever the previous token was not a
	// variable or value. (This includes the very beginning of the expression.)
	const vector<Op> UNARY_OPS = {
		{ "!", 7, 1, [](int, int b) -> int { return !b; }},
		{ "-", 7, 1, [](int, int b) -> int { return -b; }}
	};
	// Object representing an open parenthesis. Mark it with an empty function.
	// Its precedence is 0, i.e. it stays in the list until a right paren (which
	// conceptually has precedence 9) completes it.
	const Op PAREN_OP = {"(", 0, 1, function<int(int, int)>()};
	// Assignment operators. The variable's current value will be passed in a.
	const vector<Op> ASSIGN_OPS = {
		{"+=", 0, 2, [](int a, int b) -> int { return a + b; }},
		{"-=", 0, 2, [](int a, int b) -> int { return a - b; }},
		{"*=", 0, 2, [](int a, int b) -> int { return a * b; }},
		{"/=", 0, 2, [](int a, int b) -> int { return a / b; }},
		{"%=", 0, 2, [](int a, int b) -> int { return a % b; }},
		{"=", 0, 2, [](int, int b) -> int { return b; }},
	};
	// Object to return to indicate an invalid operator.
	const Op EMPTY_OP;
	
	// Pop the top operator of the given operator stack and apply it to the top
	// value(s) of the given value stack. Return false if there are not enough
	// values in the stack.
	void Apply(vector<int> &values, vector<const Op *> &ops);
	// Evaluate a variable or integer value.
	int Value(const char *it, const char *end);
	// Check if the given character is reserved for operators.
	bool IsOpChar(char c);
	// Find the first operator in the given set that matches the given string.
	const Op &FindOp(const char *it, const vector<Op> &ops);
}



// Evaluate an expression. (Note: it is assumed that this is the contents of
// an "if", so it cannot assign new values to any variables.)
int Variables::Eval(const string &line)
{
	const char *it = line.data();
	const char *end = it + line.size();
	
	// Output stack.
	vector<int> values;
	// Operator stack.
	vector<const Op *> ops;
	
	bool wasOp = true;
	while(it != end)
	{
		// Find the next token. Begin by skipping whitespace.
		if(*it <= ' ')
		{
			++it;
			continue;
		}
		
		if(*it == '(')
		{
			ops.push_back(&PAREN_OP);
			// The first thing inside the parentheses will either be a value or
			// a unary operator, never a binary operator.
			wasOp = true;
			++it;
		}
		else if(*it == ')')
		{
			// Apply all operators up until the most recent open parentheses.
			while(true)
			{
				if(ops.empty())
				{
					cerr << "Mismatched parentheses: " << line << endl;
					return 0;
				}
				if(!ops.back()->fun)
					break;
				Apply(values, ops);
			}
			// Discard the parenthesis operator.
			ops.pop_back();
			// An ending parenthesis causes the expression inside to be
			// evaluated, so it counts as a value, not an operator.
			wasOp = false;
			++it;
		}
		else if(IsOpChar(*it))
		{
			// If the previous token was an operator, this token can only be a
			// unary operator.
			const Op &op = FindOp(it, wasOp ? UNARY_OPS : BINARY_OPS);
			if(op.token.empty())
			{
				cerr << "Invalid expression: " << line << endl;
				return 0;
			}
			// Apply any operators in the stack whose precedence is greater than
			// this one. If this operator is binary, also apply operators that
			// have the same precedence as it.
			while(!ops.empty() && ops.back()->precedence >= op.precedence &&
					!(ops.back()->precedence == op.precedence && op.args == 1))
				Apply(values, ops);
			// Add this operator to the stack.
			ops.push_back(&op);
			wasOp = true;
			// Skip to the end of the operator.
			it += op.token.size();
		}
		else
		{
			// Find the end of this value by scanning forward to the next op
			// char, remembering the last non-whitespace character encountered.
			// The current char is not a space, so the token is not empty.
			const char *first = it;
			const char *last = it + 1;
			while(it != end && !IsOpChar(*it))
			{
				if(*it > ' ')
					last = it + 1;
				++it;
			}
			// Add the value of this token to the stack.
			values.push_back(Value(first, last));
			// Remember that the previous token was not an operator.
			wasOp = false;
			// The iterator can remain where it is, because it already points to
			// the beginning of the next token we want to process.
		}
	}
	while(!ops.empty())
		Apply(values, ops);
	return values.back();
}



// Evaluate a "set" command, changing the value of a single variable. If no
// assignment operator is found, an implied " = 1" is appended.
int Variables::Set(const string &line)
{
	// The line will be retrieved via Data::Value(), so it is guaranteed not to
	// have any whitespace at the beginning or end. But, it may be empty.
	if(line.empty())
		return 0;
	
	// Find the first value and the first operator.
	const char *it = line.data();
	const char *end = it + line.size();
	const char *first = it;
	const char *last = it;
	while(it != end && !IsOpChar(*it))
	{
		if(*it > ' ')
			last = it + 1;
		++it;
	}
	if(first == last)
	{
		cerr << "Set expression begins with operator: " << line << endl;
		return 0;
	}
	// Get the named variable.
	int &variable = variables[string(first, last)];
	// If no operator was found, this whole expression is just setting a
	// variable's value to "true," i.e. 1.
	if(it == end)
		return variable = 1;
	
	// Figure out what operator we're applying, and apply it.
	const Op &op = FindOp(it, ASSIGN_OPS);
	if(op.token.empty())
	{
		cerr << "Missing assignment operator: " << line << endl;
		return 0;
	}
	it += op.token.size();
	return variable = op.fun(variable, Eval(string(it, end)));
}



// Clear all variable definitions.
void Variables::Clear()
{
	variables.clear();
}



// Write the current variable values to a saved game file.
void Variables::Save(ostream &out)
{
	for(const pair<string, int> &it : variables)
	{
		if(it.second == 1)
			out << "set " << it.first << '\n';
		else if(it.second)
			out << "set " << it.first << " = " << it.second << '\n';
	}
}



namespace {
	// Helper function implementations:
	
	// Pop the top operator of the given operator stack and apply it to the top
	// value(s) of the given value stack.
	void Apply(vector<int> &values, vector<const Op *> &ops)
	{
		int b = values.back();
		values.pop_back();
		int a = 0;
		if(ops.back()->args == 2)
		{
			a = values.back();
			values.pop_back();
		}
		values.push_back(ops.back()->fun(a, b));
		ops.pop_back();
	}
	
	// Evaluate a variable or integer value.
	int Value(const char *start, const char *end)
	{
		int value = 0;
		// If any of the characters are not numerals, this is a variable name.
		for(const char *it = start; it != end; ++it)
		{
			if(*it >= '0' && *it <= '9')
				value = value * 10 + (*it - '0');
			else
				return variables[string(start, end)];
		}
		return value;
	}
	
	// Check if the given character is reserved for operators.
	bool IsOpChar(char c)
	{
		return (OP_CHARS.find(c) != string::npos);
	}
	
	// Find the first operator in the given set that matches the given string.
	const Op &FindOp(const char *it, const vector<Op> &ops)
	{
		// Note: a string is always null-terminated, so it's always safe to
		// dereference the end character. And, I know the "it" iterator does not
		// point to the end, so it[1] is always safe to dereference.
		for(const Op &op : ops)
			if(it[0] == op.token[0] && (op.token.size() == 1 || it[1] == op.token[1]))
				return op;
		
		return EMPTY_OP;
	}
}
