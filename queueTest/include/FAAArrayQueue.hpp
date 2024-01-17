#ifndef _FAA_ARRAY_QUEUE_HPP_
#define _FAA_ARRAY_QUEUE_HPP_

#include <atomic>
#include <stdexcept>
#include <cassert>
#include "MemoryManagementPool.hpp"

template <typename T>
class FAAArrayQueue {

    static const long BUFFER_SIZE = 1024;

private:
    struct Node {
        std::atomic<int>   deqidx;
        std::atomic<T*>    items[BUFFER_SIZE];
        std::atomic<int>   enqidx;
        std::atomic<Node*> next;


        Node(T* item): deqidx{0}, enqidx{1}, next{nullptr} {
            items[0].store(item, std::memory_order_relaxed);
            for (long i = 1; i < BUFFER_SIZE; i++) {
                items[i].store(nullptr, std::memory_order_relaxed);
            }
        }

        bool casNext(Node *cmp, Node *val) {
            return next.compare_exchange_strong(cmp, val);
        }
    };

    bool casTail(Node *cmp, Node *val) {
        return tail.compare_exchange_strong(cmp, val);
    }

    bool casHead(Node *cmp, Node *val) {
        return head.compare_exchange_strong(cmp, val);
    }

    alignas(128) std::atomic<Node*> head;
    alignas(128) std::atomic<Node*> tail;

    static const int MAX_THREADS = 128;
    const int maxThreads;

    T* taken = (T*) new int();

    MemoryManagementPool<Node> mm;
    const int kHPTail = 0;
    const int kHPHead = 0;

public:
    FAAArrayQueue(int maxThreads=MAX_THREADS): maxThreads(maxThreads) {
        Node* sentinel = new Node(nullptr);
        sentinel->enqidx.store(0, std::memory_order_relaxed);
        head.store(sentinel, std::memory_order_relaxed);
        tail.store(sentinel, std::memory_order_relaxed);
    }

    ~FAAArrayQueue() {
        while (dequeue(0) != nullptr);
        delete head.load();
        delete (int*) taken;
    }

    std::string className() { return "FAAArrayqueue"; }

    void enqueue(T* item, const int thread_id) {
        assert(item != nullptr && "Elemento a insertar no puede ser nullptr");
        while (true) {
            Node* ltail = mm.protectPointer(0, tail.load(), thread_id);
            const int idx = ltail->enqidx.fetch_add(1);
            if (idx > BUFFER_SIZE - 1) {
                if (ltail != tail.load()) continue;
                Node* lnext = ltail->next.load();
                if (lnext == nullptr) {
                    Node* newNode = new Node(item);
                    if (ltail->casNext(nullptr, newNode)) {
                        casTail(ltail, newNode);
                        mm.clear(thread_id);
                        return;
                    }
                    delete newNode;
                } else {
                    casTail(ltail, lnext);
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

    T* dequeue(const int thread_id) {
        while (true) {
            Node* lhead = mm.protectPointer(0, head.load(), thread_id);
            if (lhead->deqidx.load() >= lhead->enqidx.load() && lhead->next.load() == nullptr) break;
            const int idx = lhead->deqidx.fetch_add(1);
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
#endif
