#include <thread>
#include <iostream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <functional>
#include <fstream>
#include <vector>
#include <barrier>
#include "nlohmann/json.hpp"
#include "LLICTest.hpp"
#include "utils.hpp"
#include <random>

using json = nlohmann::json;
using namespace std::chrono_literals;

//////////////////////////////////////////////////////////////////
// Comparar implementaciones de LL/IC. Desde uno hasta el total //
// de hilos, realizar operaciones de LL seguidas de IC y medir  //
// cuanto tardan. En esa misma ejecución comparar respecto a    //
// Fetch&Increment                                              //
//////////////////////////////////////////////////////////////////



void print_execution_LLICRW(int cores) {
    std::cout << "Increase until to get 5'000'000: Case of LL/IC RW without False Sharing" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRW llic;
    llic.initializeDefault(cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void(int)> func = [&llic](int processID) {
        int max = 0;
        while (max < 5'000'000) {
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
              << " ns\n"
              << llic.LL() << " max value stored\n";
}

void print_execution_LLICCAS(int cores) {
    std::cout << "\nIncrease until to get 5'000'000: Case of LL/IC CAS based " << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    LLICCAS llic;
    std::vector<std::thread> vecOfThreads;
    std::function<void()> func = [&llic]() {
        int max = 0;
        while (max < 5'000'000) {
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
    std::cout << "\nIncrease until to get 5'000'000: Case of LL/IC RW With FalseSharing " << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWNC llic;
    llic.initializeDefault(cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void(int)> func = [&llic](int processID) {
        int max = 0;
        while (max < 5'000'000) {
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
    std::cout << "\nIncrease until to get 5'000'000: Case of Fetch&Increment" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    std::atomic_int fai;
    std::vector<std::thread> vecOfThreads;
    // Function to execute
    std::function<void()> func = [&fai]() {
        int max = 0;
        while (max < 5'000'000) {
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
              << 1000.0 * (c_end - c_start) / CLOCKS_PER_SEC << " ns\n"
              << "Wall clock time passed: "
              << std::chrono::duration<double, std::nano>(t_end-t_start).count()
              << " ms\n"
              << fai.load() << " max value stored\n";
}

long same_ops_LLICRW(int cores) {
    std::cout << "Performing 5'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW without False Sharing" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRW llic(cores + 1);
    int operations = 5'000'000 / (cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void(int)> func = [&](int processID) {
        int max = 0;
        for (int i = 0; i < operations; ++i) {
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
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL());
    return duration;
}

long same_ops_LLICCAS(int cores) {
    std::cout << "\nPerforming 5'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC CAS based " << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    LLICCAS llic;
    int operations = 5'000'000 / (cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void()> func = [&]() {
        int max = 0;
        for (int i = 0; i < operations; ++i) {
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
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL());
    return duration;
}

long same_ops_LLICRWNC(int cores) {
    std::cout << "\nPerforming 5'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW With FalseSharing " << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWNC llic;
    llic.initializeDefault(cores + 1);
    int operations = 5'000'000 / (cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void(int)> func = [&](int processID) {
        int max = 0;
        for (int i = 0; i < operations; ++i) {
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
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL());
    return duration;
}

long same_ops_FAI(int cores) {
    std::cout << "\nPerforming 5'000'000 of operations. Each thread do the total divided by the number of threads: Case of Fetch&Increment" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    std::atomic_int fai = 0;
    std::vector<std::thread> vecOfThreads;
    int operations = 5'000'000 / (cores + 1);
    // // Function to execute
    std::function<void()> func = [&]() {
        for (int i = 0; i < operations; ++i) {
            fai.fetch_add(1);
            // Añadir delay fijo
            // Sumar valor aleatorio
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
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, fai.load());
    return duration;
}

long same_ops_FAI_delay(int cores) {
    std::cout << "\nPerforming 5'000'000 of operations. Each thread do the total divided by the number of threads: Case of Fetch&Increment. (RANDOM time)" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    std::atomic_int fai = 0;
    std::vector<std::thread> vecOfThreads;
    int operations = 5'000'000 / (cores + 1);
    // Function to execute
    std::function<void()> func = [&]() {
        for (int i = 0; i < operations; ++i) {
            fai.fetch_add(i);
            for (int j = 0; j < 100; j++) {}
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
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, fai.load());
    return duration;
}

long same_ops_FAI_random(int cores) {
    std::cout << "\nPerforming 5'000'000 of operations. Each thread do the total divided by the number of threads: Case of Fetch&Increment (RANDOM value)" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    std::atomic_int fai = 0;
    std::vector<std::thread> vecOfThreads;
    int operations = 5'000'000 / (cores + 1);
    // // Function to execute
    std::function<void()> func = [&]() {
        std::random_device rd;  //Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> distrib(1, 100);
        for (int i = 0; i < operations; ++i) {
            fai.fetch_add(distrib(gen));
            // Añadir delay fijo
            // Sumar valor aleatorio
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
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, fai.load());
    return duration;
}

long same_ops_LLICRW16(int cores) {
    std::cout << "\nPerforming 5'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW 16bits padding without False Sharing" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRW16 llic(cores + 1);
    // llic.initializeDefault(cores + 1);
    int operations = 5'000'000 / (cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void(int)> func = [&](int processID) {
        int max = 0;
        for (int i = 0; i < operations; ++i) {
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
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL());
    return duration;
}


long same_ops_LLICRW32(int cores) {
    std::cout << "\nPerforming 5'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW 32 bits padding without False Sharing" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRW32 llic(cores + 1);
    // llic.initializeDefault(cores + 1);
    int operations = 5'000'000 / (cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void(int)> func = [&](int processID) {
        int max = 0;
        for (int i = 0; i < operations; ++i) {
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
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL());
    return duration;
}

long same_ops_LLICRW128(int cores) {
    std::cout << "\nPerforming 5'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW 128 bits padding without False Sharing" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRW128 llic(cores + 1);
    // llic.initializeDefault(cores + 1);
    int operations = 5'000'000 / (cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void(int)> func = [&](int processID) {
        int max = 0;
        for (int i = 0; i < operations; ++i) {
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
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL());
    return duration;
}

long same_ops_LLICRWWC(int cores) {
    std::cout << "\nPerforming 5'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW Without Cycle and False Sharing" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWWC llic(cores + 1);
    // llic.initializeDefault(cores + 1);
    int operations = 5'000'000 / (cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void(int)> func = [&](int processID) {
        int max = 0;
        for (int i = 0; i < operations; ++i) {
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
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL());
    return duration;
}

long same_ops_LLICRWNewSol(int cores) {
    std::cout << "\nPerforming 5'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW New Solution" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWNewSol llic(cores + 1);
    int operations = 5'000'000 / (cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void()> func = [&]() {
        int max_p = 0;
        int ind_max_p = 0;
        for (int i = 0; i < operations; ++i) {
            max_p = llic.LL(max_p, ind_max_p);
            llic.IC(max_p, ind_max_p);
        }
    };
    for (int i = 0; i < cores + 1; i++) {
        vecOfThreads.push_back(std::thread(func));
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
    int fake_ind_max_p = 0;
    std::clock_t c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL(0, fake_ind_max_p));
    return duration;
}


void experiment_time_execution(int iterations) {
    std::cout << "Testing LL/IC" << std::endl;
    const auto processor_count = std::thread::hardware_concurrency();
    std::cout << "Number of cores: " << processor_count << std::endl;
    json result;
    result["iterations"] = iterations;
    result["processors_num"] = processor_count;
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<long> llicrwvec;
        std::vector<long> llicrwncvec;
        std::vector<long> lliccasvec;
        std::vector<long> faivec;
        std::vector<long> fairandomvec;
        std::vector<long> faidelayvec;
        std::vector<long> llicrw16vec;
        std::vector<long> llicrw32vec;
        std::vector<long> llicrw128vec;
        std::vector<long> llicrwwcvec;
        std::vector<long> llicrwns;
        for (int i = 0; i < (int)processor_count; ++i) {
            std::cout << "\n\nPerforming experiment for " << i + 1 << " cores\n\n" << std::endl;
            std::cout << "Same number of operations by type: " << std::endl;
            llicrwvec.push_back(same_ops_LLICRW(i));
            llicrwncvec.push_back(same_ops_LLICRWNC(i));
            lliccasvec.push_back(same_ops_LLICCAS(i));
            faivec.push_back(same_ops_FAI(i));
            fairandomvec.push_back(same_ops_FAI_random(i));
            faidelayvec.push_back(same_ops_FAI_delay(i));
            llicrw16vec.push_back(same_ops_LLICRW16(i));
            llicrw32vec.push_back(same_ops_LLICRW32(i));
            llicrw128vec.push_back(same_ops_LLICRW128(i));
            llicrwwcvec.push_back(same_ops_LLICRWWC(i));
            llicrwns.push_back(same_ops_LLICRWNewSol(i));
        }
        json r_iter;
        r_iter["RW"] = llicrwvec;
        r_iter["RWNC"] = llicrwncvec;
        r_iter["RW16"] = llicrw16vec;
        r_iter["RW32"] = llicrw32vec;
        r_iter["RW128"] = llicrw128vec;
        r_iter["RWWC"] = llicrwwcvec;
        r_iter["CAS"] = lliccasvec;
        r_iter["FAI"] = faivec;
        r_iter["FAIRANDOM"] = fairandomvec;
        r_iter["FAIDELAY"] = faidelayvec;
        r_iter["RWNS"] = llicrwns;
        result["iter-" + std::to_string(iter)] = r_iter;
    }
    std::cout << std::setw(4) << result << std::endl;
    std::ofstream file("results.json");
    file << std::setw(4) << result << std::endl;
    file.close();
}
