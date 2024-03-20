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
#include <random>
#include <cstdlib>
#include "include/llic.hpp"
#include "include/basket_queue.hpp"
#include "include/utils.hpp"
#include "nlohmann/json.hpp"


using json = nlohmann::json;
using namespace std::chrono_literals;

namespace exp_queue {

    static constexpr uintmax_t ITERATIONS = 75;
    static constexpr uintmax_t K = 5;
    static constexpr uintmax_t P = 30;

    double calculate_mean(std::deque<double> data) {
        int size = data.size();
        double sum = 0;
        for (int i = 0; i < size; i++) sum += data.at(i);
        return sum / size;
    }

    double calculate_deviation(std::deque<double> data) {
        int size = data.size() >= 30 ? data.size() : data.size() - 1;
        double mean = calculate_mean(data);
        double sum = 0;
        for (int i = 0; i < size; i++) sum += std::pow((data.at(i) - mean), 2);
        return std::sqrt(sum / size);
    }

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

    long enq_deq_cas_cas(int cores, int operations) {
        // std::cout << "\nPerforming " << operations << " operations by Thread. All operations are evenly distributed between all threads. CAS-CAS." << std::endl;
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

        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();
        return duration;
    }

    long enq_deq_cas_fai(int cores, int operations) {
        // std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads. CAS-FAI." << std::endl;
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
        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();
        return duration;
    }

    long enq_deq_rw_cas(int cores, int operations) {
        // std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads. RW-CAS." << std::endl;
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

        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();
        return duration;
    }

    long enq_deq_rw16_cas(int cores, int operations) {
        // std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads. RW16-CAS." << std::endl;
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
        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();
        return duration;
    }

    long enq_deq_rw_fai(int cores, int operations) {
        // std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads. RW-FAI." << std::endl;
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
        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();
        return duration;
    }

    long enq_deq_rw16_fai(int cores, int operations) {
        // std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads. RW16-FAI." << std::endl;
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
        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();
        return duration;
    }


    long enq_deq_grouped16_fai(int cores, int operations) {
        // std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads. RWG16-FAI." << std::endl;
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
        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();
        return duration;
    }

    long enq_deq_grouped16_cas(int cores, int operations) {
        // std::cout << "\nPerforming " << operations << " operations. All operations are evenly distributed between all threads. RWG16-CAS." << std::endl;
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
        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();
        return duration;
    }

    long mean_cas_cas_from_cov(std::size_t cores, int operations) {
        Window w{K};

        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;

        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            execTime = enq_deq_cas_cas(cores, operations);
            w.addValue(execTime);
            if (i > K) {
                double s   = w.standard_deviation();
                double x   = w.mean();
                double cov = s / x;
                if (cov < 0.02) {
                    return x;
                }
                if (smallCoV > cov) {
                    smallCoV = cov;
                    smallX = x;
                }
            }
        }
        return smallX;
    }

    std::vector<long> invocation_cas_cas(std::size_t cores, int operations) {
        std::vector<long> results;
        long result = 0;
        std::cout << "Cores: " << cores << "; operations: " << operations / cores << std::endl;
        for (uintmax_t i = 0; i < P; i++) {
            result = mean_cas_cas_from_cov(cores, operations);
            results.push_back(result);
        }
        return results;
    }

    json experiment_cas_cas(int cores, int operations) {
        json exp_json;
        for (int i = 0; i < cores; i++) {
            std::size_t total_cores = i + 1;
            // int total_ops = operations / (i + 1);
            exp_json[std::to_string(total_cores)] = invocation_cas_cas(total_cores, operations);
        }
        return exp_json;
    }

    long mean_cas_fai_from_cov(std::size_t cores, int operations) {
        Window w{K};

        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;

        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            execTime = enq_deq_cas_fai(cores, operations);
            w.addValue(execTime);
            if (i > K) {
                double s   = w.standard_deviation();
                double x   = w.mean();
                double cov = s / x;
                if (cov < 0.02) {
                    return x;
                }
                if (smallCoV > cov) {
                    smallCoV = cov;
                    smallX = x;
                }
            }
        }
        return smallX;
    }

    std::vector<long> invocation_cas_fai(std::size_t cores, int operations) {
        std::vector<long> results;
        long result = 0;
        std::cout << "Cores: " << cores << "; operations: " << operations << std::endl;
        for (uintmax_t i = 0; i < P; i++) {
            result = mean_cas_fai_from_cov(cores, operations);
            results.push_back(result);
        }
        return results;
    }

    json experiment_cas_fai(int cores, int operations) {
        json exp_json;
        for (int i = 0; i < cores; i++) {
            std::size_t total_cores = i + 1;
            // int total_ops = operations / (i + 1);
            exp_json[std::to_string(total_cores)] = invocation_cas_fai(total_cores, operations);
        }
        return exp_json;
    }


    long mean_rw_cas_from_cov(std::size_t cores, int operations) {
        Window w{K};

        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;

        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            execTime = enq_deq_rw_cas(cores, operations);
            w.addValue(execTime);
            if (i > K) {
                double s   = w.standard_deviation();
                double x   = w.mean();
                double cov = s / x;
                if (cov < 0.02) {
                    return x;
                }
                if (smallCoV > cov) {
                    smallCoV = cov;
                    smallX = x;
                }
            }
        }
        return smallX;
    }

    std::vector<long> invocation_rw_cas(std::size_t cores, int operations) {
        std::vector<long> results;
        long result = 0;
        std::cout << "Cores: " << cores << "; operations: " << operations << std::endl;
        for (uintmax_t i = 0; i < P; i++) {
            result = mean_rw_cas_from_cov(cores, operations);
            results.push_back(result);
        }
        return results;
    }

    json experiment_rw_cas(int cores, int operations) {
        json exp_json;
        for (int i = 0; i < cores; i++) {
            std::size_t total_cores = i + 1;
            // int total_ops = operations / (i + 1);
            exp_json[std::to_string(total_cores)] = invocation_rw_cas(total_cores, operations);
        }
        return exp_json;
    }

    long mean_rw16_cas_from_cov(std::size_t cores, int operations) {
        Window w{K};

        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;

        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            execTime = enq_deq_rw16_cas(cores, operations);
            w.addValue(execTime);
            if (i > K) {
                double s   = w.standard_deviation();
                double x   = w.mean();
                double cov = s / x;
                if (cov < 0.02) {
                    return x;
                }
                if (smallCoV > cov) {
                    smallCoV = cov;
                    smallX = x;
                }
            }
        }
        return smallX;
    }

    std::vector<long> invocation_rw16_cas(std::size_t cores, int operations) {
        std::vector<long> results;
        long result = 0;
        std::cout << "Cores: " << cores << "; operations: " << operations << std::endl;
        for (uintmax_t i = 0; i < P; i++) {
            result = mean_rw16_cas_from_cov(cores, operations);
            results.push_back(result);
        }
        return results;
    }

    json experiment_rw16_cas(int cores, int operations) {
        json exp_json;
        for (int i = 0; i < cores; i++) {
            std::size_t total_cores = i + 1;
            // int total_ops = operations / (i + 1);
            exp_json[std::to_string(total_cores)] = invocation_rw16_cas(total_cores, operations);
        }
        return exp_json;
    }

    long mean_rw_fai_from_cov(std::size_t cores, int operations) {
        Window w{K};

        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;

        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            execTime = enq_deq_rw_fai(cores, operations);
            w.addValue(execTime);
            if (i > K) {
                double s   = w.standard_deviation();
                double x   = w.mean();
                double cov = s / x;
                if (cov < 0.02) {
                    return x;
                }
                if (smallCoV > cov) {
                    smallCoV = cov;
                    smallX = x;
                }
            }
        }
        return smallX;
    }

    std::vector<long> invocation_rw_fai(std::size_t cores, int operations) {
        std::vector<long> results;
        long result = 0;
        std::cout << "Cores: " << cores << "; operations: " << operations << std::endl;
        for (uintmax_t i = 0; i < P; i++) {
            result = mean_rw_fai_from_cov(cores, operations);
            results.push_back(result);
        }
        return results;
    }

    json experiment_rw_fai(int cores, int operations) {
        json exp_json;
        for (int i = 0; i < cores; i++) {
            std::size_t total_cores = i + 1;
            // int total_ops = operations / (i + 1);
            exp_json[std::to_string(total_cores)] = invocation_rw_fai(total_cores, operations);
        }
        return exp_json;
    }

    long mean_rw16_fai_from_cov(std::size_t cores, int operations) {
        Window w{K};

        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;

        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            execTime = enq_deq_rw16_fai(cores, operations);
            w.addValue(execTime);
            if (i > K) {
                double s   = w.standard_deviation();
                double x   = w.mean();
                double cov = s / x;
                if (cov < 0.02) {
                    return x;
                }
                if (smallCoV > cov) {
                    smallCoV = cov;
                    smallX = x;
                }
            }
        }
        return smallX;
    }

    std::vector<long> invocation_rw16_fai(std::size_t cores, int operations) {
        std::vector<long> results;
        long result = 0;
        std::cout << "Cores: " << cores << "; operations: " << operations << std::endl;
        for (uintmax_t i = 0; i < P; i++) {
            result = mean_rw16_fai_from_cov(cores, operations);
            results.push_back(result);
        }
        return results;
    }

    json experiment_rw16_fai(int cores, int operations) {
        json exp_json;
        for (int i = 0; i < cores; i++) {
            std::size_t total_cores = i + 1;
            // int total_ops = operations / (i + 1);
            exp_json[std::to_string(total_cores)] = invocation_rw16_fai(total_cores, operations);
        }
        return exp_json;
    }

    long mean_grouped16_fai_from_cov(std::size_t cores, int operations) {
        Window w{K};

        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;

        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            execTime = enq_deq_grouped16_fai(cores, operations);
            w.addValue(execTime);
            if (i > K) {
                double s   = w.standard_deviation();
                double x   = w.mean();
                double cov = s / x;
                if (cov < 0.02) {
                    return x;
                }
                if (smallCoV > cov) {
                    smallCoV = cov;
                    smallX = x;
                }
            }
        }
        return smallX;
    }

    std::vector<long> invocation_grouped16_fai(std::size_t cores, int operations) {
        std::vector<long> results;
        long result = 0;
        std::cout << "Cores: " << cores << "; operations: " << operations << std::endl;
        for (uintmax_t i = 0; i < P; i++) {
            result = mean_grouped16_fai_from_cov(cores, operations);
            results.push_back(result);
        }
        return results;
    }

    json experiment_grouped16_fai(int cores, int operations) {
        json exp_json;
        for (int i = 0; i < cores; i++) {
            std::size_t total_cores = i + 1;
            // int total_ops = operations / (i + 1);
            exp_json[std::to_string(total_cores)] = invocation_grouped16_fai(total_cores, operations);
        }
        return exp_json;
    }

    long mean_grouped16_cas_from_cov(std::size_t cores, int operations) {
        Window w{K};

        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;

        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            execTime = enq_deq_grouped16_cas(cores, operations);
            w.addValue(execTime);
            if (i > K) {
                double s   = w.standard_deviation();
                double x   = w.mean();
                double cov = s / x;
                if (cov < 0.02) {
                    return x;
                }
                if (smallCoV > cov) {
                    smallCoV = cov;
                    smallX = x;
                }
            }
        }
        return smallX;
    }

    std::vector<long> invocation_grouped16_cas(std::size_t cores, int operations) {
        std::vector<long> results;
        long result = 0;
        std::cout << "Cores: " << cores << "; operations: " << operations << std::endl;
        for (uintmax_t i = 0; i < P; i++) {
            result = mean_grouped16_cas_from_cov(cores, operations);
            results.push_back(result);
        }
        return results;
    }

    json experiment_grouped16_cas(int cores, int operations) {
        json exp_json;
        for (int i = 0; i < cores; i++) {
            std::size_t total_cores = i + 1;
            // int total_ops = operations / (i + 1);
            exp_json[std::to_string(total_cores)] = invocation_grouped16_cas(total_cores, operations);
        }
        return exp_json;
    }

    void to_JSON(std::string name, json alg_results) {
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
        std::string fileName = "results/" + std::string(buffer) + "__" + name + "_inner_queue_experiments.json";
        std::ofstream file(fileName);
        file << std::setw(4) << results << std::endl;
        file.close();
        std::cout << fileName << std::endl;
    }

    void experiments() {
        const auto cores = std::thread::hardware_concurrency();
        std::cout << "Inner queue experiments with " << cores << " and 1'000'000 operations\n\n";
        int operations = 1'000'000;
        std::cout << "\n\n LLIC CAS Basket CAS queue\n\n";
        to_JSON("CAS_CAS_QUEUE", experiment_cas_cas(cores, operations));
        std::cout << "\n\n LLIC CAS Basket FAI queue\n\n";
        to_JSON("CAS_FAI_QUEUE", experiment_cas_fai(cores, operations));
        std::cout << "\n\n LLIC RW Basket CAS queue\n\n";
        to_JSON("RW_CAS_QUEUE", experiment_rw_cas(cores, operations));
        std::cout << "\n\n LLIC RW16 Basket CAS queue\n\n";
        to_JSON("RW16_CAS_QUEUE", experiment_rw16_cas(cores, operations));
        std::cout << "\n\n LLIC RW Basket FAI queue\n\n";
        to_JSON("RW_FAI_QUEUE", experiment_rw_fai(cores, operations));
        std::cout << "\n\n LLIC RW16 Basket FAI queue\n\n";
        to_JSON("RW16_FAI_QUEUE", experiment_rw16_fai(cores, operations));
        std::cout << "\n\n LLIC SQRT grouped 16 bytes padding Basket FAI queue\n\n";
        to_JSON("RWSQRT16_FAI_QUEUE", experiment_grouped16_fai(cores, operations));
        std::cout << "\n\n LLIC SQRT grouped 16 bytes padding Basket CAS queue\n\n";
        to_JSON("RWSQRT16_CAS_QUEUE", experiment_grouped16_fai(cores, operations));
    }

}
