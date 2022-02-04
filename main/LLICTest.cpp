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


// //////////////////////////////////////////////////////////
// Pruebas finales:                                     //
// - Añadir delay a todos                               //
// - FAI y LL/IC con CAS         -                       //
// - R/W with false sharing       -                      //
// - Sqrt without false sharing, padding 64 bits       - //
// - Sqrt with false sharing                           - //
// - R/W without false sharing and without cycle in IC  //
// - R/W with false sharing and without cycle in IC     //
//////////////////////////////////////////////////////////

long same_ops_FAI_delay(int cores) {
    std::cout << "\nPerforming 5'000'000 of operations. Each thread do the total divided by the number of threads: Case of Fetch&Increment. (with delay)" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    std::atomic_int fai = 0;
    std::vector<std::thread> vecOfThreads;
    int operations = 5'000'000 / (cores + 1);
    // Function to execute
    std::function<void()> func = [&]() {
        std::random_device rd;  //Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> distrib(1, 5);
        for (int i = 0; i < operations; ++i) {
            fai.fetch_add(1);
            for (int j = 0; j < 30; j = j + distrib(gen)) {}
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

long same_ops_LLICCAS(int cores) {
    std::cout << "\nPerforming 5'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC CAS based " << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    LLICCAS llic;
    int operations = 5'000'000 / (cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void()> func = [&]() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 5);
        int max = 0;
        for (int i = 0; i < operations; ++i) {
            max = llic.LL();
            llic.IC(max);
            for (int j = 0; j < 30; j = j + distrib(gen)) {}
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
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 5);
        int max = 0;
        for (int i = 0; i < operations; ++i) {
            max = llic.LL();
            llic.IC(max, processID);
            for(int j = 0; j < 30; j = j + distrib(gen)) {}
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
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 5);
        int max = 0;
        for (int i = 0; i < operations; ++i) {
            max = llic.LL();
            llic.IC(max, processID);
            for (int j = 0; j < 30; j = j + distrib(gen)) {}
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

long same_ops_LLICRW_SQRT(int cores) {
    std::cout << "\nPerforming 5'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW SQRT with false sharing" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWSQRT llic(cores + 1);
    int operations = 5'000'000 / (cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 5);
        int max_p = 0;
        int ind_max_p = 0;
        for (int i = 0; i < operations; ++i) {
            max_p = llic.LL(ind_max_p);
            llic.IC(max_p, ind_max_p, processID);
            for (int j = 0; j < 30; j = j + distrib(gen)) {}
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
    int fake_ind_max_p = 0;
    std::clock_t c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL(fake_ind_max_p));
    return duration;
}

long same_ops_LLICRW_SQRT_FS(int cores) {
    std::cout << "\nPerforming 5'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW SQRT without false sharing" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWSQRTFS llic(cores + 1);
    int operations = 5'000'000 / (cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 5);
        int max_p = 0;
        int ind_max_p = 0;
        for (int i = 0; i < operations; ++i) {
            max_p = llic.LL(ind_max_p);
            llic.IC(max_p, ind_max_p, processID);
            for (int j = 0; j < 30; j = j + distrib(gen)) {}
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
    int fake_ind_max_p = 0;
    std::clock_t c_end = std::clock();
    auto t_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL(fake_ind_max_p));
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
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 5);
        int max = 0;
        for (int i = 0; i < operations; ++i) {
            max = llic.LL();
            llic.IC(max, processID);
            for (int j = 0; j < 30; j = j + distrib(gen)) {}
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

long same_ops_LLICRWWCNP(int cores) {
    std::cout << "\nPerforming 5'000'000 of operations. Each thread do the total divided by the number of threads: Case of LL/IC RW Without Cycle and False Sharing" << std::endl;
    // Measuring time
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    // Magic begins
    LLICRWWCNP llic(cores + 1);
    // llic.initializeDefault(cores + 1);
    int operations = 5'000'000 / (cores + 1);
    std::vector<std::thread> vecOfThreads;
    std::function<void(int)> func = [&](int processID) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distrib(1, 5);
        int max = 0;
        for (int i = 0; i < operations; ++i) {
            max = llic.LL();
            llic.IC(max, processID);
            for (int j = 0; j < 30; j = j + distrib(gen)) {}
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

void experiment_time_execution(int iterations) {
    std::cout << "Testing LL/IC" << std::endl;
    const auto processor_count = std::thread::hardware_concurrency();
    std::cout << "Number of cores: " << processor_count << std::endl;
    json result;
    result["iterations"] = iterations;
    result["processors_num"] = processor_count;
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<long> faidelayvec;
        std::vector<long> lliccasvec;
        std::vector<long> llicrwvec;
        std::vector<long> llicrwncvec;
        std::vector<long> llicrwsqrtvec;
        std::vector<long> llicrwsqrtfsvec;
        std::vector<long> llicrwwcvec;
        std::vector<long> llicrwwcnpvec;
        for (int i = 0; i < (int)processor_count; ++i) {
            std::cout << "\n\nPerforming experiment for " << i + 1 << " cores\n\n" << std::endl;
            std::cout << "Same number of operations by type: " << std::endl;
            faidelayvec.push_back(same_ops_FAI_delay(i));
            lliccasvec.push_back(same_ops_LLICCAS(i));
            llicrwvec.push_back(same_ops_LLICRW(i));
            llicrwncvec.push_back(same_ops_LLICRWNC(i));
            llicrwsqrtvec.push_back(same_ops_LLICRW_SQRT(i));
            llicrwsqrtfsvec.push_back(same_ops_LLICRW_SQRT_FS(i));
            llicrwwcvec.push_back(same_ops_LLICRWWC(i));
            llicrwwcnpvec.push_back(same_ops_LLICRWWCNP(i));

        }
        json r_iter;
        r_iter["FAIDELAY"] = faidelayvec;
        r_iter["CAS"] = lliccasvec;
        r_iter["RW"] = llicrwvec; // Without false sharing
        r_iter["RWNC"] = llicrwncvec; // With false sharing
        r_iter["RWSQRT"] = llicrwsqrtvec; // With false sharing
        r_iter["RWSQRTFS"] = llicrwsqrtfsvec; // Without false sharing
        r_iter["RWWC"] = llicrwwcvec; // Without false sharing
        r_iter["RWWCNP"] = llicrwwcnpvec; // With false sharing
        result["iter-" + std::to_string(iter)] = r_iter;
    }
    std::cout << std::setw(4) << result << std::endl;
    std::ofstream file("results.json");
    file << std::setw(4) << result << std::endl;
    file.close();
}
