#include "LLIC.hpp"

LLICCAS::LLICCAS() {}

int LLICCAS::LL()
{
    return R.load();
}

void LLICCAS::IC(int expected)
{
    if (R.load() == expected) {
        R.compare_exchange_strong(expected, expected + 1);
    }
}

LLICRW::LLICRW() {}

LLICRW::LLICRW(int n): num_processes(n)
{
    M = new aligned_atomic_int[num_processes];
}

// LLICRW::~LLICRW() {
//     delete M;
// }

void LLICRW::initializeDefault(int n) {
    num_processes = n;
    M = new aligned_atomic_int[num_processes];
}

int LLICRW::LL() {
    int max_p = 0;
    int tmp;
    for(int i = 0; i < num_processes; i++) {
        tmp = M[i].value.load();
        if (tmp >= max_p) max_p = tmp;
    }
    return max_p;
}

void LLICRW::IC(int max_p, int process) {
    int maximum = 0;
    int tmp;
    for(int i = 0; i < num_processes; i++) {
        tmp = M[i].value;
        if (tmp > maximum) maximum = tmp;
    }
    if (maximum == max_p) {
        M[process].value.store(max_p + 1);
    }

}

LLICRWNC::LLICRWNC() {}

LLICRWNC::LLICRWNC(int n): num_processes(n)
{
    M = new std::atomic<int>[num_processes];
    for (int i = 0; i < n; ++i) {
        M[i] = 0;
    }
}

// LLICRW::~LLICRW() {
//     delete M;
// }

void LLICRWNC::initializeDefault(int n) {
    num_processes = n;
    M = new std::atomic<int>[num_processes];
    for (int i = 0; i < n; ++i) {
        M[i] = 0;
    }
}

int LLICRWNC::LL() {
    int max_p = 0;
    for(int i = 0; i < num_processes; i++) {
        if (M[i] >= max_p)
            max_p = M[i].load();
    }
    return max_p;
}

void LLICRWNC::IC(int max_p, int process) {
    int maximum = 0;
    for(int i = 0; i < num_processes; i++) {
        if (M[i] > maximum) maximum = M[i];
    }
    if (maximum == max_p) {
        M[process].store(max_p + 1);
    }

}
