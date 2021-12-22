#include <thread>
#include <iostream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <functional>
#include "LLICTest.hpp"

//////////////////////////////////////////////////////////////////
// Comparar implementaciones de LL/IC. Desde uno hasta el total //
// de hilos, realizar operaciones de LL seguidas de IC y medir  //
// cuanto tardan. En esa misma ejecución comparar respecto a    //
// Fetch&Increment                                              //
//////////////////////////////////////////////////////////////////

void print_execution_LLICRW(int cores) {
    std::cout << "Case of LL/IC RW " << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRW llic;
    llic.initializeDefault(cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void(int)> func = [&llic](int processID) {
        int max = 0;
        while (max < 1'000'000'000) {
            max = llic.LL();
            llic.IC(max, processID);
        }
    };
    for (int i = 0; i < cores + 1; i++) {
        vecOfThreads.push_back(std::thread(func, i));
        // https://eli.thegreenplace.net/2016/c11-threads-affinity-and-hyperthreading/
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(vecOfThreads[i].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }
    for (std::thread &th : vecOfThreads) {
        if (th.joinable()) {
            th.join();
        }
    }
    // Finish execution
    std::clock_t c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    // Printing times
    // https://pythonspeed.com/articles/blocking-cpu-or-io/
    std::cout << std::fixed << std::setprecision(2) << "CPU time used: "
              << 1000.0 * (c_end - c_start) / CLOCKS_PER_SEC << " ms\n"
              << "Wall clock time passed: "
              << std::chrono::duration<double, std::milli>(t_end-t_start).count()
              << " ms\n"
              << llic.LL() << " max value stored\n";
}

void print_execution_LLICCAS(int cores) {
    std::cout << "\nCase of LL/IC CAS based " << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    LLICCAS llic;
    std::vector<std::thread> vecOfThreads;
    std::function<void()> func = [&llic]() {
        int max = 0;
        while (max < 1'000'000'000) {
            max = llic.LL();
            llic.IC(max);
        }
    };

    for (int i = 0; i < cores + 1; i++) {
        vecOfThreads.push_back(std::thread(func));
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(vecOfThreads[i].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }
    for (std::thread &th : vecOfThreads) {
        if (th.joinable()) {
            th.join();
        }
    }
    // Finish execution
    std::clock_t c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    std::cout << std::fixed << std::setprecision(2) << "CPU time used: "
              << 1000.0 * (c_end - c_start) / CLOCKS_PER_SEC << " ms\n"
              << "Wall clock time passed: "
              << std::chrono::duration<double, std::milli>(t_end-t_start).count()
              << " ms\n"
              << llic.LL() << " max value stored\n";
}


void print_execution_LLICRWNC(int cores) {
    std::cout << "\nCase of LL/IC RW With FalseSharing " << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWNC llic;
    llic.initializeDefault(cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void(int)> func = [&llic](int processID) {
        int max = 0;
        while (max < 1'000'000'000) {
            max = llic.LL();
            llic.IC(max, processID);
        }
    };
    for (int i = 0; i < cores + 1; i++) {
        vecOfThreads.push_back(std::thread(func, i));
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(vecOfThreads[i].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }
    for (std::thread &th : vecOfThreads) {
        if (th.joinable()) {
            th.join();
        }
    }
    // Finish execution
    std::clock_t c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    std::cout << std::fixed << std::setprecision(2) << "CPU time used: "
              << 1000.0 * (c_end - c_start) / CLOCKS_PER_SEC << " ms\n"
              << "Wall clock time passed: "
              << std::chrono::duration<double, std::milli>(t_end-t_start).count()
              << " ms\n"
              << llic.LL() << " max value stored\n";
}

void print_execution_FAI(int cores) {
    std::cout << "\nCase of Fetch&Increment" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    std::atomic_int fai;
    std::vector<std::thread> vecOfThreads;
    // Function to execute
    std::function<void()> func = [&fai]() {
        int max = 0;
        while (max < 1'000'000'000) {
            max = fai.fetch_add(1);
        }
    };
    for (int i = 0; i < cores + 1; i++) {
        vecOfThreads.push_back(std::thread(func));
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(vecOfThreads[i].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }
    for (std::thread &th : vecOfThreads) {
        if (th.joinable()) {
            th.join();
        }
    }
    // Finish execution
    std::clock_t c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    std::cout << std::fixed << std::setprecision(2) << "CPU time used: "
              << 1000.0 * (c_end - c_start) / CLOCKS_PER_SEC << " ms\n"
              << "Wall clock time passed: "
              << std::chrono::duration<double, std::milli>(t_end-t_start).count()
              << " ms\n"
              << fai.load() << " max value stored\n";
}


void testLLICRW() {
    std::cout << "Testing LL/IC" << std::endl;
    const auto processor_count = std::thread::hardware_concurrency();
    std::cout << "Number of cores: " << processor_count << std::endl;
    for (int i = 0; i < processor_count; ++i) {
        std::cout << "\n\nPerforming experiment for " << i + 1 << " cores\n\n" << std::endl;
        print_execution_LLICRW(i);
        print_execution_LLICRWNC(i);
        print_execution_LLICCAS(i);
        print_execution_FAI(i);
    }
}
