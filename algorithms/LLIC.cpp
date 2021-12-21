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
    M = new padded_int[num_processes];
}

LLICRW::~LLICRW() {
    delete [] M;
}

void LLICRW::initializeDefault(int n) {
    num_processes = n;
    M = new padded_int[num_processes];
}

int LLICRW::LL() {
    int max_p = 0;
    for(int i = 0; i < num_processes; i++) {
        if (M[i].value >= max_p)
            max_p = M[i].value;
    }
    return max_p;
}

void LLICRW::IC(int max_p, int process) {
    int maximum = 0;
    for(int i = 0; i < num_processes; i++) {
        if (M[i].value >= max_p) maximum = M[i].value;
    }
    if (maximum == max_p) {
        M[process].value = max_p + 1;
    }

}
