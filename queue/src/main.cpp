#include <iostream>
#include "include/llictest.hpp"
#include "include/latency.hpp"
#include "include/basket_queue_test.hpp"
#include "include/Experiments.hpp"

int main() {
    std::cout << "Hola mundo" << std::endl;
    // experiment_time_execution(40);
    // latency_experiment(40);
    // queue_time_experiments(10);
    exp_llic::experiments();
}
