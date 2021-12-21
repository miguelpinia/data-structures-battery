#include <atomic>

struct padded_int {
    int value = 0;
    char padding[64 - sizeof(int)];
};

class LLICRW
{
private:
    padded_int* M;
    int num_processes;
public:
    LLICRW();
    LLICRW(int n);
    ~LLICRW();
    void initializeDefault(int n);
    int LL();
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
