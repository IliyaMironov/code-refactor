#include <string>

class GrandBase {
public:
    virtual void action() {}
    virtual void info() {}
    virtual ~GrandBase() {}
};

class Middle : public GrandBase {
public:
    void action() override {}
    void info() override {}
};

class Leaf : public Middle {
public:
    void action() override {}
    void info() override {}
};
