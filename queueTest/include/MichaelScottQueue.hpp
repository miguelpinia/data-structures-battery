#ifndef _MICHAEL_SCOTT_QUEUE_HP_H_
#define _MICHAEL_SCOTT_QUEUE_HP_H_

#include <atomic>
#include <stdexcept>
#include <cassert>
#include "MemoryManagementPool.hpp"


template<typename T>
class MichaelScottQueue {

private:

    struct Node {
        T* item;
        std::atomic<Node*> next;

        Node(T* userItem) : item{userItem}, next{nullptr} { }

        bool casNext(Node *cmp, Node *val) {
            return next.compare_exchange_strong(cmp, val);
        }
    };

    alignas(128) std::atomic<Node*> head;
    alignas(128) std::atomic<Node*> tail;

    static const int MAX_THREADS = 128;
    const int maxThreads;

    MemoryManagementPool<Node> mm;

public:
    MichaelScottQueue(int maxThreads=MAX_THREADS) : maxThreads{maxThreads} {
        Node* sentinelNode = new Node(nullptr);
        head.store(sentinelNode, std::memory_order_relaxed);
        tail.store(sentinelNode, std::memory_order_relaxed);
    }


    ~MichaelScottQueue() {
        while (dequeue(0) != nullptr);
        delete head.load();
    }

    void enqueue(T* item, const int tid) {
        assert(item != nullptr && "Elemento a insertar no puede ser nullptr");
        Node* newNode = new Node(item);
        while (true) {
            Node* ltail = mm.protectPointer(0, tail.load(), tid);
            if (ltail == tail.load()) {
                Node* lnext = ltail->next.load();
                if (lnext == nullptr) {
                    if (ltail->next.compare_exchange_strong(lnext, newNode)) {
                        tail.compare_exchange_strong(ltail, newNode);
                        mm.clear(tid);
                        return;
                    }
                } else {
                    tail.compare_exchange_strong(ltail, lnext);
                }
            }
        }
    }


    T* dequeue(const int tid) {
        Node* node = mm.protectPointer(0, head.load(), tid);
        while (node != tail.load()) {
            Node* lnext = mm.protectPointer(1, node->next.load(), tid);
            if (head.compare_exchange_strong(node, lnext)) {
                T* item = lnext->item;
                mm.clear(tid);
                mm.retire(node, tid);
                return item;
            }
            node = mm.protectPointer(0, head.load(), tid);
        }
        mm.clear(tid);
        return nullptr;
    }
};

#endif
