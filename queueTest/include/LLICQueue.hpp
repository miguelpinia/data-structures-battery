#ifndef _LL_IC_QUEUE_HPP
#define _LL_IC_QUEUE_HPP

#include <atomic>
#include <limits>

namespace llic_queue {

    static constexpr int ENQUEUERS = 64;

    enum StateBasket {OPEN, CLOSED};
    enum StatePut {OK, FULL};

    template<typename T>
    constexpr T* empty_ptr() {
        return reinterpret_cast<T*>(std::numeric_limits<uintmax_t>::max());
    }

    template<typename T>
    constexpr T* top_ptr() {
        return reinterpret_cast<T*>(std::numeric_limits<uintmax_t>::max() - 1);
    }

    template<typename T>
    constexpr T* bottom_ptr() {
        return reinterpret_cast<T*>(std::numeric_limits<uintmax_t>::max() - 2);
    }

    template<typename T>
    constexpr T* basket_closed_ptr() {
        return reinterpret_cast<T*>(std::numeric_limits<uintmax_t>::max() - 3);
    }

    template<typename T, std::size_t K>
    class KBasketFAI {
    private:
        std::size_t size_k;
        alignas(64) std::atomic<uintmax_t> puts{0};
        alignas(64) std::atomic<uintmax_t> takes{0};
        alignas(64) std::atomic<StateBasket> state{StateBasket::OPEN};
        std::atomic<T*> A[K];
    public:
        KBasketFAI()  {
            for (unsigned i = 0; i < K; i++) {
                A[i].store(bottom_ptr<T>());
            }
        }

        StatePut put(T* val) {
            StateBasket state;
            std::size_t puts;
            while (true) {
                state = this->state.load();
                puts = this->puts.load();
                if (state == StateBasket::CLOSED || puts >= K) {
                    return StatePut::FULL;
                } else {
                    puts = this->puts.fetch_add(1);
                    if (puts >= K) {
                        return StatePut::FULL;
                    } else if (A[puts].exchange(val) == bottom_ptr<T>()) {
                        return StatePut::OK;
                    }
                }
            }
        }

        T* take() {
            int takes;
            while (true) {
                takes = this->takes.load();
                if (this->state.load() == StateBasket::CLOSED || takes >= K) {
                    return basket_closed_ptr<T>();
                } else {
                    takes = this->takes.fetch_add(1);
                    if (takes >= K) {
                        this->state.store(StateBasket::CLOSED);
                        return basket_closed_ptr<T>();
                    } else {
                        T* val = A[takes].exchange(top_ptr<T>());
                        if (val != bottom_ptr<T>()) return val;
                    }

                }
            }
        }
    };

    class LLICCAS {
    private:
        std::atomic<int> R{0};
    public:
        LLICCAS() {}

        LLICCAS(std::size_t processes) {
            (void) processes;
        }

        int LL() {
            return R.load();
        }

        void IC(int expected) {
            if (R.load() == expected) {
                R.compare_exchange_strong(expected, expected + 1);
            }
        }

        void IC(int expected, std::size_t thread_id) {
            (void) thread_id;
            this->IC(expected);
        }
    };

    template<typename T, typename LLIC, std::size_t K>
    class FAIQueue {
    private:
        std::size_t capacity;
        std::size_t size_k;
        std::size_t maxThreads;
        KBasketFAI<T, K>* A;
        LLIC head;
        LLIC tail;
    public:
        FAIQueue(std::size_t capacity, std::size_t size_k, std::size_t maxThreads) :
            capacity(capacity), size_k(size_k), maxThreads(maxThreads),
           head{maxThreads}, tail{maxThreads} {
            A = new KBasketFAI<T, K>[capacity];
        }

        ~FAIQueue() {
            delete[] A;
        }

        void enqueue(T* val, std::size_t thread_id) {
            int tail;
            while (true) {
                tail = this->tail.LL();
                if (A[tail].put(val) == StatePut::OK) {
                    this->tail.IC(tail, thread_id);
                    return;
                }
                this->tail.IC(tail, thread_id);
            }
        }

        T* dequeue(std::size_t thread_id) {
            int head = this->head.LL();
            int tail = this->tail.LL();
            T* val = nullptr;
            while (true) {
                if (head < tail) {
                    if (val != basket_closed_ptr<T>()) {
                        return val;
                    }
                    this->head.IC(head, thread_id);
                }
                auto hhead = this->head.LL();
                auto ttail = this->tail.LL();
                if (hhead == head && ttail == tail) {
                    return nullptr;
                }
                head = hhead;
                tail = ttail;
            }
        }
    };

    template<typename T, typename LLIC>
    class FAIQueueHP {
    private:
        std::size_t capacity;
        std::size_t size_k;
        std::size_t maxThreads;
    };



    template<typename T>
    class QUeue {
    };
}

#endif
