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
#include <fstream>
#include <cmath>
#include "nlohmann/json.hpp"
#include "include/FAAArrayQueue.hpp"
#include "include/MichaelScottQueue.hpp"
#include "include/LCRQ.hpp"
#include "include/YMCQueue.hpp"
#include "include/SBQQueue.hpp"
#include "include/LLICQueue.hpp"
#include "include/utils.hpp"

using json = nlohmann::json;
using namespace std::chrono_literals;

namespace tests {

    template <typename Queue>
    long queue_test_general(int cores, Queue &queue, int operations) { // Operation by thread
        std::clock_t c_start = std::clock();
        auto t_start = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads;
        auto wait_for_begin = []() noexcept {};
        std::barrier sync_point(cores, wait_for_begin);
        std::function<void(int)> func = [&] (const int thread_id) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(1, 3);
            sync_point.arrive_and_wait();
            std::string* foo = new std::string();
            for (int i = 0; i < operations; i++) {
                for (int j = 0; j < 40; j = j + distrib(gen)) {}

                *foo = std::to_string(i) + "\n";
                queue.enqueue(foo, thread_id);
            }
            for (int i = 0; i < operations; i++) {
                for (int j = 0; j < 40; j = j + distrib(gen)) {}
                queue.dequeue(thread_id);
            }
            delete foo;
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

    void queue_time_enq_deq(int iterations, std::size_t total_ops) {
        std::cout << "Testing queues\n";
        const auto cores = std::thread::hardware_concurrency();
        std::cout << "Number of hardware threads: " << cores << std::endl;
        json result;

        result["iterations"] = iterations;
        result["processors_num"] = cores;
        for (int iter = 0; iter < iterations; iter++) {
            std::vector<long> faa_queue_vec;
            std::vector<long> ms_queue_vec;
            std::vector<long> lcrq_queue_vec;
            std::vector<long> ymc_queue_vec;
            std::vector<long> sbq_queue_vec;
            std::vector<long> llic_queue_vec;

            for (int i = 0; i < (int)cores; i++) {
                std::size_t total_cores = i + 1;
                int operations = total_ops / (i + 1);
                std::cout << "\n\nPerforming experiment for " << total_cores << " hardware threads.\n\n";

                faa_array::Queue<std::string> faa_queue{total_cores};
                ms_queue::Queue<std::string> ms_queue{total_cores};
                lcrq_queue::Queue<std::string> lcrq_queue{total_cores};
                ymc_queue::Queue<std::string> ymc_queue{total_cores};
                scal_basket_queue::Queue<std::string> sbq_queue{total_cores};
                llic_queue::FAIQueue<std::string, llic_queue::LLICCAS, llic_queue::KBasketFAI<std::string, 3>, 1000020> llic_queue;
                std::cout << "FAA Queue\n";
                faa_queue_vec.push_back(queue_test_general(total_cores, faa_queue, operations));
                std::cout << "\nMS Queue\n";
                ms_queue_vec.push_back(queue_test_general(total_cores, ms_queue, operations));
                std::cout << "\nLCRQ Queue\n";
                lcrq_queue_vec.push_back(queue_test_general(total_cores, lcrq_queue, operations));
                std::cout << "\nYMC Queue\n";
                ymc_queue_vec.push_back(queue_test_general(total_cores, ymc_queue, operations));
                std::cout << "\nSBQ Queue\n";
                sbq_queue_vec.push_back(queue_test_general(total_cores, sbq_queue, operations));
                std::cout << "\nLLIC Queue\n";
                llic_queue_vec.push_back(queue_test_general(total_cores, llic_queue, operations));
            }
            json r_iter;
            r_iter["FAAQUEUE"] = faa_queue_vec;
            r_iter["MSQUEUE"] = ms_queue_vec;
            r_iter["LCRQQUEUE"] = lcrq_queue_vec;
            r_iter["YMCQUEUE"] = ymc_queue_vec;
            r_iter["SBQQUEUE"] = sbq_queue_vec;
            r_iter["LLICQUEUE"] = llic_queue_vec;
            result["iter-" + std::to_string(iter)] = r_iter;
        }
        std::cout << std::setw(4) << result << std::endl;
        std::time_t currTime;
        std::tm* currTm;
        std::time(&currTime);
        currTm = std::localtime(&currTime);
        char buffer[256];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H:%M:%S", currTm);
        std::string fileName = "results/" + std::string(buffer) + "_test_queue_time_execution.json";
        std::ofstream file(fileName);
        file << std::setw(4) << result << std::endl;
        file.close();
        std::cout << fileName << std::endl;
    }

}
#endif
