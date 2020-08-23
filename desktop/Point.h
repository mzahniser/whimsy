/* Point.h
Copyright 2020 Michael Zahniser
*/

#ifndef POINT_H_
#define POINT_H_

using namespace std;



class Point {
public:
	Point(int x = 0, int y = 0);
	
	// Check if this point is something other than (0, 0).
	operator bool() const;
	bool operator!() const;
	
	bool operator==(Point p) const;
	bool operator!=(Point p) const;
	
	Point operator+(Point p) const;
	Point &operator+=(Point p);
	Point operator-(Point p) const;
	Point &operator-=(Point p);
	
	Point operator-() const;
	
	Point operator*(int s) const;
	Point &operator*=(int s);
	friend Point operator*(int s, Point p);
	Point operator/(int s) const;
	Point &operator/=(int s);
	
	// Allow setting X and Y directly on non-const Points.
	int &X();
	int X() const;
	int &Y();
	int Y() const;
	
	int Dot(Point p) const;
	int Cross(Point p) const;
	
	float Length() const;
	int LengthSquared() const;
	float Distance(Point p) const;
	int DistanceSquared(Point p) const;
	
	
private:
	int x;
	int y;
};



#endif
