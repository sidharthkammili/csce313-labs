#include <iostream>
#include <cmath>

struct Point {
    int x, y;

    Point () : x(), y() {} //default initialize to (0,0)
    Point (int _x, int _y) : x(_x), y(_y) {}
};

class Shape {
    int vertices; //# of vertices in polygon
    Point** points; //array of pointers to heap-allocated Point objects

public: //Shape::Shape(int) and double* Shape::area() was initially private, but made it public with this line
        //now, main() can create shape instances
    Shape (int _vertices) {
        vertices = _vertices;
        points = new Point*[vertices]; //allocate array of Point*
    }

    ~Shape () {
        //free each Point that was new'd, then free pointer array
        for (int i = 0; i < vertices; i++) {
            delete points[i];
        }
        delete[] points;
    }

    //copy input Point arrays into heap-allocated storage
    void addPoints (Point pts[]) {
        for (int i = 0; i < vertices; i++) {
            //memcpy(points[i], &pts[i%vertices], sizeof(Point));
            points[i] = new Point(pts[i]); //allocate new Point, copy value from pts[i]
        }
    }

    //Computes polygon area with Shoelace formula
    double area () { //was initially double*, changed to double because function must return value, not a pointer; was returning address of local area
        int temp = 0;
        for (int i = 0; i < vertices; i++) {
            // FIXME: there are two methods to access members of pointers
            //        use one to fix lhs and the other to fix rhs

            // wrap around to connect last vertiex with first using: (i + 1) % vertices
            int lhs = (*points[i]).x * (*points[(i+1) % vertices]).y; //deref + dot style
            int rhs = points[(i+1) % vertices]->x * points[i]->y; //arrow style
            temp += (lhs - rhs);
        }
        //Shoelace abs value, then divide by 2
        double area = std::abs(temp)/2.0;
        return area; //was initially &area, which wouldve return pointer to local; now will return area by value properly 
    }
};

int main () {
    // FIXME: create the following points using the three different methods
    //        of defining structs:
    
    //tri1, default constructor, then assign variables
    Point tri1;
    tri1.x = 0;
    tri1.y = 0;

    //tri2, constructor with arguments
    Point tri2(1,2);

    //tri3, initialized with braces
    Point tri3 = {2,0};

    // adding points to tri to build shape
    Point triPts[3] = {tri1, tri2, tri3};
    Shape* tri = new Shape(3);
    tri->addPoints(triPts); //initially dot, changed to arrow

    // FIXME: create the following points using your preferred struct
    //        definition:
    Point quad1(0, 0);
    Point quad2(0, 2);
    Point quad3(2, 2);
    Point quad4(2, 0);

    // adding points to quad to build shape
    Point quadPts[4] = {quad1, quad2, quad3, quad4};
    Shape* quad = new Shape(4);
    quad->addPoints(quadPts); //initially dot, changed to arrow

    // FIXME: print out area of tri and area of quad
    std::cout << "Triangle area: " << tri->area() << std:: endl;
    std::cout << "Quadrilateral area: " << quad->area() << std:: endl;

    //free the shapes that were new'd in main
    delete tri;
    delete quad;

}
