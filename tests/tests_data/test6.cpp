#include <string>

class GrandBase {
public:
    virtual void action() {}
    virtual void info() {}
    virtual ~GrandBase() {}
};

class Middle : public GrandBase {
public:
    void action() {}
    void info() {}
};

class Leaf : public Middle {
public:
    void action() {}
    void info() {}
};
