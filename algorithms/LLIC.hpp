#include <atomic>

class LLICRW
{
private:
    int* M;
    int num_processes;
public:
    LLICRW();
    LLICRW(int n);
    ~LLICRW();
    void initializeDefault(int n);
    int LL(int max_p);
    void IC(int max_p, int process);
};

class LLICCAS
{
private:
    std::atomic_int R = 0;

public:
    int LL();
    void IC(int expected);
};
