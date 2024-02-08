#ifndef _SBQ_QUEUE_HPP_
#define _SBQ_QUEUE_HPP_

#include <atomic>
#include <stdexcept>
#include <cassert>
#include <iostream>
#include <limits>
#include <array>
#include "MemoryManagementPool.hpp"

namespace scal_basket_queue {
    static constexpr int MAX_THREADS = 64;
    static constexpr std::size_t ENQUEUERS = 64;

    template<typename T>
    constexpr T* empty_ptr() {
        return reinterpret_cast<T*>(std::numeric_limits<uint64_t>::max());
    }

    template<typename T>
    constexpr T* insert_ptr() {
        return reinterpret_cast<T*>(std::numeric_limits<uint64_t>::min());
    }

    template<typename T>
    class Queue {
    private:

        struct Basket {
            alignas(64) std::atomic<uint64_t> counter{0};
            alignas(64) std::atomic_flag empty = ATOMIC_FLAG_INIT;
            alignas(64) std::array<std::atomic<T*>, ENQUEUERS> cells{};

            Basket() {
                for(unsigned i = 0; i < ENQUEUERS; i++) {
                    cells[i].store(nullptr, std::memory_order_relaxed);
                }
            }

            bool insert(T* elem, std::size_t thread_id) {
                T* nullVal = nullptr;
                return cells[thread_id].compare_exchange_strong(nullVal, elem);
            }

            T* extract() {
                if (empty.test()) return nullptr;
                std::size_t idx;
                T* element = nullptr;
                while ((idx = counter.fetch_add(1)) < ENQUEUERS) {
                    if (idx == ENQUEUERS - 1) {
                        empty.test_and_set();
                    }
                    element = cells[idx].exchange(empty_ptr<T>());
                    if (element != nullptr) return element;
                }
                return nullptr;
            }

            bool isEmpty() {
                return empty.test();
            }


        };

        struct Node {
            std::atomic<Node*> next{nullptr};
            int index{0};
            Basket basket{};
        };

        alignas(128) std::atomic<Node*> head;
        alignas(128) std::atomic<Node*> tail;
        std::atomic<Node*> retired;
        Node* protectors[ENQUEUERS];
        // MemoryManagementPool<Node> mm;
        std::size_t maxThreads;

        enum class Status {BAD_TAIL, SUCCESS, FAILURE};

        Node* protect(std::atomic<Node*>& ptr, Node** p) {
            while (true) {
                *p = ptr.load();
                std::atomic_thread_fence(std::memory_order_seq_cst);
                if (ptr.load() == *p)  return *p;
            }
        }

        void unprotect(Node** p) {
            *p = nullptr;
        }

        void freeNodes() {
            Node* ret = retired.exchange(nullptr);
            if (ret == nullptr) return;
            int index = INT32_MAX;
            for (unsigned i = 0; i < ENQUEUERS; i++) {
                Node* p = protectors[i];
                if (p != nullptr) {
                    int idx = p->index;
                    if (idx < index) index = idx;
                }
            }
            while (ret != head.load() and ret->index < index) {
                Node* tmp = ret->next.load();
                delete ret;
                ret = tmp;
            }

            this->retired.store(ret);
        }

    public:
        Queue(std::size_t maxThreads = MAX_THREADS) : maxThreads(maxThreads) {
            Node* sentinel = new Node();
            head.store(sentinel, std::memory_order_relaxed);
            tail.store(sentinel, std::memory_order_relaxed);
            retired.store(sentinel, std::memory_order_relaxed);
            for (unsigned i = 0; i < ENQUEUERS; i++) {
                protectors[i] = nullptr;
            }
        }

        ~Queue() {
            while (dequeue(0) != nullptr);
            delete head.load();
            // delete tail.load();
        }

        void enqueue(T* elem, std::size_t thread_id) {
            assert(elem != nullptr && "Elemento a insertar no puede ser nullptr");
            Node* t = protect(tail, &protectors[thread_id]);
            // Node* t = mm.protect(0, tail, thread_id);
            Node* newNode = new Node();
            newNode->basket.insert(elem, thread_id);
            while (true) {
                t = tail.load();
                newNode->index = t->index + 1;
                Status status = tryAppend(t, newNode);
                if (status == Status::SUCCESS) {
                    tail.compare_exchange_strong(t, newNode);
                    return;
                } else if (status == Status::FAILURE) {
                    t = tail.load();
                    if (t->basket.insert(elem, thread_id)) {
                        delete newNode;
                        break;
                    }
                }
                while (t->next != nullptr) {
                    t = t->next;
                }
                advanceNode(tail, t);
            }
            unprotect(&protectors[thread_id]);
            // mm.clear(thread_id);
        }

        T* dequeue(std::size_t thread_id) {
            Node* h = protect(head, &protectors[thread_id]);
            // Node* h = mm.protect(0, head, thread_id);
            T* element = nullptr;
            while (true) {
                while (h->basket.isEmpty() && h->next.load() != nullptr) {
                    h = h->next;
                }
                element = h->basket.extract();
                if (element != nullptr || h->next == nullptr) {
                    break;
                }
            }
            advanceNode(head, h);
            freeNodes();
            unprotect(&protectors[thread_id]);
            return element;
        }

        Status tryAppend(Node* tail, Node* newNode) {
            Node* nullValue = nullptr;
            if (tail->next.load() != nullptr) return Status::BAD_TAIL;
            return tail->next.compare_exchange_strong(nullValue, newNode) ? Status::SUCCESS : Status::FAILURE;
        }

        void advanceNode(std::atomic<Node*>& ptr, Node* newNode) {
            while (true) {
                Node* oldNode = ptr.load();
                if (oldNode->index >= newNode->index) return;
                if (ptr.compare_exchange_strong(oldNode, newNode)) return;
            }
        }


    };

}
#endif
