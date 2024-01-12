#include <iostream>
#include "include/HPQueue.hpp"
#include "include/ThreadingQueue.hpp"


int main() {
    std::cout << "Hola Mundo desde queue test!" << std::endl;
    std::cout << "Comenzando inserción!" << std::endl;
    Queue<std::string> q;
    std::cout << "Por insertar: 1" << std::endl;
    q.enqueue("1");
    std::cout << "Se ha insertado: 1" << std::endl;
    q.enqueue("2");
    std::cout << "Se ha insertado: 2" << std::endl;
    q.enqueue("3");
    std::cout << "Se ha insertado: 3" << std::endl;
    q.enqueue("4");
    std::cout << "Se ha insertado: 4" << std::endl;
    q.enqueue("5");
    std::cout << "Se ha insertado: 5" << std::endl;
    std::string ex = q.dequeue();
    std::cout << "Extraido: " << ex << std::endl;
    ex = q.dequeue();
    std::cout << "Extraido: " << ex << std::endl;
    ex = q.dequeue();
    std::cout << "Extraido: " << ex << std::endl;
    ex = q.dequeue();
    std::cout << "Extraido: " << ex << std::endl;
    ex = q.dequeue();
    std::cout << "Extraido: " << ex << std::endl;
    // ex = *q.dequeue();
    // std::cout << "Extraido: " << ex << std::endl;
    long time = queue_test(8);
    std::cout << time << std::endl;
    return 0;
}
