#include <string>
#include <vector>

class Shape {
public:
    virtual void draw() = 0;
    virtual double area() const = 0;
    virtual ~Shape() {}
};

class Circle : public Shape {
public:
    void draw() {}
    double area() const { return 3.14; }
};

class Square : public Shape {
public:
    void draw() {}
    double area() const { return 4.0; }
};

class ColoredCircle : public Circle {
public:
    void draw() {}
};
