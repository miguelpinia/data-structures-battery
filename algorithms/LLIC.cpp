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

/////////////////////
// 64 bits version //
/////////////////////

LLICRW::LLICRW() {}

LLICRW::LLICRW(int n): num_processes(n)
{
    M = new aligned_atomic_int[num_processes];
}

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



/////////////////////
// 16 bits version //
/////////////////////

LLICRW16::LLICRW16() {}

LLICRW16::LLICRW16(int n): num_processes(n)
{
    M = new aligned_atomic_int_16[num_processes];
}

void LLICRW16::initializeDefault(int n) {
    num_processes = n;
    M = new aligned_atomic_int_16[num_processes];
}

int LLICRW16::LL() {
    int max_p = 0;
    int tmp;
    for(int i = 0; i < num_processes; i++) {
        tmp = M[i].value.load();
        if (tmp >= max_p) max_p = tmp;
    }
    return max_p;
}

void LLICRW16::IC(int max_p, int process) {
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


/////////////////////
// 32 bits version //
/////////////////////

LLICRW32::LLICRW32() {}

LLICRW32::LLICRW32(int n): num_processes(n)
{
    M = new aligned_atomic_int_32[num_processes];
}

void LLICRW32::initializeDefault(int n) {
    num_processes = n;
    M = new aligned_atomic_int_32[num_processes];
}

int LLICRW32::LL() {
    int max_p = 0;
    int tmp;
    for(int i = 0; i < num_processes; i++) {
        tmp = M[i].value.load();
        if (tmp >= max_p) max_p = tmp;
    }
    return max_p;
}

void LLICRW32::IC(int max_p, int process) {
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

//////////////////////
// 128 bits version //
//////////////////////


LLICRW128::LLICRW128() {}

LLICRW128::LLICRW128(int n): num_processes(n)
{
    M = new aligned_atomic_int_128[num_processes];
}

void LLICRW128::initializeDefault(int n) {
    num_processes = n;
    M = new aligned_atomic_int_128[num_processes];
}

int LLICRW128::LL() {
    int max_p = 0;
    int tmp;
    for(int i = 0; i < num_processes; i++) {
        tmp = M[i].value.load();
        if (tmp >= max_p) max_p = tmp;
    }
    return max_p;
}

void LLICRW128::IC(int max_p, int process) {
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

/////////////////////////////////////
// Version without use cycle in IC //
/////////////////////////////////////


LLICRWWC::LLICRWWC() {}

LLICRWWC::LLICRWWC(int n): num_processes(n)
{
    M = new aligned_atomic_int[num_processes];
}

void LLICRWWC::initializeDefault(int n) {
    num_processes = n;
    M = new aligned_atomic_int[num_processes];
}

int LLICRWWC::LL() {
    int max_p = 0;
    int tmp;
    for(int i = 0; i < num_processes; i++) {
        tmp = M[i].value.load();
        if (tmp >= max_p) max_p = tmp;
    }
    return max_p;
}

void LLICRWWC::IC(int max_p, int process) {
    M[process].value.store(max_p + 1);
}


LLICRWNC::LLICRWNC() {}

LLICRWNC::LLICRWNC(int n): num_processes(n)
{
    M = new std::atomic<int>[num_processes];
    for (int i = 0; i < n; ++i) {
        M[i] = 0;
    }
}

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
