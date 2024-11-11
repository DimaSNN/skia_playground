#include "point.h"

namespace hmos {

    //Comparative operators:
    //bool operator==(const hmos::Point& a, const hmos::Point& b) { return a.x == b.x && a.y == b.y; }
    //bool operator!=(const hmos::Point& a, const hmos::Point& b) { return !(a == b); }


    //Other operators:
    //hmos::Point operator+(float num, const hmos::Point& pnt) { return (pnt + num); }
    //hmos::Point operator*(float num, const hmos::Point& pnt) { return (pnt * num); }


    //Allows the display of the hmos::Points in a 'coordinate' format (x,y) to the output stream
    std::ostream& operator<<(std::ostream& os, const hmos::Point& a)
    {
        os << "(" << a.x << "," << a.y << ")";
        return os;
    }

}