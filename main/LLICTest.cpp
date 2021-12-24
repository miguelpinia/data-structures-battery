#include <thread>
#include <iostream>
#include <vector>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <functional>
#include <fstream>
#include <vector>
#include "nlohmann/json.hpp"
#include "LLICTest.hpp"


using json = nlohmann::json;

//////////////////////////////////////////////////////////////////
// Comparar implementaciones de LL/IC. Desde uno hasta el total //
// de hilos, realizar operaciones de LL seguidas de IC y medir  //
// cuanto tardan. En esa misma ejecución comparar respecto a    //
// Fetch&Increment                                              //
//////////////////////////////////////////////////////////////////

void print_time(std::clock_t time, double duration, int value) {
    // Printing times
    // https://pythonspeed.com/articles/blocking-cpu-or-io/
    std::cout << std::fixed << std::setprecision(2) << "CPU time used: "
              << 1000.0 * (time) / CLOCKS_PER_SEC << " ms\n"
              << "Wall clock time passed: " << duration << " ms\n"
              << value << " max value stored\n";
}

void print_execution_LLICRW(int cores) {
    std::cout << "Increase until to get 100,000,000: Case of LL/IC RW without False Sharing" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRW llic;
    llic.initializeDefault(cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void(int)> func = [&llic](int processID) {
        int max = 0;
        while (max < 100'000'000) {
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
    std::cout << "\nIncrease until to get 100,000,000: Case of LL/IC CAS based " << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    LLICCAS llic;
    std::vector<std::thread> vecOfThreads;
    std::function<void()> func = [&llic]() {
        int max = 0;
        while (max < 100'000'000) {
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
    std::cout << "\nIncrease until to get 100,000,000: Case of LL/IC RW With FalseSharing " << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWNC llic;
    llic.initializeDefault(cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void(int)> func = [&llic](int processID) {
        int max = 0;
        while (max < 100'000'000) {
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
    std::cout << "\nIncrease until to get 100,000,000: Case of Fetch&Increment" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    std::atomic_int fai;
    std::vector<std::thread> vecOfThreads;
    // Function to execute
    std::function<void()> func = [&fai]() {
        int max = 0;
        while (max < 100'000'000) {
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

double same_ops_LLICRW(int cores) {
    std::cout << "Performing 500000000 of operations. Each thread do the total between #threads: Case of LL/IC RW without False Sharing" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRW llic;
    llic.initializeDefault(cores + 1);
    int operations = 500000000 / (cores + 1);
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
    double duration = std::chrono::duration<double, std::milli>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL());
    return duration;
}

double same_ops_LLICCAS(int cores) {
    std::cout << "\nPerforming 500000000 of operations. Each thread do the total between #threads: Case of LL/IC CAS based " << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    LLICCAS llic;
    int operations = 500000000 / (cores + 1);
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
    double duration = std::chrono::duration<double, std::milli>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL());
    return duration;
}

double same_ops_LLICRWNC(int cores) {
    std::cout << "\nPerforming 500000000 of operations. Each thread do the total between #threads: Case of LL/IC RW With FalseSharing " << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWNC llic;
    llic.initializeDefault(cores + 1);
    int operations = 500000000 / (cores + 1);
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
    double duration = std::chrono::duration<double, std::milli>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL());
    return duration;
}

double same_ops_FAI(int cores) {
    std::cout << "\nPerforming 500000000 of operations. Each thread do the total between #threads: Case of Fetch&Increment" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    std::atomic_int fai = 0;
    std::vector<std::thread> vecOfThreads;
    int operations = 500000000 / (cores + 1);
    // // Function to execute
    std::function<void()> func = [&]() {
        for (int i = 0; i < operations; ++i) {
            fai.fetch_add(1);
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
    double duration = std::chrono::duration<double, std::milli>(t_end-t_start).count();
    print_time((c_end - c_start), duration, fai.load());
    return duration;
}

void write_to_json(std::vector<double> v1, std::vector<double> v2, std::vector<double> v3, std::vector<double> v4) {
    json j;
    j.emplace("RW", v1);
    j.emplace("RWNC", v2);
    j.emplace("CAS", v3);
    j.emplace("FAI", v4);
    std::cout << std::setw(4) << j << std::endl;
    std::ofstream file("results.json");
    file << std::setw(4) << j << std::endl;
    file.close();
}


void testLLICRW() {
    std::cout << "Testing LL/IC" << std::endl;
    const auto processor_count = std::thread::hardware_concurrency();
    std::cout << "Number of cores: " << processor_count << std::endl;
    std::vector<double> llicrwvec;
    std::vector<double> llicrwncvec;
    std::vector<double> lliccasvec;
    std::vector<double> faivec;
    for (int i = 0; i < (int)processor_count; ++i) {
        std::cout << "\n\nPerforming experiment for " << i + 1 << " cores\n\n" << std::endl;
        // print_execution_LLICRW(i);
        // print_execution_LLICRWNC(i);
        // print_execution_LLICCAS(i);
        // print_execution_FAI(i);
        std::cout << "\n\nPerforming the same number of operations by type: " << std::endl;
        llicrwvec.push_back(same_ops_LLICRW(i));
        llicrwncvec.push_back(same_ops_LLICRWNC(i));
        lliccasvec.push_back(same_ops_LLICCAS(i));
        faivec.push_back(same_ops_FAI(i));
    }
    write_to_json(llicrwvec, llicrwncvec, lliccasvec, faivec);
}
