#ifndef _LL_IC_QUEUE_HPP
#define _LL_IC_QUEUE_HPP

#include <atomic>
#include <limits>
#include <array>
#include <iostream>
#include "MemoryManagementPool.hpp"

namespace llic_queue {

    static constexpr int NODE_POW = 10;
    static constexpr int ENQUEUERS = 64;
    static constexpr std::size_t NODE_SIZE = 1ull << NODE_POW;

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

    template<typename T, int K>
    class KBasketFAI {
     private:
        alignas(128) std::atomic<StateBasket> STATE{StateBasket::OPEN}; // 1 bytes
        alignas(128) std::atomic<int> PUTS; // 8 bytes
        alignas(128) std::atomic<int> TAKES; // 8 bytes
        std::atomic<T*> A[K]; // 8 bytes
    public:
        KBasketFAI(): PUTS{0}, TAKES{0}  {
            for (unsigned i = 0; i < K; i++) {
                A[i].store(bottom_ptr<T>());
            }
        }

        StatePut put(T* val) {
            StateBasket state;
            int puts;
            while (true) {
                state = STATE.load();
                // std::cout<<state << std::endl;
                puts = PUTS.load();
                if (state == StateBasket::CLOSED || puts >= K) {
                    return StatePut::FULL;
                } else {
                    puts = PUTS.fetch_add(1);
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
                takes = TAKES.load();
                if (STATE.load() == StateBasket::CLOSED || takes >= K) {
                    return basket_closed_ptr<T>();
                } else {
                    takes = TAKES.fetch_add(1);
                    if (takes >= K) {
                        STATE.store(StateBasket::CLOSED);
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
        std::atomic<int> R;
    public:
        LLICCAS() {
            R.store(0, std::memory_order_relaxed);
        }

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

    template<typename T, typename LLIC, typename Basket, std::size_t capacity>
    class FAIQueue {
    private:
        Basket* A;
        LLIC head{};
        LLIC tail{};
    public:
        FAIQueue() {
            A = new Basket[capacity];
        }

        FAIQueue(std::size_t cores) {
            (void) cores;
            A = new Basket[capacity];
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
                    val = A[head].take();
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

    template<typename T, typename LLIC, typename Basket, int CAPACITY>
    class FAIQueueHP {
    private:
        std::size_t capacity;
        std::size_t size_k;
        std::size_t maxThreads;

        struct Node {
            alignas(64) std::atomic_intmax_t head;
            alignas(64) std::atomic_intmax_t tail;
            alignas(64) std::atomic<Node*> next;
            std::array<Basket, NODE_SIZE> ring;

            Node() {
                head.store(0, std::memory_order_relaxed);
                tail.store(0, std::memory_order_relaxed);
                next.store(nullptr, std::memory_order_relaxed);
            }
        };

        LLIC head;
        LLIC tail;
        std::atomic<int> nodes;
        std::atomic_intmax_t length;
        std::size_t m_max_threads;
        std::atomic<Node*> array[CAPACITY];

        MemoryManagementPool<Node> mm;
    public:

        FAIQueueHP(std::size_t max_threads = 64) : head{max_threads}, tail{max_threads},
                                                   nodes{1},length{CAPACITY},
                                                   m_max_threads{max_threads} {
            array[0].store(new Node());
        }

        ~FAIQueueHP() {
            while(dequeue(0) != nullptr);
        }

        void enqueue(T* elem, std::size_t thread_id) {
            while (true) {
                int n = (nodes.load() - 1) % CAPACITY;
                int tail = this->tail.LL();
                int t = tail % NODE_SIZE;
                // mark array[n] as hazardous
                if(array[n].load()->ring[t].put(elem) == StatePut::OK) {
                    this->tail.IC(tail, thread_id);
                    return;
                } else {
                    // create a new node
                    Node* newNode = new Node();
                    newNode->ring[0].put(elem);
                    // try insert a new node
                    Node* nullVal = nullptr;
                    if (array[n].load()->next.compare_exchange_strong(nullVal, newNode)) {
                        nodes.fetch_add(1);
                        this->tail.IC(tail, thread_id);
                        return;
                    }
                    this->tail.IC(tail, thread_id);
                }
            }
        }

        T* dequeue(std::size_t thread_id) {
            int head = this->head.LL();
            int tail = this->tail.LL();
            int h = head % NODE_SIZE;
            int n = (this->nodes.load() - 1);
            T* val = nullptr;
            while(true) {
                val = array[n].load()->ring[h].take();
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



    template<typename T>
    class QUeue {
    };
}

#endif
