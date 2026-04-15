#include <vector>
#include <map>
#include <string>
#include <set>

struct Point {
    int x, y;
};

void process() {
    std::vector<Point> points = {{1, 2}, {3, 4}};
    for (const auto& p : points) {
    }

    std::map<int, std::string> dict = {{1, "one"}, {2, "two"}};
    for (const auto& entry : dict) {
    }

    std::set<std::string> tags = {"a", "b"};
    for (const auto& tag : tags) {
    }

    std::vector<int> ids = {1, 2, 3};
    for (const auto id : ids) {
    }
}
