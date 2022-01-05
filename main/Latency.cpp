#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <barrier>
#include <vector>
#include <functional>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <string>

#include "Latency.hpp"
#include "utils.hpp"
#include "../algorithms/LLIC.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;


std::tuple<long, int> latency_FAI(int cores) {
    std::cout << "\n Measuring the latency of FAI when the contention grows\n" << std::endl;
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    auto wait_for_begin = []() noexcept {};
    std::barrier sync_point(cores + 1, wait_for_begin);
    std::atomic<int> fai = 0;
    std::vector<std::thread> threads;
    std::function<void()> func = [&]() {
        sync_point.arrive_and_wait();
        fai.fetch_add(1);
    };
    for (int i = 0; i < cores + 1; i++) {
        threads.push_back(std::thread(func));
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
        if (th.joinable()) {
            th.join();
        }
    }
    auto t_end = std::chrono::high_resolution_clock::now();
    std::clock_t c_end = std::clock();
    long duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, fai);
    return std::make_tuple(duration, fai.load());
};

std::tuple<long, int> latency_LLICCAS(int cores) {
    std::cout << "\nMeasuring the latency of LL/IC as contention grows\n" << std::endl;
    std::clock_t c_start = std::clock();
    auto t_start = std::chrono::high_resolution_clock::now();
    auto wait_for_begin = []() noexcept {
        std::cout << "Begining execution\n";
    };
    std::barrier sync_point(cores + 1, wait_for_begin);
    LLICCAS llic;
    std::vector<std::thread> threads;

    std::function<void()> func = [&]() {
        sync_point.arrive_and_wait();
        int m = llic.LL();
        llic.IC(m);
        std::cout << "Leí: " << m << std::endl;
    };

    for (int i = 0; i < cores + 1; i++) {
        threads.push_back(std::thread(func));
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
        if (th.joinable()) {
            th.join();
        }
    }
    auto t_end = std::chrono::high_resolution_clock::now();
    std::clock_t c_end = std::clock();
    long duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
    print_time((c_end - c_start), duration, llic.LL());
    return std::make_tuple(duration, llic.LL());
}


void latency_experiment(int iterations){
    const auto processor_count = std::thread::hardware_concurrency();
    std::cout << "Number of cores: " << processor_count << std::endl;
    json result;
    for (int iter = 0; iter < iterations; ++iter) {
        std::vector<long> latfai;
        std::vector<long> latlliccas;
        std::vector<int> maxStoredFai;
        std::vector<int> maxStoredLLICCAS;
        for (int proc = 0; proc < (int) processor_count; ++proc) {
            std::cout << "\nPerforming experiment for " << proc + 1 << " cores\n" << std::endl;
            auto t_fai = latency_FAI(proc);
            auto t_cas = latency_LLICCAS(proc);
            latfai.push_back(std::get<0>(t_fai));
            maxStoredFai.push_back(std::get<1>(t_fai));
            latlliccas.push_back(std::get<0>(t_cas));
            maxStoredLLICCAS.push_back(std::get<1>(t_cas));
        }
        json r_iter;
        r_iter["LAT_FAI"] = latfai;
        r_iter["LAT_LLICCAS"] = latlliccas;
        r_iter["max_stored_FAI"] = maxStoredFai;
        r_iter["max_stored_LLICCAS"] = maxStoredLLICCAS;
        result["iter-" + std::to_string(iter)] = r_iter;
    }
    std::cout << std::setw(4) << result << std::endl;
    std::ofstream file("latency.json");
    file << std::setw(4) << result << std::endl;
    file.close();
}
