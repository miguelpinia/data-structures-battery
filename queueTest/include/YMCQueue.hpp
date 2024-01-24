#ifndef _YMC_QUEUE_HPP_
#define _YMC_QUEUE_HPP_

#include <cstdint>
#include <atomic>
#include <array>
#include <limits>
#include <vector>
#include <deque>
#include <cassert>
#include <iostream>

namespace ymc_queue {
    static constexpr auto PATIENCE = std::size_t{10};
    static constexpr auto NO_HAZARD = std::numeric_limits<std::uintmax_t>::max();
    constexpr int NODE_POW = 10;
    constexpr std::size_t NODE_SIZE = 1ull << NODE_POW;
    constexpr auto MAX_VAL = std::numeric_limits<uint64_t>::max();

    template <typename T>
    constexpr T* top_ptr() {
        return reinterpret_cast<T*>(std::numeric_limits<std::uintmax_t>::max());
    }

    template<typename T>
    class Queue {
    private:

        // Enqueue request
        struct alignas(64) EnqReq {
            std::atomic<intmax_t> id;
            std::atomic<T*> val;
            // uint64_t pad[6];

            // EnqReq(intmax_t id, T* val) : id(id), val{val} {}
        };

        // Dequeue request
        struct alignas(64) DeqReq {
            std::atomic<intmax_t> id;
            std::atomic<intmax_t> idx;
            // uint64_t pad[6];

            // DeqReq(intmax_t id, intmax_t idx): id(id), idx(idx) {}
        };


        struct alignas(64) Cell {
            std::atomic<T*> val{nullptr};
            std::atomic<EnqReq*> enqReq{nullptr};
            std::atomic<DeqReq*> deqReq{nullptr};
            // uint64_t pad[5];
        };

        struct Segment {
            alignas(64) std::atomic<Segment*> next{nullptr};
            alignas(64) std::atomic<intmax_t> id{0};
            alignas(64) std::array<Cell, NODE_SIZE> cells{};
        };

        struct Handle {
            Handle(Segment* segment, std::size_t maxThreads) :
                tail{segment}, head{segment}, peerHandles{maxThreads} {
                for (long unsigned int i = 0; i < maxThreads; i++) {
                    this->peerHandles.push_back(nullptr);
                }
            }

            Handle* next{nullptr};
            std::atomic<uintmax_t> hzdNodeId{MAX_VAL};
            std::atomic<Segment*> tail;
            std::uintmax_t tailNodeId{0};
            std::atomic<Segment*> head;
            std::uintmax_t headNodeId{0};
            alignas(64) EnqReq enqReq{0, nullptr};
            alignas(64) DeqReq deqReq{0, -1};
            alignas(64) Handle* enqHelpHandle{nullptr};
            intmax_t Ei{0};
            Handle* deqHelpHandle{ nullptr };
            Segment* spareNode{ new Segment() } ;
            std::vector<Handle*> peerHandles;
        };

        struct FindCellResult {
            Cell& cell;
            Segment& current;
        };

        alignas(128) std::atomic<intmax_t> mEnqIdx{1};
        alignas(128) std::atomic<intmax_t> mDeqIdx{1};
        alignas(128) std::atomic<intmax_t> mHelpIdx{0};

        std::atomic<Segment*> mHead;
        std::deque<Handle*> mHandles;
        std::size_t mMaxThreads;

        Segment* check(const std::atomic<uintmax_t>& peerHzdNodeId, Segment* curr, Segment* old) {
            const auto hzdNodeId = peerHzdNodeId.load(std::memory_order_acquire);
            if (hzdNodeId < (const long unsigned int)curr->id) {
                auto tmp = old;
                while ((const long unsigned int)tmp->id < hzdNodeId) {
                    tmp = tmp->next;
                }
                curr = tmp;
            }
            return curr;
        }

        Segment* update(std::atomic<Segment*>& peerNode,
                        const std::atomic_uintmax_t& peerHzdNodeId,
                        Segment* curr, Segment* old) {
            auto node = peerNode.load(std::memory_order_acquire);
            if (node->id < curr->id) {
                if (!peerNode.compare_exchange_strong(node, curr,
                                                      std::memory_order_seq_cst,
                                                      std::memory_order_seq_cst)) {
                    if (node->id < curr->id) {
                        curr = node;
                    }
                }
                curr = check(peerHzdNodeId, curr, old);
            }
            return curr;
        }

        FindCellResult findCell(const std::atomic<Segment*>& ptr,
                                Handle& threadHandle,
                                std::intmax_t idx) {
            auto curr = ptr.load(std::memory_order_relaxed);
            for (std::size_t j = curr->id; j < (idx / NODE_SIZE); ++j) {
                auto next = curr->next.load(std::memory_order_relaxed);
                if (next == nullptr) {
                    auto tmp = threadHandle.spareNode;

                    if (tmp == nullptr) {
                        tmp = new Segment();
                        threadHandle.spareNode = tmp;
                    }
                    tmp->id = j + 1;
                    if (curr->next.compare_exchange_strong(next, tmp, std::memory_order_release, std::memory_order_acquire)) {
                        next = tmp;
                        threadHandle.spareNode = nullptr;
                    }
                }
                curr = next;
            }
            return {curr->cells[idx % NODE_SIZE], *curr};
        }

    public:
        Queue(std::size_t max_threads) : mHandles{}, mMaxThreads(max_threads)
        {
            assert(max_threads > 0 && "Max_threads must be at least 1");
            Segment* node = new Segment();
            this->mHead.store(node, std::memory_order_relaxed);
            for (std::size_t i = 0; i < max_threads; ++i) {
                this->mHandles.push_back(new Handle(node, max_threads));
                // this->mHandles.emplace_back(node, max_threads);
            }

            for (std::size_t i = 0; auto& handle : this->mHandles) {
                auto next = i == max_threads - 1
                    ? this->mHandles[0]

                    : this->mHandles[i + 1];
                handle->next = next;
                handle->enqHelpHandle = next;
                handle->deqHelpHandle = next;
                i++;
            }
        }

        ~Queue() {
            auto curr = this->mHead.load(std::memory_order_relaxed);
            while (curr != nullptr) {
                auto tmp = curr;
                curr = curr->next.load(std::memory_order_relaxed);
                delete tmp;
            }
            for (auto& handle : this->mHandles) {
                delete handle->spareNode;
                delete handle;
            }
        }

        void enqueue(T* elem, std::size_t thread_id) {
            auto th = this->mHandles[thread_id];
            th->hzdNodeId.store(th->tailNodeId, std::memory_order_relaxed);
            std::intmax_t id = 0;
            bool success = false;

            for (long unsigned int patience = 0; patience < PATIENCE; ++patience) {
                if ((success = this->enqFast(elem, *th, id))) {
                    break;
                }
            }

            if (!success) {
                this->enqSlow(elem, *th, id);
            }

            th->tailNodeId = th->tail.load(std::memory_order_relaxed)->id;
            th->hzdNodeId.store(NO_HAZARD, std::memory_order_release);
        }

        T* dequeue(std::size_t thread_id) {
            auto th = this->mHandles[thread_id];
            th->hzdNodeId.store(th->headNodeId, std::memory_order_relaxed);
            std::intmax_t id = 0;
            T* res = nullptr;

            for (long unsigned int patience = 0; patience < PATIENCE; ++patience) {
                if ((res = this->deqFast(*th, id)) != top_ptr<T>()) {
                    break;
                }
            }

            if (res == top_ptr<T>()) {
                res = this->deqSlow(*th, id);
            }
            if (res != nullptr) {
                this->helpDeq(*th, *th->deqHelpHandle);
                th->deqHelpHandle = th->deqHelpHandle->next;
            }

            th->headNodeId = th->head.load(std::memory_order_relaxed)->id;
            th->hzdNodeId.store(NO_HAZARD, std::memory_order_release);

            if (th->spareNode == nullptr) {
                this->cleanUp(*th);
                th->spareNode = new Segment();
            }

            return res;
        }


        void cleanUp(Handle& th) {
            auto oid = this->mHelpIdx.load(std::memory_order_acquire);
            auto newNode = th.head.load(std::memory_order_relaxed);

            if (oid == -1) {
                return;
            }

            if ((newNode->id - oid) < (long int) (this->mMaxThreads * 2)) {
                return;
            }

            if (!this->mHelpIdx.compare_exchange_strong(oid, -1, std::memory_order_acquire, std::memory_order_relaxed)) {
                return;
            }

            auto lDi = this->mDeqIdx.load(std::memory_order_relaxed);
            auto lEi = this->mEnqIdx.load(std::memory_order_relaxed);

            while (lEi < lDi && !this->mEnqIdx.compare_exchange_weak(lEi, lDi + 1, std::memory_order_relaxed, std::memory_order_relaxed));

            auto oldNode = this->mHead.load(std::memory_order_relaxed);
            auto ph = &th;
            auto i = 0;

            do {
                newNode = check(ph->hzdNodeId, newNode, oldNode);
                newNode = update(ph->tail, ph->hzdNodeId, newNode, oldNode);
                newNode = update(ph->head, ph->hzdNodeId, newNode, oldNode);

                th.peerHandles[i++] = ph;
                ph = ph->next;
            } while (newNode->id > oid && ph != &th);

            while (newNode->id > oid && --i >= 0) {
                newNode = check(th.peerHandles[i]->hzdNodeId, newNode, oldNode);
            }

            const std::atomic_intmax_t nid{newNode->id.load()};
            if (nid <= oid) {
                this->mHelpIdx.store(oid, std::memory_order_release);
            } else {
                this->mHead.store(newNode, std::memory_order_relaxed);
                this->mHelpIdx.store(nid, std::memory_order_release);

                while (oldNode != newNode) {
                    auto tmp = oldNode->next.load(std::memory_order_relaxed);
                    delete oldNode;
                    oldNode = tmp;
                }
            }
        }

        bool enqFast(T* elem, Handle& threadHandle, std::intmax_t& id) {
            const auto i = this->mEnqIdx.fetch_add(1, std::memory_order_seq_cst);
            auto [cell, curr] = findCell(threadHandle.tail, threadHandle, i);
            threadHandle.tail.store(&curr, std::memory_order_relaxed);

            T* expected = nullptr;
            if (cell.val.compare_exchange_strong(expected, elem,
                                                 std::memory_order_relaxed,
                                                 std::memory_order_relaxed)) {
                return true;
            } else {
                id = i;
                return false;
            }
        }

        void enqSlow(T* elem, Handle& threadHandle, std::intmax_t id) {
            auto& enq = threadHandle.enqReq;
            enq.val.store(elem, std::memory_order_relaxed);
            enq.id.store(id, std::memory_order_release);

            std::intmax_t i;
            do {
                i = this->mEnqIdx.fetch_add(1, std::memory_order_relaxed);
                auto [cell, _ignore] = findCell(threadHandle.tail, threadHandle, i);
                std::cout << &enq << "\n" ;
                EnqReq* expected = nullptr;
                if (
                    cell.enqReq.compare_exchange_strong(expected, &enq, std::memory_order_seq_cst, std::memory_order_seq_cst)
                    && cell.val.load(std::memory_order_relaxed) != top_ptr<T>()
                    ) {
                    if (enq.id.compare_exchange_strong(id, -i, std::memory_order_relaxed, std::memory_order_relaxed)) {
                        id = -1;
                    }
                    break;
                }
            } while (enq.id.load(std::memory_order_relaxed) > 0);

            id = -enq.id.load(std::memory_order_relaxed);
            auto [cell, curr] = findCell(threadHandle.tail, threadHandle, id);
            threadHandle.tail.store(&curr, std::memory_order_relaxed);

            if (id > i) {
                auto lEi = this->mEnqIdx.load(std::memory_order_relaxed);
                while (
                       lEi <= id
                       && !this->mEnqIdx.compare_exchange_strong(lEi, id + 1, std::memory_order_relaxed, std::memory_order_relaxed)
                       );

            }

            cell.val.store(elem, std::memory_order_relaxed);
        }

        T* helpEnq(Cell& cell, Handle& threadHandle, std::intmax_t nodeId) {
            auto res = cell.val.load(std::memory_order_acquire);
            if (res != top_ptr<T>() && res != nullptr) {
                return res;
            }

            if (
                res == nullptr
                && !cell.val.compare_exchange_strong(res, top_ptr<T>(), std::memory_order_seq_cst, std::memory_order_seq_cst)) {
                if (res != top_ptr<T>()) {
                    return res;
                }
            }

            auto enq = cell.enqReq.load(std::memory_order_relaxed);
            if (enq == nullptr) {
                auto ph = threadHandle.enqHelpHandle;
                auto pe = &ph->enqReq;
                auto id = pe->id.load(std::memory_order_relaxed);

                if (threadHandle.Ei != 0 && threadHandle.Ei != id) {
                    threadHandle.Ei = 0;
                    threadHandle.enqHelpHandle = ph->next;
                    pe = &ph->enqReq;
                    id = pe->id;
                }

                if (id > 0 && id <= nodeId
                    && !cell.enqReq.compare_exchange_strong(enq, pe, std::memory_order_relaxed, std::memory_order_relaxed)
                    && enq != pe
                    ) {
                    threadHandle.Ei = id;
                } else {
                    threadHandle.Ei = 0;
                    threadHandle.enqHelpHandle = ph->next;
                }


                if (enq == nullptr
                    && cell.enqReq.compare_exchange_strong(enq, top_ptr<EnqReq>(), std::memory_order_relaxed, std::memory_order_relaxed)) {
                    enq = top_ptr<EnqReq>();

                }

            }

            if (enq == top_ptr<EnqReq>()) {
                return (this->mEnqIdx.load(std::memory_order_relaxed) <= nodeId ? nullptr : top_ptr<T>());
            }

            auto enqId = enq->id.load(std::memory_order_acquire);
            const auto enqVal = enq->val.load(std::memory_order_acquire);

            if (enqId > nodeId)  {
                if (cell.val.load(std::memory_order_relaxed) == top_ptr<T>()
                    && this->mEnqIdx.load(std::memory_order_relaxed) <= nodeId) {
                    return nullptr;
                }
            } else {
                if ((enqId > 0 && enq->id.compare_exchange_strong(enqId, -nodeId + 1, std::memory_order_relaxed, std::memory_order_relaxed))
                    || (enqId == -nodeId && cell.val.load(std::memory_order_relaxed) == top_ptr<T>())) {
                    auto lEi = this->mEnqIdx.load(std::memory_order_relaxed);
                    while (lEi <= nodeId && !this->mEnqIdx.compare_exchange_strong(lEi, nodeId + 1, std::memory_order_relaxed, std::memory_order_relaxed));
                    cell.val.store(enqVal, std::memory_order_relaxed);
                }
            }

            return cell.val.load(std::memory_order_relaxed);
        }

        T* deqFast(Handle& th, std::intmax_t& id) {
            const auto i = this->mDeqIdx.fetch_add(1, std::memory_order_seq_cst);
            auto [cell, curr] = findCell(th.head, th, i);
            th.head.store(&curr, std::memory_order_relaxed);
            T* res = this->helpEnq(cell, th, i);
            DeqReq* cd = nullptr;

            if (res == nullptr) {
                return nullptr;
            }

            if (res != top_ptr<T>() && cell.deqReq.compare_exchange_strong(cd, top_ptr<DeqReq>(), std::memory_order_relaxed, std::memory_order_relaxed)) {
                return res;
            }

            id = i;

            return top_ptr<T>();
        }

        T* deqSlow(Handle& th, std::intmax_t id) {
            auto& deq = th.deqReq;
            deq.id.store(id, std::memory_order_release);
            deq.idx.store(id, std::memory_order_release);

            this->helpDeq(th, th);

            const auto i = -1 * deq.idx.load(std::memory_order_relaxed);
            auto [cell, curr] = findCell(th.head, th, i);
            th.head.store(&curr, std::memory_order_relaxed);
            auto res = cell.val.load(std::memory_order_relaxed);

            return res == top_ptr<T>() ? nullptr : res;
        }

        void helpDeq(Handle& th, Handle& ph) {
            auto& deq = ph.deqReq;
            auto idx = deq.idx.load(std::memory_order_acquire);
            const auto id = deq.id.load(std::memory_order_relaxed);

            if (idx < id) {
                return;
            }

            const auto hzdNodeId = ph.hzdNodeId.load(std::memory_order_relaxed);
            th.hzdNodeId.store(hzdNodeId, std::memory_order_seq_cst);
            idx = deq.idx.load(std::memory_order_relaxed);

            auto i = id + 1;
            auto oldVal = id;
            auto newVal = 0;

            while(true) {
                for(; idx == oldVal && newVal == 0; ++i) {
                    auto [cell, _ignore] = findCell(ph.head, th, i);

                    auto lDi = this->mDeqIdx.load(std::memory_order_relaxed);
                    while (lDi <= i && !this->mDeqIdx.compare_exchange_strong(lDi, i + 1, std::memory_order_relaxed, std::memory_order_relaxed));

                    auto res = this->helpEnq(cell, th, i);
                    if  (res == nullptr || (res != top_ptr<T>() && cell.deqReq.load(std::memory_order_relaxed) == nullptr)) {
                        newVal = i;
                    } else {
                        idx = deq.idx.load(std::memory_order_acquire);
                    }
                }
                if (newVal != 0) {
                    if (deq.idx.compare_exchange_strong(idx, newVal, std::memory_order_release, std::memory_order_acquire)) {
                        idx = newVal;
                    }

                    if (idx >= newVal) {
                        newVal = 0;
                    }
                }

                if (idx < 0 || deq.id.load(std::memory_order_relaxed) != id) {
                    break;
                }

                auto [cell, _ignore] = findCell(ph.head, th, idx);
                DeqReq* cd = nullptr;

                if (cell.val.load(std::memory_order_relaxed) == top_ptr<T>()
                    || cell.deqReq.compare_exchange_strong(cd, &deq, std::memory_order_relaxed, std::memory_order_relaxed)
                    || cd == &deq) {
                    deq.idx.compare_exchange_strong(idx, -idx, std::memory_order_relaxed, std::memory_order_relaxed);
                    break;
                }

                oldVal = idx;
                if (idx >= i) {
                    i = idx + 1;
                }
            }
        }

    };


}
#endif /* _YMC_QUEUE_HPP_ */
