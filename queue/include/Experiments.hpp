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
#include <deque>
#include <cmath>
#include "nlohmann/json.hpp"
#include "include/llic.hpp"
#include "include/utils.hpp"


using json = nlohmann::json;
using namespace std::chrono_literals;

namespace exp_llic {

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

    long exp_FAI(int cores, int operationsByThread) {
        auto t_start = std::chrono::high_resolution_clock::now();
        std::atomic_int FAI = 0;
        std::vector<std::thread> vecOfThreads;
        auto wait_for_begin = []() noexcept {};
        std::barrier sync_point(cores, wait_for_begin);
        std::function<void()> func = [&] () {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(1, 3);
            sync_point.arrive_and_wait();
            for (int i = 0; i < operationsByThread; i++) {
                FAI++;
                for (int j = 0; j < 80; j = j + distrib(gen));
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
        for (std::thread &th: vecOfThreads) {
            if (th.joinable()) {
                th.join();
            }
        }

        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<long, std::nano>(t_end - t_start).count();
        return duration;
    }

    template<typename LLIC>
    long exp_LLIC(int cores, int operationsByThread) {
        auto t_start = std::chrono::high_resolution_clock::now();
        LLIC llic;
        std::vector<std::thread> vecOfThreads;
        auto wait_for_begin = []() noexcept {};
        std::barrier sync_point(cores, wait_for_begin);
        std::function<void()> func = [&]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(1, 3);
            sync_point.arrive_and_wait();
            int max = 0;
            for (int i = 0; i < operationsByThread; ++i) {
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
        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
        return duration;
    }

    template<typename LLIC>
    long exp_LLIC_2_params(int cores, int operationsByThread) {
        auto t_start = std::chrono::high_resolution_clock::now();
        LLIC llic(cores);
        std::vector<std::thread> vecOfThreads;
        auto wait_for_begin = []() noexcept {};
        std::barrier sync_point(cores, wait_for_begin);
        std::function<void(int)> func = [&](int processID) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(1, 3);
            sync_point.arrive_and_wait();
            int max = 0;
            for (int i = 0; i < operationsByThread; ++i) {
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
        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
        return duration;
    }

    template<typename LLIC>
    long exp_LLIC_SQRT(int cores, int operationsByThread) {
        auto t_start = std::chrono::high_resolution_clock::now();
        LLIC llic(cores);
        std::vector<std::thread> vecOfThreads;
        auto wait_for_begin = []() noexcept {};
        std::barrier sync_point(cores, wait_for_begin);
        std::function<void(int)> func = [&](int processID) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(1, 3);
            sync_point.arrive_and_wait();
            int max_p = 0;
            int ind_max_p;
            for (int i = 0; i < operationsByThread; ++i) {
                max_p = llic.LL(ind_max_p);
                for (int j = 0; j < 40; j = j + distrib(gen)) {}
                llic.IC(max_p, ind_max_p,processID);
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
        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
        return duration;
    }


    template<typename LLIC>
    long exp_LLIC_SQRTG(int cores, int operationsByThread, int group_size) {
        // std::cout << "\nPerforming " << operationsByThread << " of operations by thread in " << cores << " cores. Each thread do the total divided by the number of thread. Case of Fetch&Increment. (With delay)" << std::endl;
        // std::clock_t c_start = std::clock();
        auto t_start = std::chrono::high_resolution_clock::now();
        LLIC llic(cores, group_size);
        std::vector<std::thread> vecOfThreads;
        auto wait_for_begin = []() noexcept {};
        std::barrier sync_point(cores, wait_for_begin);
        std::function<void(int)> func = [&](int processID) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(1, 3);
            sync_point.arrive_and_wait();
            int max_p = 0;
            int ind_max_p;
            for (int i = 0; i < operationsByThread; ++i) {
                max_p = llic.LL(ind_max_p);
                for (int j = 0; j < 40; j = j + distrib(gen)) {}
                llic.IC(max_p, ind_max_p,processID);
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
        // std::clock_t c_end = std::clock();
        auto t_end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<long, std::nano>(t_end-t_start).count();
        // print_time((c_end - c_start), duration, llic.LL());
        return duration;
    }

    long meanLLICFAIFromCoV(std::size_t cores, int operationsByThread) {
        Window w{K};

        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;

        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            execTime = exp_FAI(cores, operationsByThread);
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

    template<typename LLIC>
    long meanLLICFromCov(std::size_t cores, int operationsByThread) {
        Window w{K};

        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;

        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            execTime = exp_LLIC<LLIC>(cores, operationsByThread);
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

    template<typename LLIC>
    long meanLLIC2PFromCov(std::size_t cores, int operationsByThread) {
        Window w{K};

        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;

        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            execTime = exp_LLIC_2_params<LLIC>(cores, operationsByThread);
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

    template<typename LLIC>
    long meanLLICSQRTFromCov(std::size_t cores, int operationsByThread) {
        Window w{K};

        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;

        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            execTime = exp_LLIC_SQRT<LLIC>(cores, operationsByThread);
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

    template<typename LLIC>
    long meanLLICSQRTGFromCov(std::size_t cores, int operationsByThread, int groupSize) {
        Window w{K};

        double smallX = std::numeric_limits<double>::max();
        double smallCoV = std::numeric_limits<double>::max();
        long execTime;

        for (uintmax_t i = 0; i < ITERATIONS; i++) {
            execTime = exp_LLIC_SQRTG<LLIC>(cores, operationsByThread, groupSize);
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

    std::vector<long> invocationFAI(std::size_t cores, int operationsByThread) {
        std::vector<long> results;
        long result = 0;
        std::cout << "Cores: " << cores << "; operations: " << operationsByThread << std::endl;
        for (uintmax_t i = 0; i < P; i++) {
            result = meanLLICFAIFromCoV(cores, operationsByThread);
            results.push_back(result);
        }
        return results;
    }

    template<typename LLIC>
    std::vector<long> invocationLLIC(std::size_t cores, int operationsByThread) {
        std::vector<long> results;
        long result = 0;
        std::cout << "Cores: " << cores << "; operations: " << operationsByThread << std::endl;
        for (uintmax_t i = 0; i < P; i++) {
            result = meanLLICFromCov<LLIC>(cores, operationsByThread);
            results.push_back(result);
        }
        return results;
    }

    template<typename LLIC>
    std::vector<long> invocationLLIC2P(std::size_t cores, int operationsByThread) {
        std::vector<long> results;
        long result = 0;
        std::cout << "Cores: " << cores << "; operations: " << operationsByThread << std::endl;
        for (uintmax_t i = 0; i < P; i++) {
            result = meanLLIC2PFromCov<LLIC>(cores, operationsByThread);
            results.push_back(result);
        }
        return results;
    }

    template<typename LLIC>
    std::vector<long> invocationLLICSQRT(std::size_t cores, int operationsByThread) {
        std::vector<long> results;
        long result = 0;
        std::cout << "Cores: " << cores << "; operations: " << operationsByThread << std::endl;
        for (uintmax_t i = 0; i < P; i++) {
            result = meanLLICSQRTFromCov<LLIC>(cores, operationsByThread);
            results.push_back(result);
        }
        return results;
    }

    template<typename LLIC>
    std::vector<long> invocationLLICSQRTG(std::size_t cores, int operationsByThread, int groupSize) {
        std::vector<long> results;
        long result = 0;
        std::cout << "Cores: " << cores << "; operations: " << operationsByThread << std::endl;
        for (uintmax_t i = 0; i < P; i++) {
            result = meanLLICSQRTGFromCov<LLIC>(cores, operationsByThread, groupSize);
            results.push_back(result);
        }
        return results;
    }

    json experimentFAI(int cores, int operations) {
        json exp_json;
        for (int i = 0; i < cores; i++) {
            std::size_t total_cores = i + 1;
            int total_ops = operations / (i + 1);
            exp_json[std::to_string(total_cores)] = invocationFAI(total_cores, total_ops);
        }
        return exp_json;
    }

    template<typename LLIC>
    json experimentLLIC(int cores, int operations) {
        json exp_json;
        for (int i = 0; i < cores; i++) {
            std::size_t total_cores = i + 1;
            int total_ops = operations / (i + 1);
            exp_json[std::to_string(total_cores)] = invocationLLIC<LLIC>(total_cores, total_ops);
        }
        return exp_json;
    }

    template<typename LLIC>
    json experimentLLIC2P(int cores, int operations) {
        json exp_json;
        for (int i = 0; i < cores; i++) {
            std::size_t total_cores = i + 1;
            int total_ops = operations / (i + 1);
            exp_json[std::to_string(total_cores)] = invocationLLIC2P<LLIC>(total_cores, total_ops);
        }
        return exp_json;
    }

    template<typename LLIC>
    json experimentLLICSQRT(int cores, int operations) {
        json exp_json;
        for (int i = 0; i < cores; i++) {
            std::size_t total_cores = i + 1;
            int total_ops = operations / (i + 1);
            exp_json[std::to_string(total_cores)] = invocationLLICSQRT<LLIC>(total_cores, total_ops);
        }
        return exp_json;
    }

    template<typename LLIC>
    json experimentLLICSQRTG(int cores, int operations, int groupSize) {
        json exp_json;
        for (int i = 0; i < cores; i++) {
            std::size_t total_cores = i + 1;
            int total_ops = operations / (i + 1);
            exp_json[std::to_string(total_cores)] = invocationLLICSQRTG<LLIC>(total_cores, total_ops, groupSize);
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
        std::string fileName = "results/" + std::string(buffer) + "__" + name + "_LLIC_experiments.json";
        std::ofstream file(fileName);
        file << std::setw(4) << results << std::endl;
        file.close();
        std::cout << fileName << std::endl;
    }

    void experiments() {
        const auto cores = std::thread::hardware_concurrency();
        std::cout << "LL/IC operations experiment wtih " << cores << " and 1'000'000 operations\n\n";
        int operations = 1'000'000;
        std::cout << "\n\nFAI\n\n";
        to_JSON("FAI", experimentFAI(cores, operations));
        std::cout << "\n\nLL/IC CAS\n\n";
        to_JSON("LLICCAS", experimentLLIC<LLICCAS>(cores, operations));

        std::cout << "\n\nLL/IC RW\n\n";
        to_JSON("LLICRW", experimentLLIC2P<LLICRW>(cores, operations));
        std::cout << "\n\nLL/IC RW 16\n\n";
        to_JSON("LLICRW16", experimentLLIC2P<LLICRW16>(cores, operations));
        std::cout << "\n\nLL/IC RW 32\n\n";
        to_JSON("LLICRW32", experimentLLIC2P<LLICRW32>(cores, operations));
        std::cout << "\n\nLL/IC RW 128\n\n";
        to_JSON("LLICRW128", experimentLLIC2P<LLICRW128>(cores, operations));
        std::cout << "\n\nLL/IC RW Without Cycle without false sharing\n\n";
        to_JSON("LLICRWWC", experimentLLIC2P<LLICRWWC>(cores, operations));
        std::cout << "\n\nLL/IC RW without false sharing No padding\n\n";
        to_JSON("LLICRWNP", experimentLLIC2P<LLICRWNP>(cores, operations));
        std::cout << "\n\nLL/IC RW with false sharing without cycle No padding.\n\n";
        to_JSON("LLICRWWCNP", experimentLLIC2P<LLICRWWCNP>(cores, operations));
        std::cout << "\n\nLL/IC RW SQRT with false sharing.\n\n";

        to_JSON("LLICRWSQRT", experimentLLICSQRT<LLICRWSQRT>(cores, operations));
        std::cout << "\n\nLL/IC RW SQRT without false sharing. 64 bytes padding\n\n";
        to_JSON("LLICRWSQRTFS", experimentLLICSQRT<LLICRWSQRTFS>(cores, operations));
        std::cout << "\n\nLL/IC RW SQRT without false sharing. Grouped\n\n";
        to_JSON("LLICRWSQRTG", experimentLLICSQRTG<LLICRWSQRTG>(cores, operations, 4));
        std::cout << "\n\nLL/IC RW SQRT without false sharing. Grouped 16 Bytes padding \n\n";
        to_JSON("LLICRWSQRTG16", experimentLLICSQRTG<LLICRWSQRTG16>(cores, operations, 2));
        std::cout << "\n\nLL/IC RW SQRT without false sharing. Grouped 32 Bytes padding \n\n";
        to_JSON("LLICRWSQRTG16", experimentLLICSQRTG<LLICRWSQRTG32>(cores, operations, 4));

    }

}
