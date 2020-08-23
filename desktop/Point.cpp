/* Point.cpp
Copyright 2020 Michael Zahniser
*/

#include "Point.h"

#include <cmath>

using namespace std;



Point::Point(int x, int y)
	: x(x), y(y)
{
}



// Check if this point is something other than (0, 0).
Point::operator bool() const
{
	return (x || y);
}



bool Point::operator!() const
{
	return !(x || y);
}



bool Point::operator==(Point p) const
{
	return (x == p.x && y == p.y);
}



bool Point::operator!=(Point p) const
{
	return (x != p.x || y != p.y);
}



Point Point::operator+(Point p) const
{
	return Point(x + p.x, y + p.y);
}



Point &Point::operator+=(Point p)
{
	x += p.x;
	y += p.y;
	return *this;
}



Point Point::operator-(Point p) const
{
	return Point(x - p.x, y - p.y);
}



Point &Point::operator-=(Point p)
{
	x -= p.x;
	y -= p.y;
	return *this;
}



Point Point::operator-() const
{
	return Point(-x, -y);
}



Point Point::operator*(int s) const
{
	return Point(x * s, y * s);
}



Point &Point::operator*=(int s)
{
	x *= s;
	y *= s;
	return *this;
}



Point operator*(int s, Point p)
{
	return Point(p.x * s, p.y * s);
}



Point Point::operator/(int s) const
{
	return Point(x / s, y / s);
}



Point &Point::operator/=(int s)
{
	x /= s;
	y /= s;
	return *this;
}



int &Point::X()
{
	return x;
}



int Point::X() const
{
	return x;
}



int &Point::Y()
{
	return y;
}



int Point::Y() const
{
	return y;
}



int Point::Dot(Point p) const
{
	return x * p.x + y * p.y;
}



int Point::Cross(Point p) const
{
	return x * p.y - y * p.x;
}



float Point::Length() const
{
	return sqrt(static_cast<float>(LengthSquared()));
}



int Point::LengthSquared() const
{
	return Dot(*this);
}



float Point::Distance(Point p) const
{
	return (p - *this).Length();
}



int Point::DistanceSquared(Point p) const
{
	return (p - *this).LengthSquared();
}
