#include <iostream>

#include "memory/hazptr_domain.hpp"

int main(int argc, char** argv) {
    std::cout << "Hello, world" << std::endl;


    [[maybe_unused]] zoro::hazptr_domain domain;

    return 0;
}