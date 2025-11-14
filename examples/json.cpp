#include <print>
#include <fstream>

#include "sonnet/sonnet.hpp"

int main() {
    
    Sonnet::value v;
    v["name"] = "Zetta";
    v["age"] = (double)27;
    v["tags"][0] = "c++";
    v["tags"][1] = "json";
    

    std::println("{}", Sonnet::dump(v, {.pretty = true, .indent = 4}));

    Sonnet::value v2;
    auto& arr = v2.as_array();
    // arr.emplace_back(true);
    
    std::string s = Sonnet::dump(v2, {.pretty = true});
    std::println("\n\n{}", s);
    auto r = Sonnet::parse(s);
    std::println("{}", Sonnet::dump(*r, {}));

    std::ifstream ifs("/home/zbpm/code/C/Sonnet/.vscode/launch.json");
    if (!ifs) {
        std::println("Failed to open file");
        return -1;
    }

    auto file_r = Sonnet::parse(ifs);
    if (!file_r) {
        std::println("Parse error! -> {}", r.error().msg);
        return 1;
    }

    Sonnet::value file_v = std::move(file_r.value());
    println("{}", Sonnet::dump(file_v, {.pretty = true}));

    return 0;
}