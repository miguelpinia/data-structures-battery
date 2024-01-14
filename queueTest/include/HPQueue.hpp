#ifndef _HP_Queue_HPP_
#define _HP_Queue_HPP_

#include <atomic>
#include "MemoryManagementPool.hpp"

template <typename T>
struct Node {
    T data;
    std::atomic<Node*> next;
};

template <typename T>
class Queue {
private:
    Node<T> dummy;
    std::atomic<Node<T>*> Head{&dummy};
    std::atomic<Node<T>*> Tail{&dummy};
    MemoryManagementPool<Node<T>> mm;

public:
    Queue() {}

    void enqueue(T data, const int thread_id) {
        Node<T>* node = new Node<T>();
        node->data = data;
        node->next = nullptr;
        Node<T>* t = nullptr;
        Node<T>* next = nullptr;
        Node<T>* null = nullptr;
        while (true) {
            t = mm.protect(0, Tail, thread_id);
            if (Tail != t) continue;
            next = Tail.load()->next.load();
            if (Tail != t) continue;
            if (next != nullptr) {
                Tail.compare_exchange_strong(t, next);
                continue;
            }

            if (Tail.load()->next.compare_exchange_strong(null, node)) break;
        }
        Tail.compare_exchange_strong(t, node);
    }

    T dequeue(const int thread_id) {
        T data;
        Node<T>* h = nullptr;
        Node<T>* t = nullptr;
        Node<T>* next = nullptr;
        while (true) {
            h = mm.protect(0, Head, thread_id);
            if (Head != h) continue;
            t = Tail.load();
            next = mm.protect(1, h->next, thread_id);
            if (Head != h) continue;
            if (next == nullptr) return nullptr;
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
