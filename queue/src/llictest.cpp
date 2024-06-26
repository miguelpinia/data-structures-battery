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
#include "include/llic.hpp"
#include "include/utils.hpp"
#include <random>
#include <cstdlib>


using json = nlohmann::json;
using namespace std::chrono_literals;

//////////////////////////////////////////////////////////////////
// Comparar implementaciones de LL/IC. Desde uno hasta el total //
// de hilos, realizar operaciones de LL seguidas de IC y medir  //
// cuanto tardan. En esa misma ejecución comparar respecto a    //
// Fetch&Increment                                              //
//////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
// Pruebas finales:                                       //
// - Añadir delay a todos                                 //
// - FAI y LL/IC con CAS         -                        //
// - R/W with false sharing       -                       //
// - Sqrt without false sharing, padding 64 bits       -  //
// - Sqrt with false sharing                           -  //
// - R/W without false sharing and without cycle in IC    //
// - R/W with false sharing and without cycle in IC       //
////////////////////////////////////////////////////////////

long same_ops_FAI_delay(int cores) {
    std::cout << "\nPerforming 10'000'000 of operations. Each thread do the total divided by the number of threads: Case of Fetch&Increment. (with delay)" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    std::atomic_int fai = 0;
    std::vector<std::thread> vecOfThreads;
    int operations = 10'000'000 / cores;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    // Function to execute

    std::function<void()> func = [&]() {
        std::random_device rd;  //Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        for (int i = 0; i < operations; ++i) {
            fai.fetch_add(1);
            for (int j = 0; j < 80; j = j + distrib(gen)) {}
        }
    };
    for (int i = 0; i < cores; i++) {
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
    return fai.load();
}


long same_ops_LLICCAS(int cores) {
    std::cout << "\nPerforming 10'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC CAS based " << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    LLICCAS llic;
    int operations = 10'000'000 / cores;
    std::vector<std::thread> vecOfThreads;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void()> func = [&]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        int max = 0;
        for (int i = 0; i < operations; ++i) {
            max = llic.LL();

            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            llic.IC(max);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
        }
    };

    for (int i = 0; i < cores; i++) {
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
    return llic.LL();
}

long same_ops_LLICCAST(int cores) {
    std::cout << "\nPerforming 10'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC CAS based with operations togheter" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    LLICCAST llic;
    int operations = 10'000'000 / cores;
    std::vector<std::thread> vecOfThreads;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void()> func = [&]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        for (int i = 0; i < operations; ++i) {
            llic.LLIC();
            for (int j = 0; j < 80; j = j + distrib(gen)) {}
        }
    };

    for (int i = 0; i < cores; i++) {
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
    print_time((c_end - c_start), duration, llic.get());
    return llic.get();
}


long same_ops_LLICRW(int cores) {
    std::cout << "\nPerforming 10'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW Without FalseSharing " << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRW llic(cores);
    // LLICRW llic;
    // llic.initializeDefault(cores + 1);
    int operations = 10'000'000 / cores;
    std::vector<std::thread> vecOfThreads;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        int max = 0;
        for (int i = 0; i < operations; ++i) {
            max = llic.LL();
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            llic.IC(max, processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            // for (int j = 0; j < 30; j = j + distrib(gen)) {}
        }
    };
    for (int i = 0; i < cores; i++) {
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
    return llic.LL();
}

long same_ops_LLICRW16(int cores) {
    std::cout << "\nPerforming 10'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW With FalseSharing. 16 bytes of padding." << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRW16 llic(cores);
    // LLICRW llic;
    // llic.initializeDefault(cores + 1);
    int operations = 10'000'000 / cores;
    std::vector<std::thread> vecOfThreads;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        int max = 0;
        for (int i = 0; i < operations; ++i) {
            max = llic.LL();
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            llic.IC(max, processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            // for (int j = 0; j < 30; j = j + distrib(gen)) {}
        }
    };
    for (int i = 0; i < cores; i++) {
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
    return llic.LL();
}

long same_ops_LLICRW32(int cores) {
    std::cout << "\nPerforming 10'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW With FalseSharing. 32 bytes of padding." << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRW32 llic(cores);
    // LLICRW llic;
    // llic.initializeDefault(cores + 1);
    int operations = 10'000'000 / cores;
    std::vector<std::thread> vecOfThreads;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        int max = 0;
        for (int i = 0; i < operations; ++i) {
            max = llic.LL();
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            llic.IC(max, processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            // for (int j = 0; j < 30; j = j + distrib(gen)) {}
        }
    };
    for (int i = 0; i < cores; i++) {
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
    return llic.LL();
}

long same_ops_LLICRW128(int cores) {
    std::cout << "\nPerforming 10'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW Without FalseSharing. 128 bytes of padding." << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRW128 llic(cores);
    // LLICRW llic;
    // llic.initializeDefault(cores + 1);
    int operations = 10'000'000 / cores;
    std::vector<std::thread> vecOfThreads;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        int max = 0;
        for (int i = 0; i < operations; ++i) {
            max = llic.LL();
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            llic.IC(max, processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            // for (int j = 0; j < 30; j = j + distrib(gen)) {}
        }
    };
    for (int i = 0; i < cores; i++) {
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
    return llic.LL();
}

long same_ops_LLICRWWC(int cores) {
    std::cout << "\nPerforming 10'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW Without Cycle without false sharing." << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWWC llic(cores);
    int operations = 10'000'000 / cores;
    std::vector<std::thread> vecOfThreads;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        int max = 0;
        for (int i = 0; i < operations; ++i) {
            max = llic.LL();
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            llic.IC(max, processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
        }
    };
    for (int i = 0; i < cores; i++) {
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
    return llic.LL();
}

long same_ops_LLICRWNP(int cores) {
    std::cout << "\nPerforming 10'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW With FalseSharing. No padding." << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWNP llic(cores);
    int operations = 10'000'000 / (cores);
    std::vector<std::thread> vecOfThreads;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        int max = 0;
        for (int i = 0; i < operations; ++i) {
            max = llic.LL();
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            llic.IC(max, processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            // for (int j = 0; j < 30; j = j + distrib(gen)) {}
        }
    };
    for (int i = 0; i < cores; i++) {
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
    return llic.LL();
}

long same_ops_LLICRWNCT(int cores) {
    std::cout << "\nPerforming 10'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW With FalseSharing. No padding, togheter." << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWNCT llic(cores);
    // LLICRW llic;
    // llic.initializeDefault(cores + 1);
    int operations = 10'000'000 / cores;
    std::vector<std::thread> vecOfThreads;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        for (int i = 0; i < operations; ++i) {
            llic.LLIC(processID);
            for (int j = 0; j < 80; j = j + distrib(gen)) {}
            // for (int j = 0; j < 30; j = j + distrib(gen)) {}
        }
    };
    for (int i = 0; i < cores; i++) {
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
    print_time((c_end - c_start), duration, llic.get());
    return llic.get();
}

long same_ops_LLICRWWCNP(int cores) {
    std::cout << "\nPerforming 10'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW With FalseSharing. Without Cycle, no padding." << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWWCNP llic(cores);
    int operations = 10'000'000 / cores;
    std::vector<std::thread> vecOfThreads;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        int max = 0;
        for (int i = 0; i < operations; ++i) {
            max = llic.LL();
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            llic.IC(max, processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
        }
    };
    for (int i = 0; i < cores; i++) {
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
    return llic.LL();
}

long same_ops_LLICRW_SQRT(int cores) {
    std::cout << "\nPerforming 10'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW SQRT with false sharing" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWSQRT llic{cores};
    // LLICRWSQRTFS llic;
    int operations = 10'000'000 / cores;
    std::vector<std::thread> vecOfThreads;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        int max_p = 0;
        int ind_max_p = 0;
        for (int i = 0; i < operations; ++i) {
            max_p = llic.LL(ind_max_p);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            llic.IC(max_p, ind_max_p, processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
        }
    };
    for (int i = 0; i < cores; i++) {
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
    int fake_ind_max_p = 0;
    std::clock_t c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL(fake_ind_max_p));
    return llic.LL(fake_ind_max_p);
}

long same_ops_LLICRW_SQRT_FS(int cores) {
    std::cout << "\nPerforming 10'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW SQRT without false sharing. 64 bytes of padding." << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWSQRTFS llic{cores};
    int operations = 10'000'000 / (cores);
    std::vector<std::thread> vecOfThreads;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        int max_p = 0;
        int ind_max_p = 0;
        for (int i = 0; i < operations; ++i) {
            max_p = llic.LL(ind_max_p);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            llic.IC(max_p, ind_max_p, processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            // for (int j = 0; j < 30; j = j + distrib(gen)) {}
        }
    };
    for (int i = 0; i < cores; i++) {
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
    int fake_ind_max_p = 0;
    std::clock_t c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL(fake_ind_max_p));
    return llic.LL(fake_ind_max_p);
}

long same_ops_LLICRW_SQRT_G(int cores, int group_size) {
    std::cout << "\nPerforming 10'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW grouped" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWSQRTG llic(cores, group_size);
    // LLICRWSQRTFS llic;
    int operations = 10'000'000 / (cores);
    std::vector<std::thread> vecOfThreads;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        int max_p = 0;
        int ind_max_p = 0;
        for (int i = 0; i < operations; ++i) {
            max_p = llic.LL(ind_max_p);

            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            llic.IC(max_p, ind_max_p, processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
        }
    };
    for (int i = 0; i < cores; i++) {
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
    int fake_ind_max_p = 0;
    std::clock_t c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL(fake_ind_max_p));
    return llic.LL(fake_ind_max_p);
}

long same_ops_LLICRW_SQRT_G_16(int cores, int group_size) {
    std::cout << "\nPerforming 10'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW grouped. 16 bytes of padding." << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWSQRTG16 llic(cores, group_size);
    // LLICRWSQRTFS llic;
    int operations = 10'000'000 / cores;
    std::vector<std::thread> vecOfThreads;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        int max_p = -1;
        int ind_max_p = 0;
        for (int i = 0; i < operations; ++i) {
            max_p = llic.LL(ind_max_p);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            llic.IC(max_p, ind_max_p, processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
        }
    };
    for (int i = 0; i < cores; i++) {
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
    int fake_ind_max_p = 0;
    std::clock_t c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL(fake_ind_max_p));
    return llic.LL(fake_ind_max_p);
}

long same_ops_LLICRW_SQRT_G_32(int cores, int group_size) {
    std::cout << "\nPerforming 10'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW grouped. 32 bytes of padding." << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWSQRTG32 llic(cores, group_size);
    // LLICRWSQRTFS llic;
    int operations = 10'000'000 / cores;
    std::vector<std::thread> vecOfThreads;
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores, wait_for_begin);
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 3);
        sync_point.arrive_and_wait();
        int max_p = 0;
        int ind_max_p = 0;
        for (int i = 0; i < operations; ++i) {
            max_p = llic.LL(ind_max_p);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
            llic.IC(max_p, ind_max_p, processID);
            for (int j = 0; j < 40; j = j + distrib(gen)) {}
        }
    };
    for (int i = 0; i < cores; i++) {
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
    int fake_ind_max_p = 0;
    std::clock_t c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL(fake_ind_max_p));
    return llic.LL(fake_ind_max_p);
}


void experiment_time_execution(int iterations) {
    std::cout << "Testing LL/IC" << std::endl;
    const auto processor_count = std::thread::hardware_concurrency();
    std::cout << "Number of cores: " << processor_count << std::endl;
    json result;
    result["iterations"] = iterations;
    result["processors_num"] = processor_count;
    for (int iter = 0; iter < iterations; ++iter) {
        // FAI
        std::vector<long> faidelayvec;
        // LLICCAS
        std::vector<long> lliccasvec;
        std::vector<long> lliccastvec;
        // LLICRW - aligned
        std::vector<long> llicrwvec;
        // LLICRW16 - aligned
        std::vector<long> llicrwvec16;
        // LLICRW32 - aligned
        std::vector<long> llicrwvec32;
        // LLICRW128 - aligned
        std::vector<long> llicrwvec128;
        // LLICRWWC - aligned
        std::vector<long> llicrwwcvec;
        // LLICRWWCNP
        std::vector<long> llicrwwcnpvec;
        // LLICRWNC - Without alignment. Why NC?
        std::vector<long> llicrwncvec;
        std::vector<long> llicrwnctvec;
        // LLICRWSQRT
        std::vector<long> llicrwsqrtvec;
        // LLICRWSQRTG
        std::vector<long> llicrwsqrtgvec;
        // LLICRWSQRTG16
        std::vector<long> llicrwsqrtg16vec;
        // LLICRWSQRTG32
        std::vector<long> llicrwsqrtg32vec;
        // LLICRWSQRTFS - aligned
        std::vector<long> llicrwsqrtfsvec;
        for (int i = 0; i < (int)processor_count; ++i) {
            std::cout << "\n\nPerforming experiment for " << i + 1 << " cores\n\n" << std::endl;
            std::cout << "Same number of operations by type" << std::endl;
            int totalCores = i + 1;
            faidelayvec.push_back(same_ops_FAI_delay(totalCores));

            lliccasvec.push_back(same_ops_LLICCAS(totalCores));
            lliccastvec.push_back(same_ops_LLICCAST(totalCores));

            llicrwvec.push_back(same_ops_LLICRW(totalCores));
            llicrwvec16.push_back(same_ops_LLICRW16(totalCores));
            llicrwvec32.push_back(same_ops_LLICRW32(totalCores));
            llicrwvec128.push_back(same_ops_LLICRW128(totalCores));

            llicrwwcvec.push_back(same_ops_LLICRWWC(totalCores));

            llicrwwcnpvec.push_back(same_ops_LLICRWWCNP(totalCores));

            llicrwncvec.push_back(same_ops_LLICRWNP(totalCores));
            llicrwnctvec.push_back(same_ops_LLICRWNCT(totalCores));

            llicrwsqrtvec.push_back(same_ops_LLICRW_SQRT(totalCores));
            llicrwsqrtgvec.push_back(same_ops_LLICRW_SQRT_G(totalCores, 4));
            llicrwsqrtg16vec.push_back(same_ops_LLICRW_SQRT_G_16(totalCores, 4));
            llicrwsqrtg32vec.push_back(same_ops_LLICRW_SQRT_G_32(totalCores, 2));
            llicrwsqrtfsvec.push_back(same_ops_LLICRW_SQRT_FS(totalCores));
        }
        json r_iter;
        r_iter["FAIDELAY"] = faidelayvec;
        r_iter["CAS"] = lliccasvec;
        r_iter["CAST"] = lliccastvec;
        r_iter["RW"] = llicrwvec; // Without false sharing
        r_iter["RW16"] = llicrwvec16; // Without false sharing
        r_iter["RW32"] = llicrwvec32; // Without false sharing
        r_iter["RW128"] = llicrwvec128; // Without false sharing
        r_iter["RWWC"] = llicrwwcvec; // Without false sharing
        r_iter["RWWCNP"] = llicrwwcnpvec; // With false sharing
        r_iter["RWNC"] = llicrwncvec; // With false sharing
        r_iter["RWNCT"] = llicrwnctvec; // With false sharing
        r_iter["RWSQRT"] = llicrwsqrtvec; // With false sharing
        r_iter["RWSQRTG"] = llicrwsqrtgvec; // With false sharing
        r_iter["RWSQRTG16"] = llicrwsqrtg16vec;
        r_iter["RWSQRTG32"] = llicrwsqrtg32vec;
        r_iter["RWSQRTFS"] = llicrwsqrtfsvec; // Without false sharing
        result["iter-" + std::to_string(iter)] = r_iter;
    }
    std::cout << std::setw(4) << result << std::endl;
    std::time_t currTime;
    std::tm* currTm;
    std::time(&currTime);
    currTm = std::localtime(&currTime);
    char buffer[256];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H:%M:%S", currTm);
    std::string fileName = std::string(buffer) + "_experiment_last_value.json";
    std::ofstream file(fileName);
    file << std::setw(4) << result << std::endl;
    file.close();
}
