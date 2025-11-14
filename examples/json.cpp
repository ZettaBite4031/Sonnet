#include <iostream>

#include "sonnet/sonnet.hpp"

int main() {
    
    Sonnet::value v;
    v["name"] = "Zetta";
    v["age"] = (double)27;
    v["tags"][0] = "c++";
    v["tags"][1] = "json";

    std::cout << Sonnet::dump(v, {.pretty = true, .indent = 4}) << '\n';

    return 0;
}