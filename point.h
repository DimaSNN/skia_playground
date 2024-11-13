#ifndef POINT_H
#define POINT_H

#include <ostream>
#include <cmath>

namespace hmos {

struct Ranges{ int minX=0, minY=0, maxX=0, maxY=0; };
struct Point {
public:
    using speed_type_t = float; 
    /* The x and y variable are made PUBLIC because they are only limited by
     * the float data type constraints.
     */
    float x, y;

    /* The constructor will accept float and int data types or none at all.
     * Two parameters sets to x,y respectively; one parameter sets both
     * x and y to the same value.  An empty constructor sets x,y = 0.0
     *
     * There is no destructor since the Point is essential just two floats
     */
    Point() : x(0.0), y(0.0) {};
    Point(float x, float y) : x(x), y(y) {};
    explicit Point(float val) : x(val), y(val) {};
    Point(int x, int y) : x(static_cast<float>(x)), y(static_cast<float>(y)) {};
    explicit Point(int val) : x(static_cast<float>(val)), y(static_cast<float>(val)) {};


    /* Returns the slope of two Points.
     * Formula for finding the slope(m) between
     * two coordinate Points is: (y2-y1) / (x2-x1)
     */
    float slope(Point p)
    {
        return ((p.y - y) / (p.x - x));
    }


    /* Returns the distance between two Points.
     * Formula for finding the distance(d) between
     * two Points: square_root of [ (x2-x1)^2 + (y2-y1)^2 ]
     */
    float distance(Point p) const
    {
        float d = ((p.x - x) * (p.x - x)) + ((p.y - y) * (p.y - y));
        return std::sqrt(d);
    }


    /* Increments the distance between two Points.
     * Formula for finding a new Point between this Point
     * and Point 'p' given a distance to increment:
     * New(x) = x + (dist / square root of [ (m^2 + 1) ])
     * New(y) = y + (m*dist / square root of [ (m^2 + 1) ])
     */
    Point increment(Point p, float distance)
    {
        float m = slope(p);

        float newX = (distance / (sqrt((m * m) + 1)));
        newX = ((x < p.x) ? (x + newX) : (x - newX));

        float newY = ((m * distance) / (sqrt((m * m) + 1)));
        if (m >= 0.0) newY = ((y < p.y) ? (y + newY) : (y - newY));
        if (m < 0.0) newY = ((y < p.y) ? (y - newY) : (y + newY));

        return Point(newX, newY);
    }


    //Returns the magnitude of a Point.
    //Distance from Point to (0,0)
    float magnitude()
    {
        return distance(Point(0, 0));
    }


    //Returns a normalized Point
    Point normalize()
    {
        return Point(x / magnitude(), y / magnitude());
    }


    //Operators:

    //Addition
    Point operator+=(Point pnt) { (*this).x += pnt.x; (*this).y += pnt.y; return (*this); }
    Point operator+=(float num) { (*this).x += num; (*this).y += num; return (*this); }
    Point operator+(Point pnt) { return Point((*this).x + pnt.x, (*this).y + pnt.y); }
    Point operator+(float num) { return Point((*this).x + num, (*this).y + num); }

    //Subtraction
    Point operator-=(Point pnt) { (*this).x -= pnt.x; (*this).y -= pnt.y; return (*this); }
    Point operator-=(float num) { (*this).x -= num; (*this).y -= num; return (*this); }
    Point operator-(Point pnt) { return Point((*this).x - pnt.x, (*this).y - pnt.y); }
    Point operator-(float num) { return Point((*this).x - num, (*this).y - num); }

    //Multiplication
    Point operator*=(Point pnt) { (*this).x *= pnt.x; (*this).y *= pnt.y; return (*this); }
    Point operator*=(float num) { (*this).x *= num; (*this).y *= num; return (*this); }
    Point operator*(Point pnt) { return Point((*this).x * pnt.x, (*this).y * pnt.y); }
    Point operator*(float num) { return Point((*this).x * num, (*this).y * num); }

    //Division
    Point operator/=(Point pnt) { (*this).x /= pnt.x; (*this).y /= pnt.y; return (*this); }
    Point operator/=(float num) { (*this).x /= num; (*this).y /= num; return (*this); }
    Point operator/(Point pnt) { return Point((*this).x / pnt.x, (*this).y / pnt.y); }
    Point operator/(float num) { return Point((*this).x / num, (*this).y / num); }

    //Equal (Assignment)
    Point operator=(Point pnt) { (*this).x = pnt.x; (*this).y = pnt.y; return (*this); }
    Point operator=(float num) { (*this).x = num; (*this).y = num; return (*this); }

    bool operator==(const hmos::Point& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const hmos::Point& other) const
    {
        return !this->operator==(other);
    }
};


//Comparative operators:
//bool operator==(const hmos::Point& a, const hmos::Point& b);
//bool operator!=(const hmos::Point& a, const hmos::Point& b);


//Other operators:
//hmos::Point operator+(float num, const hmos::Point& pnt);
//hmos::Point operator*(float num, const hmos::Point& pnt);


//Allows the display of the Points in a 'coordinate' format (x,y) to the output stream
std::ostream& operator<<(std::ostream& os, const hmos::Point& a);


}



#endif 