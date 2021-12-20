#include "BasketQueue.hpp"

#define UNUSED(expr) (void)(expr)

//////////////////////////
// First Version        //
// LLIC CAS, Basket CAS //
//////////////////////////

CASCASBasketQueue::CASCASBasketQueue(int capacity, int k): capacity(capacity), k(k) {
    A = new KBasketCAS[capacity];
    for(int i = 0; i < capacity; i++) {
        A[i].initializeDefault(k);
    }
}

void CASCASBasketQueue::enqueue(int x, int process)
{
    int tail;
    while (true) {
        tail = TAIL.LL();
        if (A[tail].put(x, process) == OK) {
            TAIL.IC(tail);
            return;
        }
        TAIL.IC(tail);
    }

}

int CASCASBasketQueue::dequeue(int process)
{
    int head = HEAD.LL();
    int tail = TAIL.LL();
    int x;
    while (true) {
        if (head < tail) {
            x = A[head].take(process);
            if (x != BASKET_CLOSED) {
                return x;
            }
            HEAD.IC(head);
        }
        int hhead = HEAD.LL();
        int ttail = TAIL.LL();
        if (hhead == head && ttail == tail) {
            return EMPTY;
        }
        head = hhead;
        tail = ttail;
    }
}

void CASCASBasketQueue::enqueue(int x)
{
    UNUSED(x);
    throw NotImplementedException();
}

int CASCASBasketQueue::dequeue()
{
    throw NotImplementedException();
}

//////////////////////////
// Second version       //
// LLIC CAS, BASKET FAI //
//////////////////////////

CASFAIBasketQueue::CASFAIBasketQueue(int capacity, int k) : capacity(capacity), k(k)
{
    A = new KBasketFAI[capacity];
    for (int i = 0; i < capacity; i++) {
        A[i].initializeDefault(k);
    }
}

void CASFAIBasketQueue::enqueue(int x)
{
    int tail;
    while (true) {
        tail = TAIL.LL();
        if (A[tail].put(x) == OK) {
            TAIL.IC(tail);
            return;
        }
        TAIL.IC(tail);
    }

}

int CASFAIBasketQueue::dequeue()
{
    int head = HEAD.LL();
    int tail = TAIL.LL();
    int x;
    while (true) {
        if (head < tail) {
            x = A[head].take();
            if (x != BASKET_CLOSED) {
                return x;
            }
            HEAD.IC(head);
        }
        int hhead = HEAD.LL();
        int ttail = TAIL.LL();
        if (hhead == head && ttail == tail) {
            return EMPTY;
        }
        head = hhead;
        tail = ttail;
    }
}

void CASFAIBasketQueue::enqueue(int x, int process) {
    UNUSED(x);
    UNUSED(process);
    throw NotImplementedException();
}

int CASFAIBasketQueue::dequeue(int process) {
    UNUSED(process);
    throw NotImplementedException();
}

/////////////////////////
// Third version       //
// LLIC RW, BASKET CAS //
/////////////////////////

RWCASBasketQueue::RWCASBasketQueue(int capacity, int k, int total_processes):
    capacity(capacity), k(k), total_processes(total_processes)
{
    A = new KBasketCAS[capacity];
    for(int i = 0; i < capacity; i++) {
        A[i].initializeDefault(k);
    }
    HEAD.initializeDefault(total_processes);
    TAIL.initializeDefault(total_processes);
}

void RWCASBasketQueue::enqueue(int x, int process)
{
    int tail;
    while (true) {
        tail = TAIL.LL(process);
        if (A[tail].put(x, process) == OK) {
            TAIL.IC(tail, process);
            return;
        }
        TAIL.IC(tail, process);
    }

}

int RWCASBasketQueue::dequeue(int process)
{
    int head = HEAD.LL(process);
    int tail = TAIL.LL(process);
    int x;
    while (true) {
        if (head < tail) {
            x = A[head].take(process);
            if (x != BASKET_CLOSED) {
                return x;
            }
            HEAD.IC(head, process);
        }
        int hhead = HEAD.LL(process);
        int ttail = TAIL.LL(process);
        if (hhead == head && ttail == tail) {
            return EMPTY;
        }
        head = hhead;
        tail = ttail;
    }
}

void RWCASBasketQueue::enqueue(int x) {
    UNUSED(x);
    throw NotImplementedException();
}

int RWCASBasketQueue::dequeue() {
    throw NotImplementedException();
}

/////////////////////////
// Four Version        //
// LLIC RW, BASKET FAI //
/////////////////////////

RWFAIBasketQueue::RWFAIBasketQueue(int capacity, int k, int total_processes)
    : capacity(capacity), k(k), total_processes(total_processes)
{
    A = new KBasketFAI[capacity];
    for (int i = 0; i < capacity; i++) {
        A[i].initializeDefault(k);
    }
    HEAD.initializeDefault(total_processes);
    TAIL.initializeDefault(total_processes);
}


void RWFAIBasketQueue::enqueue(int x, int process)
{
    int tail;
    while (true) {
        tail = TAIL.LL(process);
        if (A[tail].put(x) == OK) {
            TAIL.IC(tail, process);
            return;
        }
        TAIL.IC(tail, process);
    }

}

int RWFAIBasketQueue::dequeue(int process)
{
    int head = HEAD.LL(process);
    int tail = TAIL.LL(process);
    int x;
    while (true) {
        if (head < tail) {
            x = A[head].take();
            if (x != BASKET_CLOSED) {
                return x;
            }
            HEAD.IC(head, process);
        }
        int hhead = HEAD.LL(process);
        int ttail = TAIL.LL(process);
        if (hhead == head && ttail == tail) {
            return EMPTY;
        }
        head = hhead;
        tail = ttail;
    }
}

void RWFAIBasketQueue::enqueue(int x) {
    UNUSED(x);
    throw NotImplementedException();
}

int RWFAIBasketQueue::dequeue() {
    throw NotImplementedException();
}
