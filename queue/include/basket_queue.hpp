#ifndef BASKETQUEUE_HPP
#define BASKETQUEUE_HPP

#include "kbasket.hpp"
#include "llic.hpp"

// - CAS
// - RW padding 16
// - Rw no padding
// - SQRT
// - Fetch&Inc
// - Grouped 16 bytes

// Template

template<class T>
class CASQueue {
private:
    int capacity;
    int numProcesses;
    NBasketCAS *A;
    T HEAD;
    T TAIL;
public:
    CASQueue(int capacity, int numProcesses);
    void enqueue(int x, int process);
    int dequeue(int process);
    ~CASQueue();
};

template<class T>
CASQueue<T>::CASQueue(int capacity, int numProcesses) : capacity(capacity),
                                                        numProcesses(numProcesses) {
    A = new NBasketCAS[capacity];
    for (int i = 0; i < capacity; ++i) {
        A[i].initializeDefault(numProcesses);
    }
    HEAD.initializeDefault(numProcesses);
    TAIL.initializeDefault(numProcesses);
}

template<class T>
void CASQueue<T>::enqueue(int x, int process) {
    int tail;
    while (true) {
        tail = TAIL.LL();
        if (A[tail].put(x, process) == OK) {
            TAIL.IC(tail, process);
            return;
        }
        TAIL.IC(tail, process);
    }
}

template<class T>
int CASQueue<T>::dequeue(int process) {
    int head = HEAD.LL();
    int tail = TAIL.LL();
    int x;
    while (true) {
        if (head < tail) {
            x = A[head].take(process);
            if (x != BASKET_CLOSED) {
                return x;
            }
            HEAD.IC(head, process);
        }
        auto hhead = HEAD.LL();
        auto ttail = TAIL.LL();
        if (hhead == head && ttail == tail) {
            return EMPTY;
        }
        head = hhead;
        tail = ttail;
    }
}

template<class T>
CASQueue<T>::~CASQueue() {
    delete[] A;
}

template<class T>
class FAIQueue {
private:
    int capacity;
    int k;
    int numProcesses;
    KBasketFAI *A;
    T HEAD;
    T TAIL;
public:
    FAIQueue(int capacity, int k, int numProcesses);
    void enqueue(int x, int process);
    int dequeue(int process);
    ~FAIQueue();
};

template<class T>
FAIQueue<T>::FAIQueue(int capacity, int k, int numProcesses) : capacity(capacity), k(k), numProcesses(numProcesses) {
    A = new KBasketFAI[capacity];
    for (int i = 0; i < capacity; ++i) {
        A[i].initializeDefault(k);
    }
    HEAD.initializeDefault(numProcesses);
    TAIL.initializeDefault(numProcesses);
}

template<class T>
void FAIQueue<T>::enqueue(int x, int process) {
    int tail;
    while (true) {
        tail = TAIL.LL();
        if (A[tail].put(x) == OK) {
            TAIL.IC(tail, process);
            return;
        }
        TAIL.IC(tail, process);
    }
}

template<class T>
int FAIQueue<T>::dequeue(int process) {
    int head = HEAD.LL();
    int tail = TAIL.LL();
    int x;
    while (true) {
        if (head < tail) {
            x = A[head].take();
            if (x != BASKET_CLOSED) {
                return x;
            }
            HEAD.IC(head, process);
        }
        auto hhead = HEAD.LL();
        auto ttail = TAIL.LL();
        if (hhead == head && ttail == tail) {
            return EMPTY;
        }
        head = hhead;
        tail = ttail;
    }
}

template<class T>
FAIQueue<T>::~FAIQueue() {
    delete[] A;
}

template<class T>
class CASGQueue {
private:
    int capacity;
    int numProcesses;
    int head_idx_max;
    int tail_idx_max;
    T HEAD;
    T TAIL;
    NBasketCAS *A;
public:
    CASGQueue(int capacity, int numProcesses);
    void enqueue(int x, int process);
    int dequeue(int process);
    ~CASGQueue();
};

template<class T>
CASGQueue<T>::CASGQueue(int capacity, int numProcesses) : capacity(capacity),
                                                        numProcesses(numProcesses) {
    A = new NBasketCAS[capacity];
    for (int i = 0; i < capacity; ++i) {
        A[i].initializeDefault(numProcesses);
    }
    HEAD.initializeDefault(numProcesses);
    TAIL.initializeDefault(numProcesses);
    head_idx_max = 0;
    tail_idx_max = 0;
}

template<class T>
void CASGQueue<T>::enqueue(int x, int process) {
    int tail;
    while (true) {
        tail = TAIL.LL();
        if (A[tail].put(x, process) == OK) {
            TAIL.IC(tail, tail_idx_max, process);
            return;
        }
        TAIL.IC(tail, process);
    }
}

template<class T>
int CASGQueue<T>::dequeue(int process) {
    int head = HEAD.LL();
    int tail = TAIL.LL();
    int x;
    while (true) {
        if (head < tail) {
            x = A[head].take(process);
            if (x != BASKET_CLOSED) {
                return x;
            }
            HEAD.IC(head, process);
        }
        auto hhead = HEAD.LL();
        auto ttail = TAIL.LL();
        if (hhead == head && ttail == tail) {
            return EMPTY;
        }
        head = hhead;
        tail = ttail;
    }
}

template<class T>
CASGQueue<T>::~CASGQueue() {
    delete[] A;
}


template<class T>
class FAIGQueue {
private:
    int capacity;
    int k;
    int numProcesses;
    KBasketFAI *A;
    T HEAD;
    T TAIL;
public:
    FAIGQueue(int capacity, int k, int numProcesses);
    void enqueue(int x, int process);
    int dequeue(int process);
    ~FAIGQueue();
};

template<class T>
FAIGQueue<T>::FAIGQueue(int capacity, int k, int numProcesses) : capacity(capacity), k(k), numProcesses(numProcesses) {
    A = new KBasketFAI[capacity];
    for (int i = 0; i < capacity; ++i) {
        A[i].initializeDefault(k);
    }
    HEAD.initializeDefault(numProcesses);
    TAIL.initializeDefault(numProcesses);
}

template<class T>
void FAIGQueue<T>::enqueue(int x, int process) {
    int tail;
    int idx_max_p = 0;
    while (true) {
        tail = TAIL.LL(idx_max_p);
        if (A[tail].put(x) == OK) {
            TAIL.IC(tail, idx_max_p, process);
            return;
        }
        TAIL.IC(tail, idx_max_p, process);
    }
}

template<class T>
int FAIGQueue<T>::dequeue(int process) {
    int head = HEAD.LL();
    int tail = TAIL.LL();
    int x;
    int idx_max_p = 0;
    while (true) {
        if (head < tail) {
            x = A[head].take();
            if (x != BASKET_CLOSED) {
                return x;
            }
            HEAD.IC(head, idx_max_p, process);
        }
        auto hhead = HEAD.LL(idx_max_p);
        auto ttail = TAIL.LL(idx_max_p);
        if (hhead == head && ttail == tail) {
            return EMPTY;
        }
        head = hhead;
        tail = ttail;
    }
}

template<class T>
FAIGQueue<T>::~FAIGQueue() {
    delete[] A;
}

#endif
