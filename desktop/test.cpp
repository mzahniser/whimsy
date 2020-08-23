// g++ --std=c++17 -o test test.cpp Edge.cpp Ring.cpp Point.cpp
#include "Ring.h"
#include "Point.h"

#include <iostream>

using namespace std;



int main(int argc, char *argv[])
{
	Ring ring;
	ring.emplace_back(-10, 10);
	ring.emplace_back(10, 10);
	ring.emplace_back(20, 0);
	ring.emplace_back(10, -10);
	ring.emplace_back(-10, -10);
	ring.emplace_back(-20, 0);
	
	cout << ring.IsHole() << endl;
	cout << ring.Contains(Point(-15, 5)) << endl;
	cout << ring.Contains(Point(0, 5)) << endl;
	cout << ring.Contains(Point(15, 5)) << endl;
	
	return 0;
}
