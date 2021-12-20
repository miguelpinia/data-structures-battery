#include "KBasket.hpp"
#include "LLIC.hpp"

// LLIC CAS, Basket CAS
class CASCASBasketQueue {
private:
    int capacity;
    int k;
    KBasketCAS* A;
    LLICCAS HEAD;
    LLICCAS TAIL;
public:
    CASCASBasketQueue(int capacity, int k);
    void enqueue(int x);
    int dequeue();
    void enqueue(int x, int process);
    int dequeue(int process);
};

// LLIC CAS, BASKET RW

class CASFAIBasketQueue {
private:
    int capacity;
    int k;
    KBasketFAI* A;
    LLICCAS HEAD;
    LLICCAS TAIL;
public:
    CASFAIBasketQueue(int capacity, int k);
    void enqueue(int x);
    int dequeue();
    void enqueue(int x, int process);
    int dequeue(int process);
};

// LLIC RW, Basket CAS

class RWCASBasketQueue {
private:
    int capacity;
    int k;
    int total_processes;
    KBasketCAS* A;
    LLICRW HEAD;
    LLICRW TAIL;
public:
    /**
     * capacity: Size of queue, k: size of kbasket, total_processes: total of processes to use with LLICRW
     */
    RWCASBasketQueue(int capacity, int k, int total_processes);
    void enqueue(int x);
    int dequeue();
    void enqueue(int x, int process);
    int dequeue(int process);
};

// LLIC RW, Basket RW
class RWFAIBasketQueue {
private:
    int capacity;
    int k;
    int total_processes;
    KBasketFAI* A;
    LLICRW HEAD;
    LLICRW TAIL;
public:
    RWFAIBasketQueue(int capacity, int k, int total_processes);
    void enqueue(int x);
    int dequeue();
    void enqueue(int x, int process);
    int dequeue(int process);
};
