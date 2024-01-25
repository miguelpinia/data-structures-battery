#ifndef _HP_Queue_HPP_
#define _HP_Queue_HPP_

#include <atomic>
#include "MemoryManagementPool.hpp"

template <typename T>
class Queue {
private:
    struct Node {
        T* data;
        std::atomic<Node*> next{nullptr};

        Node() {
        }

        Node(T* d) {
            data = d;
        }
   };


    T defaultValue;

    Node* dummy = new Node(&defaultValue);

    alignas(128) std::atomic<Node*> Head = dummy;
    alignas(128) std::atomic<Node*> Tail = dummy;
    MemoryManagementPool<Node> mm;

public:
    Queue() {}

    Queue(T defaultVal) : defaultValue(defaultVal) {}

    void enqueue(T* data, const int thread_id) {
        Node* node = new Node(data);
        Node* t = nullptr;
        Node* next = nullptr;
        Node* null = nullptr;
        while (true) {

            t = mm.protectPointer(0, Tail.load(), thread_id);
            if (Tail.load() != t) continue;
            next = Tail.load()->next.load();
            if (Tail.load() != t) continue;
            if (next != nullptr) {
                Tail.compare_exchange_strong(t, next);
                continue;
            }
            if (Tail.load()->next.compare_exchange_strong(null, node)) break;
        }
        Tail.compare_exchange_strong(t, node);
    }

    T* dequeue(const int thread_id) {
        T* data;
        Node* h = nullptr;
        Node* t = nullptr;
        Node* next = nullptr;
        while (true) {
            h = Head.load();
            h = mm.protectPointer(0, h, thread_id);
            if (Head.load() != h) continue;
            t = Tail.load();
            next = h->next.load();
            next = mm.protectPointer(1, next, thread_id);
            if (Head.load() != h) continue;
            if (next == nullptr) return &defaultValue;
            if (h == t) {
                Tail.compare_exchange_strong(t, next);
                continue;
            }
            data = next->data;
            if (Head.compare_exchange_strong(h, next)) break;
        }
        mm.retire(h, thread_id);
        return data;
    }
};

#endif
