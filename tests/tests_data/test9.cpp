#include <string>

class Interface {
public:
    virtual std::string name() const = 0;
    virtual int count() const { return 0; }
    virtual void reset() {}
    virtual ~Interface() {}
};

class Impl : public Interface {
public:
    std::string name() const { return "impl"; }
    int count() const { return 42; }
    void reset() {}
};
