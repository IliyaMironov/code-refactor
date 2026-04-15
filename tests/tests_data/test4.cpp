#include <vector>
#include <string>

class Animal {
public:
    virtual void speak() {}
    ~Animal() {}
};

class Dog : public Animal {
public:
    void speak() {}

    void process() {
        std::vector<std::string> names = {"Rex", "Buddy"};
        for (const auto name : names) {
        }
    }
};
