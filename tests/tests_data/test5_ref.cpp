#include <vector>
#include <string>

class Base {
public:
    virtual void foo() {}
    virtual ~Base() {}
};

class Derived : public Base {
public:
    void foo() override {}
};

void bar() {
    std::vector<std::string> v = {"a", "b"};
    for (const auto& s : v) {
    }
    std::vector<int> ints = {1, 2};
    for (const int x : ints) {
    }
}
