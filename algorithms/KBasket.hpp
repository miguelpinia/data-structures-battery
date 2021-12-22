#ifndef KBASKET_HPP
#define KBASKET_HPP
#include <unordered_set>
#include <atomic>
#include "utils.hpp" // Se declaran los estados para el basket y para put

class KBasketFAI
{
private:
    std::atomic_int* A;
public:
    int size_k;
    std::atomic_int PUTS{0};
    std::atomic_int TAKES{0};
    std::atomic<STATE_BASKET> STATE = OPEN;

    KBasketFAI();
    KBasketFAI(int k);
    ~KBasketFAI();

    void initializeDefault(int k);

    STATE_PUT put(int x);
    int take();
};

class KBasketCAS
{
private:
    std::atomic_int* A;
    int compete(int pos);
public:
    int size_n;
    std::atomic<STATE_BASKET> STATE = OPEN;
    std::unordered_set<int> takes_p;

    KBasketCAS();
    KBasketCAS(int n);
    ~KBasketCAS();

    void initializeDefault(int n);

    STATE_PUT put(int x, int process);
    int take(int process);
};
#endif
