/* export
Copyright 2020 Michael Zahniser

Program to merge data files into a single file, for use in the web app.
*/

#include <fstream>
#include <iostream>
#include <string>

using namespace std;



void Export(const string &path)
{
	ifstream in(path);
	
	string line;
	while(getline(in, line))
	{
		static const string INCLUDE = "include ";
		if(line.rfind(INCLUDE, 0) == 0)
		{
			cout << '\n';
			Export(line.substr(INCLUDE.length()));
			cout << '\n';
		}
		else
			cout << line << '\n';
	}
}



int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		cerr << "Usage: $ ./export <file>" << endl;
		return 1;
	}
	Export(argv[1]);
	
	return 0;
}
