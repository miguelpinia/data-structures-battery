#ifndef KBASKET_HPP
#define KBASKET_HPP
#include <unordered_set>
#include <atomic>
#include "utils.hpp" // Se declaran los estados para el basket y para put

class KBasketFAI
{
private:
    std::atomic<int> *A;
public:
    int size_k;
    std::atomic<int> PUTS{0};
    std::atomic<int> TAKES{0};
    std::atomic<STATE_BASKET> STATE{OPEN};

    KBasketFAI();
    KBasketFAI(int k);
    ~KBasketFAI();

    void initializeDefault(int k);

    STATE_PUT put(int x);
    int take();
};

class NBasketCAS
{
private:
    std::atomic<int> *A;
    int compete(int pos);
public:
    int size_n;
    std::atomic<STATE_BASKET> STATE{OPEN};
    // std::unordered_set<int> takes_p;

    NBasketCAS();
    NBasketCAS(int n);
    ~NBasketCAS();

    void initializeDefault(int n);

    STATE_PUT put(int x, int process);
    int take(int process);
};
#endif
