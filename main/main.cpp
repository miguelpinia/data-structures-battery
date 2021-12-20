#include <iostream>
#include "BasketQueue.hpp"

int main(int argc, char** argv) {
    CASCASBasketQueue* queue = new CASCASBasketQueue(10, 1);
    for(int i = 0; i < 10; i++) {
        queue->enqueue(i, 0);
    }
    int result;
    for(int i = 0; i < 10; i++) {
        result = queue->dequeue(0);
        std::cout << "salida: " << result << std::endl;
    }

}
