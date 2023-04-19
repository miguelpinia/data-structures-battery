#include <random>
#include <iterator>
#include <iostream>
#include <algorithm>
#include "include/kbasket.hpp"

void initializeFAI(std::atomic<int>* array, int size) {
    for (int i = 0; i < size; i++) {
        array[i] = BOTTOM;
    }
}

KBasketFAI::KBasketFAI() {}

KBasketFAI::KBasketFAI(int k) : size_k(k)
{
    A = new std::atomic<int>[size_k];
    for (int i = 0; i < size_k; i++) {
        A[i] = BOTTOM;
    }
}


void KBasketFAI::initializeDefault(int n)
{ // To perform a lazy loading after create a  object with default constructor
    A = new std::atomic<int>[n];
    size_k = n;
    for (int i = 0; i < size_k; i++) {
        A[i] = BOTTOM;
    }
}

KBasketFAI::~KBasketFAI() {
    delete [] A;
}

STATE_PUT KBasketFAI::put(int x)
{

    STATE_BASKET state;
    int puts;
    while(true) {
        state = STATE.load();
        puts = PUTS.load();
        if (state == CLOSED || puts >= size_k) {
            return FULL;
        } else {
            puts = PUTS++; // Equivalent to fetch_add(1) https://en.cppreference.com/w/cpp/atomic/atomic/operator_arith FAI
            if (puts >= size_k) {
                return FULL;
            } else if (A[puts].exchange(x) == BOTTOM) {// https://en.cppreference.com/w/cpp/atomic/atomic/exchange (swap)
                return OK;
            }
        }
    }
}

int KBasketFAI::take()
{
    STATE_BASKET state;
    int takes;
    while (true) {
        state = STATE.load();
        takes = TAKES.load();
        if (state == CLOSED or takes >= size_k) {
            return BASKET_CLOSED;
        } else {
            takes = TAKES++;
            if (takes >= size_k) {
                STATE.store(CLOSED);
                return BASKET_CLOSED;
            } else {
                int x = A[takes].exchange(TOP);
                if (x != BOTTOM) return x;
            }
        }
    }
}

NBasketCAS::NBasketCAS() {}

NBasketCAS::NBasketCAS(int n) : size_n(n)
{
    A = new std::atomic<int>[size_n];
    for (int i = 0; i < size_n; i++) {
        A[i] = BOTTOM;
    }
}

NBasketCAS::~NBasketCAS() {
    delete [] A;
}

void NBasketCAS::initializeDefault(int number_processes) { // To perform a lazy loading after create a  object with default constructor
    A = new std::atomic<int>[number_processes];
    size_n = number_processes;
    for (int i = 0; i < size_n; i++) {
        A[i] = BOTTOM;
    }
}

int NBasketCAS::compete(int pos)
{
    int x = A[pos].load();
     if (x == TOP) {
        return TOP;
    } else if (A[pos].compare_exchange_strong(x, TOP)) { // https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
        return x;
    }
    return BOTTOM;
}

STATE_PUT NBasketCAS::put(int x, int process)
{
    int bottom = BOTTOM; // can't compare const int& respect to the value in atomic<int>
    if (STATE.load() == CLOSED) {
        return FULL;
    } else if (A[process].load() == BOTTOM) {
        if (A[process].compare_exchange_strong(bottom, x)) {
            return OK;
        }
    }
    return FULL;
}

// bool inTakes(int process, std::unordered_set<int> set) {
//     std::unordered_set<int>::const_iterator got = set.find(process);
//     return got != set.end();
// }

// int randomValInSet(std::unordered_set<int> set) { //  TODO: Add tests (MAPA 2021-12-19)
//     int size = set.size();
//     std::default_random_engine generator;
//     std::uniform_int_distribution<int> distribution(0,size);
//     std::unordered_set<int> :: iterator itr;
//     int pos = distribution(generator);
//     auto it = set.begin();
//     std::advance(it, pos); //  TODO: Delete object once it's hit (MAPA 2021-12-19)
//     return *it;
// }

// int KBasketCAS::take(int process)
// {
//     int pos;
//     while(true) {
//         if (STATE.load() == CLOSED) {
//             return BASKET_CLOSED;
//         } else {
//             if (inTakes(process, takes_p)) {
//                 pos = process;
//             } else {
//                 pos = randomValInSet(takes_p);
//             }
//             takes_p.erase(pos);
//             if (takes_p.empty()) {
//                 STATE.store(CLOSED);
//             }
//             int x = compete(pos);
//             if (x != TOP && x!= BOTTOM) {
//                 return x;
//             } else if (x == BOTTOM) {
//                 x = compete(pos);
//                 if (x != TOP && x != BOTTOM) {
//                     return x;
//                 }
//             }
//         }
//     }
// }


int NBasketCAS::take(int process)
{
    int pos = process;
    while(true) {
        if (STATE.load() == CLOSED) {
            return BASKET_CLOSED;
        } else {
            if (pos == process + size_n ) {
                STATE.store(CLOSED);
                continue;
            }
            int x = compete(pos % size_n);
            if (x != TOP && x!= BOTTOM) {
                return x;
            } else if (x == BOTTOM) {
                x = compete(pos % size_n);
                if (x != TOP && x != BOTTOM) {
                    return x;
                }
            }
            pos++;
        }
    }
}
