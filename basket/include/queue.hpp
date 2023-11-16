#include <queue>

class Queue
{
public:
    Queue();
    void push(int value);
    int pop();
    int size();
protected:
    std::queue<int> q;
};
