#ifndef _Experiments_HPP
#define _Experiments_HPP

#include <thread>
#include <iostream>
#include <chrono>
#include <ctime>
#include <functional>
#include <barrier>
#include <random>
#include <cstdlib>
#include <fstream>
#include <cmath>
#include <limits>
#include <deque>
#include <vector>
#include <fstream>
#include <exception>
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

namespace experiments {

    static constexpr uintmax_t ITERATIONS = 75;
    static constexpr uintmax_t K = 5;
    static constexpr uintmax_t P = 30;



    template <typename Queue>
    long same_number_enq_deq_test(int cores, Queue &queue, int operationsByThread) {
        // std::clock_t c_start = std::clock();
        auto t_start = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads;
        auto wait_for_begin = []() noexcept {};
        std::barrier sync_point(cores, wait_for_begin);
        // std::cout << "Cores: " << cores << "; operations: " << operationsByThread << std::endl;
        std::function<void(int)> func = [&] (const int thread_id) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(1, 3);
            sync_point.arrive_and_wait();
            std::string* foo = new std::string();
            for (int i = 0; i < operationsByThread; i++) {
                for (int j = 0; j < 40; j = j + distrib(gen)) {}
                *foo = std::to_string(i) + "\n";
                queue.enqueue(foo, thread_id);
            }
            for (int i = 0; i < operationsByThread; i++) {
                for (int j = 0; j < 40; j = j + distrib(gen)) {}
                try {
                    queue.dequeue(thread_id);
                } catch (const std::runtime_error& e) {
                    std::cout << "Error al desencolar" << e.what() << std::endl;
                }
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

        // std::clock_t c_end = std::clock();
        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();
        // print_time((c_end - c_start), duration, 0);
        return duration;
    };


    template<typename Queue>
    long only_enq_test(int cores, Queue &queue, int operationsByThread) {
        // std::cout << "Only ENQUEUE: " << &queue << std::endl;
        std::vector<std::thread> threads;
        auto wait_for_begin = []() noexcept {};
        std::barrier sync_point(cores, wait_for_begin);

        auto t_start = std::chrono::high_resolution_clock::now();
        std::function<void(int)> func = [&] (const int thread_id) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(1, 3);
            sync_point.arrive_and_wait();
            std::string* foo = new std::string();
            for (int i = 0; i < operationsByThread; i++) {
                for (int j = 0; j < 40; j = j + distrib(gen)) {}
                *foo = std::to_string(i) + "\n";
                queue.enqueue(foo, thread_id);
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
                std::cerr << "Error calling pthread_setaffinity_np" << rc << "\n";
            }
        }

        for (std::thread &th: threads) {
            if (th.joinable()) {
                th.join();
            }
        }
        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();


        for (int i = 0; i < operationsByThread * cores; i++) {
            queue.dequeue(0);
        }
        // std::cout << "Finished: " << &queue << " | " << queue.dequeue(0) << std::endl;
        return duration;
    }

    template<typename Queue>
    long only_deq_test(int cores, Queue &queue, int operationsByThread) {
        std::vector<std::thread> threads;
        int totalOps = cores * operationsByThread;
        // int rest = totalOps % 100;
        // totalOps += rest;
        auto wait_for_begin = []() noexcept {};
        std::barrier sync_point(cores, wait_for_begin);
        std::string *foo = new std::string();
        for (int i = 0; i < totalOps; i++) {
            *foo = std::to_string(i) + "\n";
            queue.enqueue(foo, 0);
        }

        std::function<void(int)> func = [&] (const int thread_id) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(1, 3);
            sync_point.arrive_and_wait();
            for (int i = 0; i < operationsByThread; i++) {
                for (int j = 0; j < 40; j = j + distrib(gen)) {}
                try {
                    queue.dequeue(thread_id);
                } catch (const std::runtime_error &e) {
                    std::cout << "Error al desencolar " << e.what() << std::endl;
                }
            }
        };
        auto t_start = std::chrono::high_resolution_clock::now();
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
        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();
        delete foo;
        return duration;
    };

    double calculate_mean(std::deque<double> data) {
        int size = data.size();
        double sum = 0;
        for (int i = 0; i < size; i++) sum += data.at(i);
        return sum / size;

    };

    double calculate_deviation(std::deque<double> data) {
        int size = data.size() >= 30 ? data.size() : data.size() - 1;
        double mean = calculate_mean(data);
        double sum = 0;
        for (int i = 0; i < size; i++) sum += std::pow((data.at(i) - mean), 2);
        return std::sqrt(sum/size);
    };

    class Window {
    private:
        std::deque<double> data;
        std::size_t k;

    public:
        Window(std::size_t k) : k(k) {}

        void addValue(double value) {
            if (data.size() < k) {
                data.push_back(value);
            } else {
                data.pop_front();
                data.push_back(value);
            }
        }

        double mean() {
            return calculate_mean(data);
        }

        double standard_deviation() {
            return calculate_deviation(data);
        }
    };


    // Arithmetic mean obtained from the coefficient of variation

    // To calculate this mean, we need to have an experiment
    // =evaluation()= that returns a long value. The pseudocode is the following:
    template<typename Queue>
    long meanFromCoV(std::size_t cores, int operationsByThread) {
        Window w{K};
        // double smallS = std::numeric_limits<double>::max();
        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;
        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            Queue *queue = new Queue(cores);
            execTime = same_number_enq_deq_test(cores, *queue, operationsByThread);
            delete queue;
            w.addValue(execTime);
            if (i > K) {
                double s = w.standard_deviation();
                double x = w.mean();
                double cov = s / x;
                if (cov < 0.05) {
                    return x;
                }
                if (smallCoV > cov) {
                    smallCoV = cov;
                    smallX = x;
                    // smallS = s;
                }
            }
        }
        return smallX;
    };


    template <typename Queue>
    long meanFromCoVOnlyEnq(std::size_t cores, int operationsByThread) {
        Window w{K};
        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;
        Queue *queue;
        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            queue = new Queue(cores);
            execTime = only_enq_test(cores, *queue, operationsByThread);
            delete queue;
            w.addValue(execTime);
            if (i > K) {
                double s = w.standard_deviation();
                double x = w.mean();
                double CoV = s / x;
                if (CoV < 0.05) {
                    return x;
                }
                if (smallCoV > CoV) {
                    smallCoV = CoV;
                    smallX = x;
                }
            }
        }

        return smallX;
    };


    template<typename Queue>
    long meanFromCoVOnlyDeq(std::size_t cores, int operationsByThread) {
        Window w{K};
        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;
        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            Queue queue{cores};
            execTime = only_deq_test(cores, queue, operationsByThread);
            w.addValue(execTime);
            if (i > K) {
                double s = w.standard_deviation();
                double x = w.mean();
                double CoV = s / x;
                if (CoV < 0.05) {
                    return x;
                }
                if (smallCoV > CoV) {
                    smallCoV = CoV;
                    smallX = x;
                }
            }
        }
        return smallX;
    };

    template<typename Queue>
    long meanFromCoV2(std::size_t cores, int operationsByThread, std::function<long(int, Queue, int)> func) {
        Window w{K};
        // double smallS = std::numeric_limits<double>::max();
        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;
        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            Queue queue{cores};
            execTime = func(cores, queue, operationsByThread);
            w.addValue(execTime);
            if (i > K) {
                double s = w.standard_deviation();
                double x = w.mean();
                double cov = s / x;

                if (cov < 0.05) {
                    return x;
                }
                if (smallCoV > cov) {
                    smallCoV = cov;
                    smallX = x;
                    // smallS = s;
                }
            }
        }
        return smallX;
    };


    // Four-step methodology

    // - Consider p invocations of evaluation running at most q
    //   benchmark iterations. We want to retain k measurements per
    //   invocation.

    // - For each evaluation invocation i, determine the iteration
    //   where steady-state performance is reached.

    // - For each evalution invocation, compute the mean \bar{x}_i of
    //   the k benchmark iterations under of steady-state

    // - Compute the confidence interval for a given confidence level
    //   across the computed means from the different evaluation
    //   invocations. The overall means \bar{x} = \sum_{i = 1}^p
    //   \bar{x}_i and the confidence interval is computed over the
    //   \bar{x}_i measurements.

    /**
     * Considering i cores, we return the P invocations of enq_deq of
     * the queue Queue.
     */
    template<typename Queue>
    std::vector<long> invocation(std::size_t cores, int operationsByThread) {
        std::vector<long> results;
        long result = 0;
        std::cout << "Cores: " << cores << "; operations: " << operationsByThread << std::endl;
        for(uintmax_t i = 0; i < P; i++) {
            result = meanFromCoV<Queue>(cores, operationsByThread);
            results.push_back(result);
        }
        return results;
    };

    template<typename Queue>
    std::vector<long> invocationOnlyEnq(std::size_t cores, int operationsByThread) {
        std::vector<long> results;
        long result = 0;
        std::cout << "Cores: " << cores << "; operations: " << operationsByThread << std::endl;
        for(uintmax_t i = 0; i < P; i++) {
            result = meanFromCoVOnlyEnq<Queue>(cores, operationsByThread);
            results.push_back(result);
        }
        return results;
    };

    template<typename Queue>
    std::vector<long> invocationOnlyDeq(std::size_t cores, int operationsByThread) {
        std::vector<long> results;
        long result = 0;
        std::cout << "Cores: " << cores << "; operations: " << operationsByThread << std::endl;
        for (uintmax_t i = 0; i < P; i++) {
            result = meanFromCoVOnlyDeq<Queue>(cores, operationsByThread);
            results.push_back(result);
        }
        return results;
    };

    /**
     * We return a map, where each entry corresponds with the
     * evaluation at the core i, and the result is a vector with the P
     * invocations of enq_deq experiment of the queue Queue.
     */
    template<typename Queue>
    json experiment(int cores, int operations) {
        std::map<std::string, std::vector<long>> results;
        json exp_json;
        for (int i = 0; i < cores; i++) {
            std::size_t total_cores = i + 1;
            int total_ops = operations / (i  + 1);
            // std::cout << "Queue experiment " << i + 1<< std::endl;
            exp_json[std::to_string(total_cores)] = invocation<Queue>(total_cores, total_ops);
            // results[std::to_string(total_cores)] = invocation<Queue>(total_cores, total_ops);
        }
        return exp_json;
    };

    template<typename Queue>
    json experimentOnlyEnq(int cores, int operations) {
        json exp_json;
        for(int i = 0; i < cores; i++) {
            std::size_t total_cores = i + 1;
            int total_ops = operations / (i + 1);
            exp_json[std::to_string(total_cores)] = invocationOnlyEnq<Queue>(total_cores, total_ops);
        }
        return exp_json;
    };

    template<typename Queue>
    json experimentOnlyDeq(int cores, int operations) {
        json exp_json;
        for (int i = 0; i < cores; i++) {
            std::size_t total_cores = i + 1;
            int total_ops = operations / (i + 1);
            exp_json[std::to_string(total_cores)] = invocationOnlyDeq<Queue>(total_cores, total_ops);
        }
        return exp_json;
    };


    void exp_json(std::string name, json alg_results) {
        json results;
        results["algorithm"] = name;
        results["results"] = alg_results;
        std::cout << std::setw(4) << results << std::endl;
        std::time_t currTime;
        std::tm* currTm;
        std::time(&currTime);
        currTm = std::localtime(&currTime);
        char buffer[256];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H:%M:%S", currTm);
        std::string fileName = "results/" + std::string(buffer) + "__" +name + "_test_enq_deq_time.json";
        std::ofstream file(fileName);
        file << std::setw(4) << results << std::endl;
        file.close();
        std::cout << fileName << std::endl;
    };

    void exp_json_only_enq(std::string name, json alg_results) {
        json results;
        results["algorithm"] = name;
        results["results"] = alg_results;
        std::cout << std::setw(4) << results << std::endl;
        std::time_t currTime;
        std::tm* currTm;
        std::time(&currTime);
        currTm = std::localtime(&currTime);
        char buffer[256];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H:%M:%S", currTm);
        std::string fileName = "results/" + std::string(buffer) + "__" + name + "_test_only_enq_time.json";
        std::ofstream file(fileName);
        file << std::setw(4) << results << std::endl;
        file.close();
        std::cout << fileName << std::endl;
    };

    void exp_json_only_deq(std::string name, json alg_results) {
        json results;
        results["algorithm"] = name;
        results["results"] = alg_results;
        std::cout << std::setw(4) << results << std::endl;
        std::time_t currTime;
        std::tm* currTm;
        std::time(&currTime);
        currTm = std::localtime(&currTime);
        char buffer[256];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H:%M:%S", currTm);
        std::string fileName = "results/" + std::string(buffer) + "__" + name + "_test_only_deq_time.json";
        std::ofstream file(fileName);
        file << std::setw(4) << results << std::endl;
        file.close();
        std::cout << fileName << std::endl;
    };



    void experiments() {
        const auto cores = std::thread::hardware_concurrency();
        // std::map<std::string, std::map<std::string, std::vector<long>>> results;
        std::cout << "\n\nEnqueue-dequeue experiment with " << cores << " and 1'000'000 operations\n\n";
        int operations = 1'000'000;
        // std::cout << "\n\nFAA-QUEUE\n\n";
        // exp_json("FAAQUEUE", experiment<faa_array::Queue<std::string>>(cores, operations));
        // std::cout << "\n\nMS-QUEUE\n\n";
        // exp_json("MSQUEUE", experiment<ms_queue::Queue<std::string>>(cores, operations));
        // std::cout << "\n\nLCRQ-QUEUE\n\n";
        // exp_json("LCRQQUEUE", experiment<lcrq_queue::Queue<std::string>>(cores, operations));
        // std::cout << "\n\nYMC-QUEUE\n\n";
        // exp_json("YMCQUEUE", experiment<ymc_queue::Queue<std::string>>(cores, operations));
        // std::cout << "\n\nSBQ-QUEUE\n\n";
        // exp_json("SBQQUEUE", experiment<scal_basket_queue::Queue<std::string>>(cores, operations));
        // std::cout << "\n\nLLIC-QUEUE\n\n";
        // exp_json("LLICQUEUE", experiment<llic_queue::FAIQueue<std::string, llic_queue::LLICCAS, llic_queue::KBasketFAI<std::string, 4>, 1000000>>(cores, operations));
        std::cout << "\n\nLLIC-Queue-Segments\n\n";
        exp_json("LLICQUEUE_SEGMENT", experiment<llic_queue::Queue<std::string, llic_queue::LLICCAS, llic_queue::KBasketFAI<std::string, 4>>>(cores, operations));
        std::cout << "\n\nLLIC-Queue-Array\n\n";
        exp_json("LLICQUEUE_ARRAY", experiment<llic_queue::FAIQueueArray<std::string, llic_queue::LLICCAS, llic_queue::KBasketFAI<std::string, 4>>>(cores, operations));
    };

    void experiments_only_enq() {
        const auto cores = 4;//std::thread::hardware_concurrency();
        // std::map<std::string, std::map<std::string, std::vector<long>>> results;
        std::cout << "\n\nOnly enqueue experiment with " << cores << " and 1'000'000 operations\n\n";
        int operations = 1'000'000;
        // std::cout << "\n\nFAA-QUEUE\n\n";
        // exp_json_only_enq("FAAQUEUE", experimentOnlyEnq<faa_array::Queue<std::string>>(cores, operations));
        // std::cout << "\n\nMS-QUEUE\n\n";
        // exp_json_only_enq("MSQUEUE", experimentOnlyEnq<ms_queue::Queue<std::string>>(cores, operations));
        // std::cout << "\n\nLCRQ-QUEUE\n\n";
        // exp_json_only_enq("LCRQQUEUE", experimentOnlyEnq<lcrq_queue::Queue<std::string>>(cores, operations));
        // std::cout << "\n\nYMC-QUEUE\n\n";
        // exp_json_only_enq("YMCQUEUE", experimentOnlyEnq<ymc_queue::Queue<std::string>>(cores, operations));
        // std::cout << "\n\nSBQ-QUEUE\n\n";
        // exp_json_only_enq("SBQQUEUE", experimentOnlyEnq<scal_basket_queue::Queue<std::string>>(cores, operations));
        // std::cout << "\n\nLLIC-QUEUE\n\n";
        // exp_json_only_enq("LLICQUEUE", experimentOnlyEnq<llic_queue::FAIQueue<std::string, llic_queue::LLICCAS, llic_queue::KBasketFAI<std::string, 4>, 1000000>>(cores, operations));
        std::cout << "\n\nLLIC-Queue-Segments\n\n";
        exp_json_only_enq("LLICQUEUE_SEGMENT", experimentOnlyEnq<llic_queue::Queue<std::string, llic_queue::LLICCAS, llic_queue::KBasketFAI<std::string, 4>>>(cores, operations));
        std::cout << "\n\nLLIC-Queue-Array\n\n";
        exp_json_only_enq("LLICQUEUE_ARRAY", experimentOnlyEnq<llic_queue::FAIQueueArray<std::string, llic_queue::LLICCAS, llic_queue::KBasketFAI<std::string, 4>>>(cores, operations));
    };

    void experiments_only_deq() {
        const auto cores = std::thread::hardware_concurrency();
        std::map<std::string, std::map<std::string, std::vector<long>>> results;
        std::cout << "\n\nOnly dequeue experiment with " << cores << " and 1'000'000 operations\n\n";
        int operations = 1'000'000;
        // std::cout << "\n\nFAA-QUEUE\n\n";
        // exp_json_only_deq("FAAQUEUE", experimentOnlyDeq<faa_array::Queue<std::string>>(cores, operations));
        // std::cout << "\n\nMS-QUEUE\n\n";
        // exp_json_only_deq("MSQUEUE", experimentOnlyDeq<ms_queue::Queue<std::string>>(cores, operations));
        // std::cout << "\n\nLCRQ-QUEUE\n\n";
        // exp_json_only_deq("LCRQQUEUE", experimentOnlyDeq<lcrq_queue::Queue<std::string>>(cores, operations));
        // std::cout << "\n\nYMC-QUEUE\n\n";
        // exp_json_only_deq("YMCQUEUE", experimentOnlyDeq<ymc_queue::Queue<std::string>>(cores, operations));
        // std::cout << "\n\nSBQ-QUEUE\n\n";
        // exp_json_only_deq("SBQQUEUE", experimentOnlyDeq<scal_basket_queue::Queue<std::string>>(cores, operations));
        // std::cout << "\n\nLLIC-QUEUE\n\n";
        // exp_json_only_deq("LLICQUEUE", experimentOnlyDeq<llic_queue::FAIQueue<std::string, llic_queue::LLICCAS, llic_queue::KBasketFAI<std::string, 4>, 1000000>>(cores, operations));
        std::cout << "\n\nLLIC-QUEUE-Segments\n\n";
        exp_json_only_deq("LLICQUEUE_SEGMENT", experimentOnlyDeq<llic_queue::Queue<std::string, llic_queue::LLICCAS, llic_queue::KBasketFAI<std::string, 4>>>(cores, operations));
        std::cout << "\n\nLLIC-QUEUE-Array\n\n";
        exp_json_only_deq("LLICQUEUE_ARRAY", experimentOnlyDeq<llic_queue::FAIQueueArray<std::string, llic_queue::LLICCAS, llic_queue::KBasketFAI<std::string, 4>>>(cores, operations));
    };


}
#endif
