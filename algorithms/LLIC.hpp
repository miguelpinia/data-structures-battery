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

class LLICCAS
{
private:
    std::atomic_int R = 0;

public:
    LLICCAS();
    int LL();
    void IC(int expected);
};
#endif
