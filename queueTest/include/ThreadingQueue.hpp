#ifndef _Threading_Queue_HPP_
#define _Threading_Queue_HPP_

#include <thread>
#include <iostream>
#include <chrono>
#include <ctime>
#include <functional>
#include <barrier>
#include <vector>
#include <random>
#include <cstdlib>
#include "HPQueue.hpp"
#include "FAAArrayQueue.hpp"
#include "include/utils.hpp"

template <typename Queue>
long queue_test_general(int cores, Queue &queue) {
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> threads;
    int operations = 10'000'000 / cores;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&] (const int thread_id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        std::string foo;
        for (int i = 0; i < operations; i++) {
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::string* foo = new std::string();
            *foo = std::to_string(i);
            queue.enqueue(foo, thread_id);
        }
        for (int i = 0; i < operations; i++) {
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            queue.dequeue(thread_id);
        }
    };
    for (int i = 0; i < cores; i++) {
        const int thread_id = i;
        threads.push_back(std::thread(func, thread_id));
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(threads[i].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }
    for (std::thread &th: threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    std::clock_t c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();
    print_time((c_end - c_start), duration, 0);
    return duration;
}

#endif
