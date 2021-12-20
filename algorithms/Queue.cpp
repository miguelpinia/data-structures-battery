#include "Queue.h"

Queue::Queue()
{};

void Queue::push(int value)
{
    q.push(value);
}

int Queue::pop()
{
    int result = q.front();
    q.pop();
    return result;
}

int Queue::size()
{
    return q.size();
}
