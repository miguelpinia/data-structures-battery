#include <iostream>
#include "include/llictest.hpp"
#include "include/latency.hpp"

int main() {
    std::cout << "Hola mundo" << std::endl;
    experiment_time_execution(5);
    latency_experiment(5);
}
