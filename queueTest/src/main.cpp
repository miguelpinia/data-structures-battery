#include <iostream>
#include "include/FAAArrayQueue.hpp"
#include "include/HPQueue.hpp"
#include "include/ThreadingQueue.hpp"
#include "include/MichaelScottQueue.hpp"
#include "include/SBQQueue.hpp"
// #include "include/SBQQueueHP.hpp"
#include "include/YMCQueue.hpp"
// #include "include/LCRQueue.hpp"
#include "include/LCRQ.hpp"

int main() {
    std::cout << "Hola Mundo desde queue test!" << std::endl;
    std::cout << "Comenzando inserción!" << std::endl;
    std::size_t cores = 8;
    int operations = 1'000'000 / cores;
    // Queue<std::string> queue {"-1"};
    scal_basket_queue::Queue<std::string> sbq_queue{cores};
    // LCRQueue<std::string> queue{8};
    faa_array::Queue<std::string> faaqueue{cores};
    ms_queue::Queue<std::string> msqueue{cores};
    lcrq_queue::Queue<std::string> lcrq_queue{cores};
    ymc_queue::Queue<std::string> ymc_queue{cores};

    std::cout << "\n\nEjecutando prueba FAA-Array-queue!" << std::endl;
    long time = queue_test_general(8, faaqueue, operations);
    std::cout << time << std::endl;
    std::cout << "\n\nEjecutando prueba MS-queue!" << std::endl;
    time = queue_test_general(8, msqueue, operations);
    std::cout << time << std::endl;
    std::cout << "\n\nEjecutando prueba LCRQ-queue!" << std::endl;
    time = queue_test_general(8, lcrq_queue, operations);
    std::cout << time << std::endl;
    std::cout << "\n\nEjecutando prueba YMC-queue!" << std::endl;
    time = queue_test_general(8, ymc_queue, operations);
    std::cout << time << std::endl;
    std::cout << "\n\nEjecutando prueba template SBQ-queue!" << std::endl;
    time = queue_test_general(8, sbq_queue, operations);
    std::cout << time << std::endl;
    return 0;
}
