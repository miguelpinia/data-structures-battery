#include <tuple>

std::tuple<long, int> latency_FAI(int cores);
std::tuple<long, int> latency_LLICCAS(int cores);
void latency_experiment(int iterations);
