#ifndef _LL_IC_QUEUE_HPP
#define _LL_IC_QUEUE_HPP

#include <atomic>
#include <limits>
#include <array>
#include <iostream>
#include "MemoryManagementPool.hpp"

namespace llic_queue {

    static constexpr int NODE_POW = 10;
    static constexpr int ARRAY_POW = 16;
    static constexpr int ENQUEUERS = 64;
    static constexpr std::size_t NODE_SIZE = 1ull << NODE_POW;
    static constexpr std::size_t ARRAY_SIZE = 1ull << ARRAY_POW;

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
        alignas(128) std::atomic<StateBasket> STATE{StateBasket::OPEN}; // 1 byte
        alignas(128) std::atomic<int> PUTS; // 8 bytes
        alignas(128) std::atomic<int> TAKES; // 8 bytes
        std::atomic<T*> items[K]; // 8 * K bytes
    public:
        KBasketFAI(): PUTS{0}, TAKES{0}  {
            for (unsigned i = 0; i < K; i++) {
                items[i].store(bottom_ptr<T>());
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
                    } else if (items[puts].exchange(val) == bottom_ptr<T>()) {
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
                        T* val = items[takes].exchange(top_ptr<T>());
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

    template <typename T, typename LLIC, typename Basket>
    class FAIQueueArray {
        // This queue must simulate an infinite array-based queue, but
        // using nodes, where each contains an array of baskets
        // We must remember that the baskets only store items whose
        // insertions were concurrent.
    private:
        struct Node {
            std::array<Basket, NODE_SIZE> ring;
        };
        LLIC Head; // Actual head index
        LLIC Tail;
        // We need an array of pointers to segments
        std::array<std::atomic<Node*>, ARRAY_SIZE> array;
        // std::atomic<Node*> array[NODE_SIZE]; // Without parameterize the class, we can store a maximum of 2^20 items, approx 1 million of items
        MemoryManagementPool<Node> mm;

    public:
        FAIQueueArray() {
            for (unsigned i = 0; i < ARRAY_SIZE; i++) {
                array[i].store(nullptr, std::memory_order_relaxed);
            }
        }

        FAIQueueArray(std::size_t cores) {
            (void) cores;
            for (unsigned i = 0; i < ARRAY_SIZE; i++) {
                array[i].store(nullptr, std::memory_order_relaxed);
            }
        };

        ~FAIQueueArray() {
            for (unsigned i = 0; i < ARRAY_SIZE; i++) {
                if (array[i] != nullptr) delete array[i];
            }
        }

        void enqueue(T* val, std::size_t thread_id) {
            int tail, node, pos;
            while (true) {
                tail = this->Tail.LL();
                node = tail / ARRAY_SIZE;
                pos = tail % NODE_SIZE;
                if (array[node] == nullptr) {
                    Node* newNode = new Node();
                    newNode->ring[0].put(val);
                    Node* nullNode = nullptr;
                    if (array[node].compare_exchange_strong(nullNode, newNode)) {
                        this->Tail.IC(tail, thread_id);
                        return;
                    }
                    delete newNode;
                    this->Tail.IC(tail, thread_id);
                    continue;
                } else if (array[node].load()->ring[pos].put(val) == StatePut::OK) {
                    this->Tail.IC(tail, thread_id);
                    return;
                }
                this->Tail.IC(tail, thread_id);
            }
        }

        T* dequeue(std::size_t thread_id) {
            int head = this->Head.LL();
            int tail = this->Tail.LL();
            int node, pos;
            T* val = nullptr;

            while (true) {
                if (head < tail) {
                    node = head / ARRAY_SIZE;
                    pos = head % NODE_SIZE;
                    val = array[node].load()->ring[pos].take();
                    if (val != basket_closed_ptr<T>()) {
                        return val;
                    }
                    this->Head.IC(head, thread_id);
                }
                auto hhead = this->Head.LL();
                auto ttail = this->Tail.LL();
                if (hhead == head && ttail == tail && head == tail) return nullptr;
                head = hhead;
                tail = ttail;
            }
            return val;
        }

    };


    template <typename T, typename LLIC, typename Basket>
    class FAIQueueArray2 {
        // This queue must simulate an infinite array-based queue, but
        // using nodes, where each contains an array of baskets
        // We must remember that the baskets only store items whose
        // insertions were concurrent.
    private:
        struct Node {
            std::array<Basket, NODE_SIZE> ring;
        };
        LLIC Head; // Actual head index
        LLIC Tail;
        // We need an array of pointers to segments
        std::array<Node, ARRAY_SIZE> array;
        // std::atomic<Node*> array[NODE_SIZE]; // Without parameterize the class, we can store a maximum of 2^20 items, approx 1 million of items
        MemoryManagementPool<Node> mm;

    public:
        FAIQueueArray2() {
        }

        FAIQueueArray2(std::size_t cores) {
            (void) cores;
        };

        ~FAIQueueArray2() {
        }

        void enqueue(T* val, std::size_t thread_id) {
            int tail, node, pos;
            while (true) {
                tail = this->Tail.LL();
                node = tail / ARRAY_SIZE;
                pos = tail % NODE_SIZE;
                if (array[node].ring[pos].put(val) == StatePut::OK) {
                    this->Tail.IC(tail, thread_id);
                    return;
                }
                this->Tail.IC(tail, thread_id);
            }
        }

        T* dequeue(std::size_t thread_id) {
            int head = this->Head.LL();
            int tail = this->Tail.LL();
            int node, pos;
            T* val = nullptr;

            while (true) {
                if (head < tail) {
                    node = head / ARRAY_SIZE;
                    pos = head % NODE_SIZE;
                    val = array[node].ring[pos].take();
                    if (val != basket_closed_ptr<T>()) {
                        return val;
                    }
                    this->Head.IC(head, thread_id);
                }
                auto hhead = this->Head.LL();
                auto ttail = this->Tail.LL();
                if (hhead == head && ttail == tail && head == tail) return nullptr;
                head = hhead;
                tail = ttail;
            }
            return val;
        }

    };


    template<typename T, typename LLIC, typename Basket>
    class Queue {

        struct Segment {
            Basket* items;
            LLIC HEAD;
            LLIC TAIL;
            std::atomic<Segment*> next;

            Segment() {
                items = new Basket[NODE_SIZE];
                next.store(nullptr, std::memory_order_relaxed);
            }

            ~Segment() {
                delete[] items;
            }

            bool isFull() {
                return TAIL.LL() >= (int)NODE_SIZE;
            }

            bool isClosed() {
                return HEAD.LL() >= (int)NODE_SIZE;
            }
        };

        alignas(64) std::atomic<Segment*> Head;
        alignas(64) std::atomic<Segment*> Tail;
        MemoryManagementPool<Segment> mm;

    public:
        Queue(std::size_t max_threads = 64) {
            (void) max_threads;
            Segment* sentinel = new Segment();
            Head.store(sentinel, std::memory_order_relaxed);
            Tail.store(sentinel, std::memory_order_relaxed);
        }

        ~Queue() {
            while (dequeue(0) != nullptr);
            delete Head.load();
        }

        void enqueue(T* val, std::size_t thread_id)  {
            while (true) {
                Segment* lastTail = mm.protectPointer(0, Tail.load(), thread_id);
                if (lastTail != Tail.load()) continue;
                Segment* lastNext = lastTail->next.load();
                if (lastNext != nullptr) {
                    Tail.compare_exchange_strong(lastTail, lastNext);
                    continue;
                }
                long basketTicket = lastTail->TAIL.LL();
                if (lastTail->isFull()) {
                    Segment* newSegment = new Segment();
                    Segment* nullSegment = nullptr;
                    newSegment->items[0].put(val);
                    newSegment->TAIL.IC(basketTicket, thread_id);
                    if (lastTail->next.compare_exchange_strong(nullSegment, newSegment)) {
                        Tail.compare_exchange_strong(lastTail, newSegment);
                        mm.clear(thread_id);
                        return;
                    }
                    delete newSegment;
                    continue;
                }
                if (lastTail->items[basketTicket].put(val) == StatePut::OK) {
                    lastTail->TAIL.IC(basketTicket, thread_id);
                    mm.clear(thread_id);
                    return;
                }
            }
        }

        T* dequeue(std::size_t thread_id) {
            while (true) {
                Segment* lastHead = mm.protectPointer(0, Head.load(), thread_id);
                if (lastHead == nullptr) return nullptr;
                if (lastHead != Head.load()) continue;
                if (lastHead->isClosed()) {
                    Segment* next = lastHead->next.load();
                    if (Head.compare_exchange_strong(lastHead, next)) {
                        mm.retire(lastHead, thread_id);
                        continue;
                    }
                    mm.clear(thread_id);
                    continue;
                }
                long headTicket = lastHead->HEAD.LL();
                long tailTicket = lastHead->TAIL.LL();

                while (!lastHead->isClosed()) {
                    if (headTicket < tailTicket) {
                        T* val = lastHead->items[headTicket].take();
                        if (val != basket_closed_ptr<T>()) return val;
                        lastHead->HEAD.IC(headTicket, thread_id);
                    }
                    long head = lastHead->HEAD.LL();
                    long tail = lastHead->TAIL.LL();
                    if ((headTicket == head) && (tail == tailTicket) && (headTicket == tailTicket)) return nullptr;
                    headTicket = head;
                    tailTicket = tail;
                }
            }
            return nullptr;
        }
    };
}

#endif
