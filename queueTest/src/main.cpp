#include <iostream>
#include "include/HPQueue.hpp"
#include "include/ThreadingQueue.hpp"
#include "include/MichaelScottQueue.hpp"


int main() {
    std::cout << "Hola Mundo desde queue test!" << std::endl;
    std::cout << "Comenzando inserción!" << std::endl;
    Queue<std::string> queue {"-1"};
    FAAArrayQueue<std::string> faaqueue;
    MichaelScottQueue<std::string> msqueue;
    std::cout << "\n\nEjecutando prueba template-queue!" << std::endl;
    long time = queue_test_general(8, queue);
    std::cout << time << std::endl;
    std::cout << "\n\nEjecutando prueba template FAA-Array-queue!" << std::endl;
    time = queue_test_general(8, faaqueue);
    std::cout << time << std::endl;
    std::cout << "\n\nEjecutando prueba template MS-queue!" << std::endl;
    time = queue_test_general(8, msqueue);
    std::cout << time << std::endl;
    return 0;
}
