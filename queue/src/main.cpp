#include <iostream>
#include "include/llictest.hpp"
#include "include/latency.hpp"
#include "include/basket_queue_test.hpp"

int main() {
    std::cout << "Hola mundo" << std::endl;
    // experiment_time_execution(40);
    // latency_experiment(40);
    queue_time_experiments(40);
}
