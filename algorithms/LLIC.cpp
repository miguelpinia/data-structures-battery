#include "LLIC.hpp"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <random>
#include <iterator>
#include <iostream>

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
        tmp = M[i].value.load();
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
        tmp = M[i].value.load();
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
        tmp = M[i].value.load();
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
        tmp = M[i].value.load();
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


LLICRWWCNP::LLICRWWCNP() {}

LLICRWWCNP::LLICRWWCNP(int n): num_processes(n)
{
    M = new std::atomic<int>[num_processes];
    for (int i = 0; i < n; i++) M[i] = 0;
}

void LLICRWWCNP::initializeDefault(int n) {
    num_processes = n;
    M = new std::atomic<int>[num_processes];
    for (int i = 0; i < n; i++) M[i] = 0;
}

int LLICRWWCNP::LL() {
    int max_p = 0;
    int tmp;
    for(int i = 0; i < num_processes; i++) {
        tmp = M[i].load();
        if (tmp >= max_p) max_p = tmp;
    }
    return max_p;
}

void LLICRWWCNP::IC(int max_p, int process) {
    M[process].store(max_p + 1);
}

/////////////////////
// Without padding //
/////////////////////

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
    int tmp;
    for(int i = 0; i < num_processes; i++) {
        tmp = M[i].load();
        if (tmp >= max_p) max_p = tmp;
    }
    return max_p;
}

void LLICRWNC::IC(int max_p, int process) {
    int maximum = 0;
    int tmp;
    for(int i = 0; i < num_processes; i++) {
        tmp = M[i].load();
        if (tmp > maximum) maximum = tmp;
    }
    if (maximum == max_p) {
        M[process].store(max_p + 1);
    }

}

//////////////////
// New Solution //
//////////////////

template<typename Iter, typename RandomGenerator>
Iter select_randomly(Iter start, Iter end, RandomGenerator& g) {
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}

template<typename Iter>
Iter select_randomly(Iter start, Iter end) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return select_randomly(start, end, gen);
}

int get_random_from_range(int begin, int end, int exclude)
{
    std::vector<int> range;
    for (int i = begin; i < end; i++) {
        if (i != exclude) {
            range.push_back(i);
        }
    }

    return *select_randomly(range.begin(), range.end());
}


LLICRWSQRT::LLICRWSQRT() {
    size = 2;
    M = new std::atomic<int>[2];
    for (int i = 0; i < size; i++) {
        M[i] = 0;
    }
}

LLICRWSQRT::LLICRWSQRT(int n) : num_processes(n)
{
    size = (int) std::sqrt(n);
    M = new std::atomic<int>[size];
    for (int i = 0; i < size; i++) {
        M[i] = 0;
    }
}

void LLICRWSQRT::initializeDefault(int n)
{
    size = (int) std::sqrt(n);
    M = new std::atomic<int>[size];
    for (int i = 0; i < size; i++) {
        M[i] = 0;
    }
}

int LLICRWSQRT::LL(int& ind_max_p)
{
    int max_p = -1;
    int x;
    for (int i = 0; i < size; i++) {
        x = M[i].load();
        if (x > max_p) {
            max_p = x;
            ind_max_p = i;
        }
    }
    return max_p;
}

bool LLICRWSQRT::IC(int max_p, int ind_max_p, int thread_i)
{
    int pos = -1;
    if (size < 2) {
        pos = 0;
    } else {
        pos = (ind_max_p + max_p + thread_i) % size; // sumar el índice del hilo
        if (pos == ind_max_p)
            pos = (pos + 1) % size;
    }
    int x = M[pos];
    if (x < max_p + 1) {
        if (M[pos].compare_exchange_strong(x, max_p + 1)) {
            return true;
        }
    }
    if (M[ind_max_p] == max_p) {
        M[ind_max_p].compare_exchange_strong(max_p, max_p + 1);
    }
    return true;
}


LLICRWSQRTFS::LLICRWSQRTFS() {}

LLICRWSQRTFS::LLICRWSQRTFS(int n) : num_processes(n)
{
    size = (int) std::sqrt(n);
    M = new aligned_atomic_int[size];
}

void LLICRWSQRTFS::initializeDefault(int n)
{
    size = (int) std::sqrt(n);
    M = new aligned_atomic_int[size];
}

int LLICRWSQRTFS::LL(int& ind_max_p)
{
    int max_p = -1;
    int x;
    for (int i = 0; i < size; i++) {
        x = M[i].value.load();
        if (x > max_p) {
            max_p = x;
            ind_max_p = i;
        }
    }
    return max_p;
}

bool LLICRWSQRTFS::IC(int max_p, int ind_max_p, int thread_i)
{
    int pos = -1;
    if (size < 2) {
        pos = 0;
    } else {

        pos = (ind_max_p + max_p + thread_i) % size; // sumar el índice del hilo
        if (pos == ind_max_p)
            pos = (pos + 1) % size;
    }
    int x = M[pos].value.load();
    if (x < max_p + 1) {
        if (M[pos].value.compare_exchange_strong(x, max_p + 1)) {
            return true;
        }
    }
    if (M[ind_max_p].value == max_p) {
        M[ind_max_p].value.compare_exchange_strong(max_p, max_p + 1);
    }
    return true;
}



//////////////////////////////////////////////////
// Solution without use restrictive randomness; //
//////////////////////////////////////////////////

LLICRWNewSolRandom::LLICRWNewSolRandom() {}

LLICRWNewSolRandom::LLICRWNewSolRandom(int n) : num_processes(n)
{
    std::srand (std::time(NULL));
    size = (int) std::sqrt(n);
    M = new std::atomic<int>[size];
    for (int i = 0; i < size; i++) {
        M[i] = 0;
    }
}

void LLICRWNewSolRandom::initializeDefault(int n)
{
    size = (int) std::sqrt(n);
    M = new std::atomic<int>[size];
    for (int i = 0; i < size; i++) {
        M[i] = 0;
    }
}

int LLICRWNewSolRandom::LL(int max_p, int& ind_max_p)
{
    max_p = -1;
    int x;
    for (int i = 0; i < size; i++) {
        x = M[i].load();
        if (x > max_p) {
            max_p = x;
            ind_max_p = i;
        }
    }
    return max_p;
}

bool LLICRWNewSolRandom::IC(int max_p, int ind_max_p, int thread_i)
{
    int pos = -1;
    if (size < 2) {
        pos = 0;
    } else {
        // pos = rand() % size;
        pos = (ind_max_p + max_p + thread_i) % size; // sumar el índice del hilo
        if (pos == ind_max_p)
            pos = (pos + 1) % size;
    }
    int x = M[pos];
    if (x < max_p + 1) {
        if (M[pos].compare_exchange_strong(x, max_p + 1)) {
            return true;
        }
    }
    if (M[ind_max_p] == max_p) {
        M[ind_max_p].compare_exchange_strong(max_p, max_p + 1);
    }
    return true;
}
