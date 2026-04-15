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
    std::string name() const override { return "impl"; }
    int count() const override { return 42; }
    void reset() override {}
};
