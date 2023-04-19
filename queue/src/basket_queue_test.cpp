//
// Created by miguel on 18/04/23.
//
#include <thread>
#include <iostream>
#include <vector>
#include <barrier>
#include <functional>
#include <random>
#include <iomanip>
#include <fstream>
#include <cmath>
#include "include/basket_queue_test.hpp"
#include "include/basket_queue.hpp"
#include "include/llic.hpp"
#include "nlohmann/json.hpp"


using json = nlohmann::json;
using namespace std::chrono_literals;

long enq_deq_rw_cas(int cores, int operations) {
    std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads." << std::endl;
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    CASQueue<LLICRW> queue{operations, cores};
    std::vector<std::thread> threads;
    int totalOps = operations / cores;
    auto wait_for_begin = [] () noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        for (int i = 0; i < totalOps; i++) {
            queue.enqueue(distrib(gen), processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            queue.dequeue(processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
        }
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
    for (std::thread &th : threads) {
        if (th.joinable()) th.join();
    }

    std::clock_t  c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();
    print_time((c_end - c_start), duration, 1);
    return duration;
}

long enq_deq_rw16_cas(int cores, int operations) {
    std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads." << std::endl;
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    CASQueue<LLICRW16> queue{operations, cores};
    std::vector<std::thread> threads;
    int totalOps = operations / cores;
    auto wait_for_begin = [] () noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        for (int i = 0; i < totalOps; i++) {
            queue.enqueue(distrib(gen), processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            queue.dequeue(processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
        }
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
    for (std::thread &th : threads) {
        if (th.joinable()) th.join();
    }
    std::clock_t  c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();
    print_time((c_end - c_start), duration, 1);
    return duration;
}

long enq_deq_rw_fai(int cores, int operations) {
    std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads." << std::endl;
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    FAIQueue<LLICRW> queue{operations, (int) std::sqrt(cores), cores};
    std::vector<std::thread> threads;
    int totalOps = operations / cores;
    auto wait_for_begin = [] () noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        for (int i = 0; i < totalOps; i++) {
            queue.enqueue(distrib(gen), processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            queue.dequeue(processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
        }
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
    for (std::thread &th : threads) {
        if (th.joinable()) th.join();
    }
    std::clock_t  c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();
    print_time((c_end - c_start), duration, 1);
    return duration;
}

long enq_deq_rw16_fai(int cores, int operations) {
    std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads." << std::endl;
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    FAIQueue<LLICRW16> queue{operations, (int) std::sqrt(cores), cores};
    std::vector<std::thread> threads;
    int totalOps = operations / cores;
    auto wait_for_begin = [] () noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        for (int i = 0; i < totalOps; i++) {
            queue.enqueue(distrib(gen), processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            queue.dequeue(processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
        }
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
    for (std::thread &th : threads) {
        if (th.joinable()) th.join();
    }
    std::clock_t  c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();
    print_time((c_end - c_start), duration, 1);
    return duration;
}


void queue_time_experiments(int iterations) {
    std::cout << "Testing queues" << std::endl;
    const auto processor_count = std::thread::hardware_concurrency();
    std::cout << "Number of cores: " << processor_count << std::endl;
    json result;
    result["iterations"] = iterations;
    result["processors_num"] = processor_count;
    int totalOps = 5'000'000;
    for (int iter = 0; iter < iterations; iter++) {
        std::vector<long> queuerwcas;
        std::vector<long> queuerwfai;
        std::vector<long> queuerw16cas;
        std::vector<long> queuerw16fai;
        for (int i = 0; i < (int) processor_count; ++i) {
            int totalCores = i + 1;
            std::cout << "Performing experiment for " << totalCores << " cores.\n\n" << std::endl;
            queuerwcas.push_back(enq_deq_rw_cas(totalCores, totalOps));
            queuerwfai.push_back(enq_deq_rw_fai(totalCores, totalOps));
            queuerw16cas.push_back(enq_deq_rw16_cas(totalCores, totalOps));
            queuerw16fai.push_back(enq_deq_rw16_fai(totalCores, totalOps));
        }
        json r_iter;
        r_iter["RWCAS"] = queuerwcas;
        r_iter["RWFAI"] = queuerwfai;
        r_iter["RW16CAS"] = queuerwcas;
        r_iter["RW16FAI"] = queuerwfai;
        result["iter-" + std::to_string(iter)] = r_iter;
    }
    std::cout << std::setw(4) << result << std::endl;
    std::time_t currTime;
    std::tm* currTm;
    std::time(&currTime);
    currTm = std::localtime(&currTime);
    char buffer[256];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H:%M:%S", currTm);
    std::string fileName = std::string(buffer) + "_queue_time_execution.json";
    std::ofstream file(fileName);
    file << std::setw(4) << result << std::endl;
    file.close();
}
