#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include "utils.hpp"


void print_time(std::clock_t time, double duration, int value) {
    // Printing times
    // https://pythonspeed.com/articles/blocking-cpu-or-io/
    std::cout << std::fixed << std::setprecision(2) << "CPU time used: "
              << 1000.0 * (time) / CLOCKS_PER_SEC << " ms\n"
              << "Wall clock time passed: " << duration << " ms\n"
              << value << " max value stored\n";
}
