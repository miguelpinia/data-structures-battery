#ifndef _FAA_ARRAY_QUEUE_HPP_
#define _FAA_ARRAY_QUEUE_HPP_

#include <atomic>
#include <stdexcept>
#include <cassert>
#include "MemoryManagementPool.hpp"

namespace faa_array {
    static constexpr int NODE_POW = 10;
    static constexpr std::size_t BUFFER_SIZE = 1ull<<NODE_POW;
    static constexpr int MAX_THREADS = 64;

    template <typename T>
    class Queue {

    private:
        struct Node {
            std::atomic<int>   deqIdx;
            std::atomic<int>   enqIdx;
            std::atomic<Node*> next;
            std::atomic<T*>    items[BUFFER_SIZE];


            Node(T* item): deqIdx{0}, enqIdx{1}, next{nullptr} {
                items[0].store(item, std::memory_order_relaxed);
                for (std::size_t i = 1; i < BUFFER_SIZE; i++) {
                    items[i].store(nullptr, std::memory_order_relaxed);
                }
            }
        };

        alignas(128) std::atomic<Node*> head;
        alignas(128) std::atomic<Node*> tail;

        std::size_t maxThreads;
        T* taken = (T*) new int();
        MemoryManagementPool<Node> mm;


    public:
        Queue(std::size_t maxThreads=MAX_THREADS): maxThreads(maxThreads) {
            Node* sentinel = new Node(nullptr);
            sentinel->enqIdx.store(0, std::memory_order_relaxed);
            head.store(sentinel, std::memory_order_relaxed);
            tail.store(sentinel, std::memory_order_relaxed);
        }

        ~Queue() {
            while (dequeue(0) != nullptr);
            delete head.load();
            delete (int*) taken;
        }

        void enqueue(T* item, std::size_t thread_id) {
            assert(item != nullptr && "Elemento a insertar no puede ser nullptr");
            Node* nullValue = nullptr;
            while (true) {
                Node* ltail = mm.protect(0, tail, thread_id);
                std::size_t idx = ltail->enqIdx.fetch_add(1);
                if (idx > BUFFER_SIZE - 1) {
                    if (ltail != tail.load()) continue;
                    Node* lnext = ltail->next.load();
                    if (lnext == nullptr) {
                        Node* newNode = new Node(item);
                        if (ltail->next.compare_exchange_strong(nullValue, newNode)) {
                            tail.compare_exchange_strong(ltail, newNode);
                            mm.clear(thread_id);
                            return;
                        }
                        delete newNode;
                    } else {
                        tail.compare_exchange_strong(ltail, lnext);
                    }
                    continue;
                }
                T* itemnull = nullptr;
                if (ltail->items[idx].compare_exchange_strong(itemnull, item)) {
                    mm.clear(thread_id);
                    return;
                }
            }
        }

        T* dequeue(std::size_t thread_id) {
            while (true) {
                Node* lhead = mm.protect(0, head, thread_id);
                if (lhead->deqIdx.load() >= lhead->enqIdx.load() && lhead->next.load() == nullptr) break;
                std::size_t idx = lhead->deqIdx.fetch_add(1);
                if (idx > (BUFFER_SIZE - 1)) {
                    Node* lnext = lhead->next.load();
                    if (lnext == nullptr) break;
                    if (head.compare_exchange_strong(lhead, lnext)) {
                        mm.retire(lhead, thread_id);
                    }
                    continue;
                }
                T* item = lhead->items[idx].exchange(taken);
                if (item == nullptr) continue;
                mm.clear(thread_id);
                return item;
            }
            mm.clear(thread_id);
            return nullptr;
        }

    };

}

#endif
