#include "LLIC.hpp"

int LLICCAS::LL()
{
    return R.load();
}

void LLICCAS::IC(int expected)
{
    auto r = R.load();
    if (r == expected) {
        R.compare_exchange_strong(expected, expected + 1);
    }
}

LLICRW::LLICRW() {}

LLICRW::LLICRW(int n): num_processes(n)
{
    M = new int[num_processes]();//Adding parenthesis after initialization, put all entries to zero.
}

LLICRW::~LLICRW() {
    delete [] M;
}

void LLICRW::initializeDefault(int n) {
    num_processes = n;
    M = new int[num_processes]();
}

int LLICRW::LL(int max_p) {
    for(int i = 0; i < num_processes; i++) {
        if (M[i] >= max_p)
            max_p = M[i];
    }
    return max_p;
}

void LLICRW::IC(int max_p, int process) {
    for(int i = 0; i < num_processes; i++) {
        if (M[i] == max_p) {
            M[process] = max_p + 1;
            break;
        }
    }
}
