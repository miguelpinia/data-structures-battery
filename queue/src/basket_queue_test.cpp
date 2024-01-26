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

long enq_deq_cas_cas(int cores, int operations) {
    std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads. CAS-CAS." << std::endl;
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    CASQueue<LLICCAS> queue{operations, cores};
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
            std::atomic_thread_fence(std::memory_order_release);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::atomic_thread_fence(std::memory_order_release);
            queue.dequeue(processID);
            std::atomic_thread_fence(std::memory_order_acquire);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::atomic_thread_fence(std::memory_order_release);
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

long enq_deq_cas_fai(int cores, int operations) {
    std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads. CAS-FAI." << std::endl;
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    int k = (int) std::sqrt(cores);
    if (cores > 1) k++;
    FAIQueue<LLICCAS> queue{operations, k, cores};
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
            std::atomic_thread_fence(std::memory_order_release);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::atomic_thread_fence(std::memory_order_release);
            queue.dequeue(processID);
            std::atomic_thread_fence(std::memory_order_acquire);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::atomic_thread_fence(std::memory_order_release);
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

long enq_deq_rw_cas(int cores, int operations) {
    std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads. RW-CAS." << std::endl;
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
            std::atomic_thread_fence(std::memory_order_release);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::atomic_thread_fence(std::memory_order_release);
            queue.dequeue(processID);
            std::atomic_thread_fence(std::memory_order_acquire);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::atomic_thread_fence(std::memory_order_release);
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
    std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads. RW16-CAS." << std::endl;
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
            std::atomic_thread_fence(std::memory_order_release);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::atomic_thread_fence(std::memory_order_release);
            queue.dequeue(processID);
            std::atomic_thread_fence(std::memory_order_acquire);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::atomic_thread_fence(std::memory_order_release);
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
    std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads. RW-FAI." << std::endl;
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    int k = (int) std::sqrt(cores);
    if (cores > 1) k++;
    std::cout << "K: " << k << std::endl;
    FAIQueue<LLICRW> queue{operations, k, cores};
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
            std::atomic_thread_fence(std::memory_order_release);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::atomic_thread_fence(std::memory_order_release);
            queue.dequeue(processID);
            std::atomic_thread_fence(std::memory_order_acquire);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::atomic_thread_fence(std::memory_order_release);
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
    std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads. RW16-FAI." << std::endl;
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    int k = (int) std::sqrt(cores);
    if (cores > 1) k++;
    FAIQueue<LLICRW16> queue{operations, k, cores};
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
            std::atomic_thread_fence(std::memory_order_release);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::atomic_thread_fence(std::memory_order_release);
            queue.dequeue(processID);
            std::atomic_thread_fence(std::memory_order_acquire);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::atomic_thread_fence(std::memory_order_release);
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


long enq_deq_grouped16_fai(int cores, int operations) {
    std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads. RWG16-FAI." << std::endl;
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    int k = (int) std::sqrt(cores);
    if (cores > 1) k++;
    std::cout << "K: " << k << std::endl;
    FAIGQueue<LLICRWSQRTG16> queue{operations, k, cores, 4};
    std::vector<std::thread> threads;
    int totalOps = operations / cores;
    auto wait_for_begin = [] () noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        int tail_idx_max = 0;
        int head_idx_max = 0;
        for (int i = 0; i < totalOps; i++) {
            queue.enqueue(distrib(gen), processID, tail_idx_max);
            std::atomic_thread_fence(std::memory_order_release);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::atomic_thread_fence(std::memory_order_release);
            queue.dequeue(processID, tail_idx_max, head_idx_max);
            std::atomic_thread_fence(std::memory_order_acquire);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::atomic_thread_fence(std::memory_order_release);
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

long enq_deq_grouped16_cas(int cores, int operations) {
    std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads. RWG16-CAS." << std::endl;
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    CASGQueue<LLICRWSQRTG16> queue{operations, cores, 4};
    std::vector<std::thread> threads;
    int totalOps = operations / cores;
    auto wait_for_begin = [] () noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        int tail_idx_max = 0;
        int head_idx_max = 0;
        for (int i = 0; i < totalOps; i++) {
            queue.enqueue(distrib(gen), processID, tail_idx_max);
            std::atomic_thread_fence(std::memory_order_release);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::atomic_thread_fence(std::memory_order_release);
            queue.dequeue(processID, tail_idx_max, head_idx_max);
            std::atomic_thread_fence(std::memory_order_acquire);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            std::atomic_thread_fence(std::memory_order_release);
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
        std::vector<long> queuecascas;
        std::vector<long> queuecasfai;
        std::vector<long> queuerwg16fai;
        std::vector<long> queuerwg16cas;
        for (int i = 0; i < (int) processor_count; ++i) {
            int totalCores = i + 1;
            std::cout << "Performing experiment for " << totalCores << " cores.\n\n" << std::endl;
            queuerwcas.push_back(enq_deq_rw_cas(totalCores, totalOps));
            queuerwfai.push_back(enq_deq_rw_fai(totalCores, totalOps));
            queuerw16cas.push_back(enq_deq_rw16_cas(totalCores, totalOps));
            queuerw16fai.push_back(enq_deq_rw16_fai(totalCores, totalOps));
            queuecascas.push_back(enq_deq_cas_cas(totalCores, totalOps));
            queuecasfai.push_back(enq_deq_cas_fai(totalCores, totalOps));
            queuerwg16fai.push_back(enq_deq_grouped16_fai(totalCores, totalOps));
            queuerwg16cas.push_back(enq_deq_grouped16_cas(totalCores, totalOps));
        }
        json r_iter;
        r_iter["RWCAS"] = queuerwcas;
        r_iter["RWFAI"] = queuerwfai;
        r_iter["RW16CAS"] = queuerw16cas;
        r_iter["RW16FAI"] = queuerw16fai;
        r_iter["CASCAS"] = queuecascas;
        r_iter["CASFAI"] = queuecasfai;
        r_iter["RWG16FAI"] = queuerwg16fai;
        r_iter["RWG16CAS"] = queuerwg16cas;
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
