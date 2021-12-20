#include <random>
#include <iterator>
#include <iostream>
#include <algorithm>
#include "KBasket.hpp"

void initializeFAI(std::atomic_int* array, int size) {
    for (int i = 0; i < size; i++) {
        array[i] = BOTTOM;
    }
}

KBasketFAI::KBasketFAI() {}

KBasketFAI::KBasketFAI(int k) : size_k(k)
{
    A = new std::atomic_int[size_k];
    initializeFAI(A, size_k);
}


void KBasketFAI::initializeDefault(int n) { // To perform a lazy loading after create a  object with default constructor
    A = new std::atomic_int[n];
    size_k = n;
    initializeFAI(A, n);
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
    return BASKET_CLOSED;
}

void initializeCAS(std::atomic_int* array, int size) {
    for (int i = 0; i < size; i++) {
        array[i] = BOTTOM;
    }
}

KBasketCAS::KBasketCAS() {}

KBasketCAS::KBasketCAS(int n) : size_n(n)
{
    A = new std::atomic_int[size_n];
    initializeCAS(A, size_n);
}

KBasketCAS::~KBasketCAS() {
    delete [] A;
}

void KBasketCAS::initializeDefault(int n) { // To perform a lazy loading after create a  object with default constructor
    A = new std::atomic_int[n];
    size_n = n;
    initializeCAS(A, n);
    for(int i = 0; i < n; i++) {
        takes_p.insert(i);
    }
}

int KBasketCAS::compete(int pos)
{
    int x = A[pos].load();
    if (x == TOP) {
        return TOP;
    } else if (A[pos].compare_exchange_strong(x, TOP)) { // https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange
        return x;
    }
    return BOTTOM;
}

STATE_PUT KBasketCAS::put(int x, int process)
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

bool inTakes(int process, std::unordered_set<int> set) {
    std::unordered_set<int>::const_iterator got = set.find(process);
    return got != set.end();
}

int randomValInSet(std::unordered_set<int> set) { //  TODO: Add tests (MAPA 2021-12-19)
    int size = set.size();
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0,size);
    std::unordered_set<int> :: iterator itr;
    int pos = distribution(generator);
    auto it = set.begin();
    std::advance(it, pos); //  TODO: Delete object once it's hit (MAPA 2021-12-19)
    return *it;
}

int KBasketCAS::take(int process)
{
    int pos;
    while(true) {
        if (STATE.load() == CLOSED) {
            return BASKET_CLOSED;
        } else {
            if (inTakes(process, takes_p)) {
                pos = process;
            } else {
                pos = randomValInSet(takes_p);
            }
            takes_p.erase(pos);
            if (takes_p.empty()) {
                STATE.store(CLOSED);
            }
            int x = compete(pos);
            if (x != TOP && x!= BOTTOM) {
                return x;
            } else if (x == BOTTOM) {
                x = compete(pos);
                if (x != TOP && x != BOTTOM) {
                    return x;
                }
            }
        }
    }
}
