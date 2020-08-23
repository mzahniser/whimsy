// g++ --std=c++17 -o svg svg.cpp
// Parse an SVG path spec and output vertex coordinates.

#include <iostream>
#include <string>

using namespace std;

int ReadInt(string::const_iterator &it, const string::const_iterator &end)
{
	if(*it == ' ')
		++it;
	bool negative = (*it == '-');
	if(negative)
		++it;
	int value = 0;
	while(*it >= '0' && *it <= '9')
		value = (value * 10) + (*it++ - '0');
	return negative ? -value : value;
}



int main(int argc, char *argv[])
{
	string line;
	while(getline(cin, line))
	{
		const string PREFIX = "<path d=\"";
		size_t pos = line.find(PREFIX);
		if(pos == string::npos)
			continue;

		char mode = 'l';
		int x = 0, y = 0;
		
		cout << "mask";

		string::const_iterator it = line.begin() + pos + PREFIX.size();
		while(it != line.end())
		{
			if(*it >= 'a' && *it <= 'z')
				mode = *it++;
			else
			{
				if(mode == 'v')
					y += ReadInt(it, line.end());
				else if(mode == 'h')
					x += ReadInt(it, line.end());
				else if(mode == 'l' || mode == 'm')
				{
					x += ReadInt(it, line.end());
					y += ReadInt(it, line.end());
				}
				else
					break;
				cout << ' ' << x << ',' << y;
			}
		}
		cout << endl << endl;
	}
	
	return 0;
}
