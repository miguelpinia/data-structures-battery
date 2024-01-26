#include <iostream>
#include <thread>
#include <vector>
#include <barrier>
#include <random>
#include "testqueue.h"
#include "include/basket_queue.hpp"
#include "include/queue.hpp"
#include "include/utils.hpp"
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

TEST_F(TestQueue, allEnqueued)
{
    const auto cores = std::thread::hardware_concurrency();
    const auto operations = 100'000;
    int k = (int) std::sqrt(cores);
    FAIQueue<LLICCAS> queue{operations, k, cores};
    std::vector<std::thread> threads;
    int totalOps = operations / cores;
    auto wait_for_begin = [] () noexcept {};
    std::barrier sync_points(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processId) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_points.arrive_and_wait();
        for (int i = 0; i < totalOps; i++) {
            queue.enqueue(distrib(gen), processId);
            for (int j = 0; j < 60; j = j + distrib(gen)) {}
        }
        std::cout << "Thread " << processId << " finished" << std::endl;
    };
    for (int i = 0; i < cores; i++) {
        threads.emplace_back(func, i);
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(threads[i].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }
    // std::cout << threads.size() << std::endl;
    for (std::thread &th : threads) {
        if (th.joinable()) th.join();
    }

    int totalEnqueued = 0;
    while (queue.dequeue(0) != EMPTY) {
        totalEnqueued++;
    }
    EXPECT_EQ(totalEnqueued, operations);
}
