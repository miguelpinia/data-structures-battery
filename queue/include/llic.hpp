#ifndef LLIC_HPP
#define LLIC_HPP
#include <atomic>
#include <stdalign.h>

// https://en.cppreference.com/w/cpp/language/object#Alignment
struct alignas(64) aligned_int { // https://en.cppreference.com/w/cpp/language/alignas
    int value = 0; // We use 64 bytes to allocate the int value;
};

struct alignas(64) aligned_atomic_int {
    std::atomic<int> value = 0;
};

struct alignas(16) aligned_atomic_int_16 {
    std::atomic<int> value = 0;
};

struct alignas(32) aligned_atomic_int_32 {
    std::atomic<int> value = 0;
};

struct alignas(128) aligned_atomic_int_128 {
    std::atomic<int> value = 0;
};

class LLICRW
{
private:
    aligned_atomic_int* M;
    int num_processes;
public:
    LLICRW();
    LLICRW(int n);
    // ~LLICRW();
    void initializeDefault(int n);
    int LL();
    void IC(int max_p, int process);
};


class LLICRW16
{
private:
    aligned_atomic_int_16* M;
    int num_processes;
public:
    LLICRW16();
    LLICRW16(int n);
    // ~LLICRW();
    void initializeDefault(int n);
    int LL();
    void IC(int max_p, int process);
};

class LLICRW32
{
private:
    aligned_atomic_int_32* M;
    int num_processes;
public:
    LLICRW32();
    LLICRW32(int n);
    // ~LLICRW();
    void initializeDefault(int n);
    int LL();
    void IC(int max_p, int process);
};

class LLICRW128
{
private:
    aligned_atomic_int_128* M;
    int num_processes;
public:
    LLICRW128();
    LLICRW128(int n);
    // ~LLICRW();
    void initializeDefault(int n);
    int LL();
    void IC(int max_p, int process);
};


// Without cycle
class LLICRWWC
{
private:
    aligned_atomic_int* M;
    int num_processes;
public:
    LLICRWWC();
    LLICRWWC(int n);
    // ~LLICRW();
    void initializeDefault(int n);
    int LL();
    void IC(int max_p, int process);
};

// Without cycle
class LLICRWWCNP
{
private:
    std::atomic<int>* M;
    int num_processes;
public:
    LLICRWWCNP();
    LLICRWWCNP(int n);
    // ~LLICRW();
    void initializeDefault(int n);
    int LL();
    void IC(int max_p, int process);
};

// No padding
class LLICRWNC
{
private:
    std::atomic<int>* M;
    int num_processes;
public:
    LLICRWNC();
    LLICRWNC(int n);
    // ~LLICRW();
    void initializeDefault(int n);
    int LL();
    void IC(int max_p, int process);
};

class LLICRWSQRT
{
private:
    std::atomic<int>* M;
    int num_processes;
    int size;
public:
    LLICRWSQRT();
    LLICRWSQRT(int n);
    void initializeDefault(int n);
    int LL(int& ind_max_p);
    bool IC(int max_p, int ind_max_p, int thread_id);
};

class LLICRWSQRTFS
{
private:
    aligned_atomic_int* M;
    int num_processes;
    int size;
public:
    LLICRWSQRTFS();
    LLICRWSQRTFS(int n);
    void initializeDefault(int n);
    int LL(int& ind_max_p);
    bool IC(int max_p, int ind_max_p, int thread_id);
};

class LLICRWNewSolRandom
{
private:
    std::atomic<int>* M;
    int num_processes;
    int size;
public:
    LLICRWNewSolRandom();
    LLICRWNewSolRandom(int n);
    void initializeDefault(int n);
    int LL(int max_p, int& ind_max_p);
    bool IC(int max_p, int ind_max_p, int thread_id);
};

class LLICCAS
{
private:
    std::atomic_int R = 0;

public:
    LLICCAS();
    int LL();
    void IC(int expected);
};

class LLICCAST
{
private:
    std::atomic_int R = 0;

public:
    LLICCAST();
    bool LLIC();
    int get();
};

class LLICRWNCT {

private:
    std::atomic<int>* M;
    int num_processes;

public:

    LLICRWNCT();
    LLICRWNCT(int n);

    void initializeDefault(int n);
    bool LLIC(int process);
    int get();
};

// class LLICRWT2 {
// private:
//     std::atomic<int>* M;
//     int num_processes;

// public:
//     LLICRWT2();
//     LLICRWT2(int n);

//     void initializeDefault(int n);
//     int LL();
//     void IC(int)
// };
#endif
