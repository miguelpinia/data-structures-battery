#ifndef _LCRQ_HPP_
#define _LCRQ_HPP_

#include <cstdint>
#include <atomic>
#include <cassert>
#include <limits>
#include <array>
#include <iostream>
#include "MemoryManagementPool.hpp"

namespace lcrq_queue {

#define __CAS2(ptr, o1, o2, n1, n2)                             \
    ({                                                          \
        char __ret;                                             \
        __typeof__(o2) __junk;                                  \
        __typeof__(*(ptr)) __old1 = (o1);                       \
        __typeof__(o2) __old2 = (o2);                           \
        __typeof__(*(ptr)) __new1 = (n1);                       \
        __typeof__(o2) __new2 = (n2);                           \
        asm volatile("lock cmpxchg16b %2;setz %1"               \
                     : "=d"(__junk), "=a"(__ret), "+m"(*ptr)    \
                     : "b"(__new1), "c"(__new2),                \
                       "a"(__old1), "d"(__old2));               \
        __ret; })

#define CAS2(ptr, o1, o2, n1, n2)    __CAS2(ptr, o1, o2, n1, n2)

#define BIT_TEST_AND_SET(ptr, b)                                        \
    ({                                                                  \
        char __ret;                                                     \
        asm volatile("lock btsq $63, %0; setnc %1" : "+m"(*ptr), "=a"(__ret) : : "cc"); \
        __ret;                                                          \
    })

    constexpr int NODE_POW = 10;
    constexpr std::size_t NODE_SIZE = 1ull << NODE_POW;

    template<typename T>
    class Queue {
    private:

        struct alignas(128) Node { // Each addess is aligned to 128 bytes.
            std::atomic<T*> val; // Pointer of 64 bits (8 bytes)

            std::atomic<uint64_t> idx; // Integer of 64 bits (8 bytes)

            uint64_t pad[14]; // Guessing that cache line size is of
                              // 64 bits, we add junk with size of 14
                              // * 8 = 102 bytes. This not affect the
                              // CAS2, due we use only the 2 first
                              // words. Then, all the struct uses 128
                              // bytes, reason for the 128 alignment
        };

        struct CRQ {
            alignas(128) std::atomic_intmax_t head;
            alignas(128) std::atomic_intmax_t tail;
            alignas(128) std::atomic<CRQ*> next;
            std::array<Node, NODE_SIZE> ring;

            CRQ() {
                for (unsigned i = 0; i < NODE_SIZE; i++) {
                    ring[i].val.store(nullptr, std::memory_order_relaxed);
                    ring[i].idx.store(i, std::memory_order_relaxed);
                }
                head.store(0, std::memory_order_relaxed);
                tail.store(0, std::memory_order_relaxed);
                next.store(nullptr, std::memory_order_relaxed);
            };
        };

        alignas(64) std::atomic<CRQ*> head;
        alignas(64) std::atomic<CRQ*> tail;
        std::size_t m_max_threads;
        MemoryManagementPool<CRQ> mm;

        bool isEmpty(T* v) {
            return v == nullptr;
        }

        uint64_t getNodeIndex(uint64_t i) {
            return (i & ~(1ull << 63));
        }

        uint64_t setUnsafe(uint64_t i) {
            return (i | (1ull << 63));
        }

        uint64_t getNodeUnsafe(uint64_t i) {
            return (i & (1ull << 63));
        }

        inline uint64_t tailIndex(uint64_t t) {
            return (t & ~(1ull << 63));
        }

        bool crqIsClosed(uint64_t t) {
            return (t & (1ull << 63)) != 0;
        }

        void fixState(CRQ* lhead) {
            while(true) {
                uint64_t tail_idx = lhead->tail.fetch_add(0);
                uint64_t head_idx = lhead->head.fetch_add(0);

                if (lhead->tail.load() != (int64_t)tail_idx) continue;
                if (head_idx > tail_idx) {
                    int64_t tmp = tail_idx;
                    if (lhead->tail.compare_exchange_strong(tmp, head_idx)) break;
                    continue;
                }
                break;
            }
        }

        bool closeCRQ(CRQ* rq, const uint64_t tailTkt, const int tries) {
            if (tries < 10) {
                int64_t tmp = tailTkt + 1;
                return rq->tail.compare_exchange_strong(tmp, (tailTkt+1)|(1ull<<63));
            } else {
                return BIT_TEST_AND_SET(&rq->tail, 63);
            }
        }

    public:
        Queue(std::size_t max_threads = 64) : m_max_threads(max_threads) {
            CRQ* sentinel = new CRQ();
            head.store(sentinel, std::memory_order_relaxed);
            tail.store(sentinel, std::memory_order_relaxed);
        }

        ~Queue() {
            while (dequeue(0) != nullptr);
            delete head.load();
        };

        void enqueue(T* elem, std::size_t thread_id) {
            int try_close = 0;
            while (true) {
                CRQ* ltail = mm.protectPointer(0, tail.load(), thread_id);
                if (ltail != tail.load()) continue;
                CRQ* lnext = ltail->next.load();
                if (lnext != nullptr) {
                    tail.compare_exchange_strong(ltail, lnext);
                    continue;
                }
                uint64_t tailTkt = ltail->tail.fetch_add(1);
                if (crqIsClosed(tailTkt)) {
                    CRQ* newNode = new CRQ();
                    newNode->tail.store(1, std::memory_order_relaxed);
                    newNode->ring[0].val.store(elem, std::memory_order_relaxed);
                    newNode->ring[0].idx.store(0, std::memory_order_relaxed);
                    CRQ* nullNode = nullptr;
                    if (ltail->next.compare_exchange_strong(nullNode, newNode)) {
                        tail.compare_exchange_strong(ltail, newNode);
                        mm.clear(thread_id);
                        return;
                    }
                    delete newNode;
                    continue;
                }
                Node* cell = &ltail->ring[tailTkt & (NODE_SIZE - 1)];
                uint64_t idx = cell->idx.load();
                if (cell->val.load() == nullptr) {
                    if (getNodeIndex(idx) <= tailTkt) {
                        if ((!getNodeUnsafe(idx) || (ltail->head.load() < (int64_t)tailTkt))) {
                            if (CAS2((void**)cell, nullptr, idx, elem, tailTkt)) {
                                mm.clear(thread_id);
                                return;
                            }
                        }
                    }
                }
                if (((int64_t)(tailTkt - ltail->head.load()) >= (int64_t)NODE_SIZE)
                    && closeCRQ(ltail, tailTkt, ++try_close)) continue;
            }
        };

        T* dequeue(std::size_t thread_id) {
            while (true) {
                CRQ* lhead = mm.protectPointer(0, head.load(), thread_id);
                if (lhead != head.load()) continue;
                uint64_t headTkt = lhead->head.fetch_add(1);
                Node* node = &lhead->ring[headTkt & (NODE_SIZE - 1)];

                int r = 0;
                uint64_t tt = 0;

                while (true) {
                    uint64_t node_idx = node->idx.load();
                    uint64_t unsafe = getNodeUnsafe(node_idx);
                    uint64_t idx = getNodeIndex(node_idx);
                    T* val = node->val.load();

                    if (idx > headTkt) break;

                    if (val != nullptr) {
                        if (idx == headTkt){
                            if (CAS2((void**)node, val, node_idx, nullptr, unsafe | (headTkt+NODE_SIZE))) {
                                mm.clear(thread_id);
                                return val;
                            }
                        } else {
                            if (CAS2((void**)node, val, node_idx, val, setUnsafe(idx)))
                                break;
                        }
                    } else {
                        if ((r & (NODE_SIZE - 1)) == 0) tt = lhead->tail.load();
                        int crqClosed = crqIsClosed(tt);
                        uint64_t t = tailIndex(tt);
                        if (unsafe) {
                            if (CAS2((void**)node, val, node_idx, val, unsafe | (headTkt + NODE_SIZE)))
                                break;
                        } else if (t < headTkt + 1 || r > 200000 || crqClosed) {
                            if (CAS2((void**)node, val, idx, val, (headTkt + NODE_SIZE))) {
                                if (r > 200000 && tt > NODE_SIZE) BIT_TEST_AND_SET(&lhead->tail, 63);
                                break;
                            }
                        } else {
                            ++r;
                        }
                    }


                }
                if (tailIndex(lhead->tail.load()) <= headTkt + 1) {
                    fixState(lhead);
                    CRQ* lnext = lhead->next.load();
                    if (lnext == nullptr) {
                        mm.clear(thread_id);
                        return nullptr;
                    }
                    if (tailIndex(lhead->tail) <= headTkt + 1) {
                        if (head.compare_exchange_strong(lhead, lnext)) mm.retire(lhead, thread_id);

                    }
                }
            }
        }

        // Queue(const Queue&)                  = delete;
        // Queue(Queue&&)                       = delete;
        // const Queue& operator=(const Queue&) = delete;
        // const Queue& operator=(Queue&&)      = delete;
    };


};

#endif
