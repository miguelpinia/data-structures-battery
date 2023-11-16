#include <iostream>
#include "testqueue.h"
#include "include/basket_queue.hpp"
#include "include/queue.hpp"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

using ::testing::Return;

TestQueue::TestQueue() {};

TestQueue::~TestQueue() {};

void TestQueue::SetUp() {};

void TestQueue::TearDown() {};

// Pruebas dummy
TEST_F(TestQueue, isInsertedOneElement)
{
    Queue q;
    q.push(0);
    EXPECT_EQ(q.size(), 1);
}

TEST_F(TestQueue, isInsertedAndDeleted)
{
    Queue q;
    q.push(1);
    int result = q.pop();
    EXPECT_EQ(result, 1);
    EXPECT_EQ(q.size(), 0);
}

TEST_F(TestQueue, isEnqueueAndDequeueRWFAI)
{
    // std::cout << "Testing RWFAIBasketQueue with 10,000,000 elements enqueued and dequeued " << std::endl;
    // Crea una cola de 1,000,000 de elementos, con baskets de tamaño 1
    FAIQueue<LLICRW> queue{10000000, 1, 1};
    for(int i = 0; i < 10000000; i++) {
        // Los procesos comienzan en cero
        queue.enqueue(i, 0);
    }
    for(int i = 0; i < 10000000; i++) {
        EXPECT_EQ(queue.dequeue(0), i);
    }
}

TEST_F(TestQueue, isEnqueueAndDequeueCASFAI)
{
    // std::cout << "Testing CASFAIBasketQueue with 10,000,000 elements enqueued and dequeued " << std::endl;
    // Crea una cola de 1,000,000 de elementos, con baskets de tamaño 1
    FAIQueue<LLICCAS>  queue{10000000, 1, 1};
    for(int i = 0; i < 10000000; i++) {
        // Los procesos comienzan en cero
        queue.enqueue(i, 0);
    }
    for(int i = 0; i < 10000000; i++) {
        EXPECT_EQ(queue.dequeue(0), i);
    }
}

TEST_F(TestQueue, isEnqueueAndDequeueCASCAS)
{
    // std::cout << "Testing CASCASBasketQueue with 10,000,000 elements enqueued and dequeued " << std::endl;
    // Crea una cola de 1,000,000 de elementos, con baskets de tamaño 1
    CASQueue<LLICCAS> queue{10000000, 1};
    for(int i = 0; i < 10000000; i++) {
        // Los procesos comienzan en cero
        queue.enqueue(i, 0);
    }
    for(int i = 0; i < 10000000; i++) {
        EXPECT_EQ(queue.dequeue(0), i);
    }

}


TEST_F(TestQueue, isEnqueueAndDequeueRWCAS)
{
    // std::cout << "Testing RWCASBasketQueue with 10,000,000 elements enqueued and dequeued " << std::endl;
    // Crea una cola de 1,000,000 de elementos, con baskets de tamaño 1
    CASQueue<LLICRW> queue{10000000, 1};
    for(int i = 0; i < 10000000; i++) {
        // Los procesos comienzan en cero
        queue.enqueue(i, 0);
    }
    for(int i = 0; i < 10000000; i++) {
        EXPECT_EQ(queue.dequeue(0), i);
    }
}
