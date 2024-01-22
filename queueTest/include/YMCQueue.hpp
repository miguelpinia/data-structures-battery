#ifndef _YMC_QUEUE_HPP_
#define _YMC_QUEUE_HPP_

#include <cstdint>
#include <atomic>
#include <array>
#include <limits>
#include <vector>
#include <deque>
#include <cassert>

namespace ymc_queue {
    constexpr std::size_t NODE_SIZE = 1024;
    constexpr auto MAX_U64 = std::numeric_limits<uint64_t>::max();

    struct alignas(64) EnqReq {
        std::atomic_intmax_t id;
        std::atomic<void*> val;
    };

    struct alignas(64) DeqReq {
        std::atomic_intmax_t id;
        std::atomic_intmax_t idx;
    };

    struct alignas(64) Cell {
        std::atomic<void*> val{nullptr};
        std::atomic<EnqReq*> enq_req{nullptr};
        std::atomic<DeqReq*> deq_req{nullptr};
    };

    struct Segment {
        alignas(64) std::atomic<Segment*> next{nullptr};
        alignas(64) std::atomic_intmax_t id{0};
        alignas(64) std::array<Cell, NODE_SIZE> cells{};
    };

    struct Handle {
        Handle(Segment* segment, std::size_t max_threads) :
            tail{segment}, head{segment}, peer_handles{max_threads} {
            for (long unsigned int i = 0; i < max_threads; i++) {
                this->peer_handles.push_back(nullptr);
            }
        }

        Handle* next{nullptr};
        std::atomic_uintmax_t hzd_node_id{MAX_U64};
        std::atomic<Segment*> tail;
        std::uintmax_t tail_node_id{0};
        std::atomic<Segment*> head;
        std::uintmax_t head_node_id{0};
        alignas(64) EnqReq enq_req;
        alignas(64) DeqReq deq_req;
        alignas(64) Handle* enq_help_handle{nullptr};
        intmax_t Ei{0};
        Handle* deq_help_handle{ nullptr };
        Segment* spare_node{ new Segment() } ;
        std::vector<Handle*> peer_handles;
    };

    struct FindCellResult {
        Cell& cell;
        Segment& current;
    };

    template <typename T>
    constexpr T* top_ptr() {
        return reinterpret_cast<T*>(std::numeric_limits<std::uintmax_t>::max());
    }

    Segment* check(const std::atomic_uintmax_t& peer_hzd_node_id, Segment* curr, Segment* old) {
        const auto hzd_node_id = peer_hzd_node_id.load(std::memory_order_acquire);
        if (hzd_node_id < curr->id) {
            auto tmp = old;
            while (tmp->id < hzd_node_id) {
                tmp = tmp->next;
            }
            curr = tmp;
        }
        return curr;
    }

    Segment* update(std::atomic<Segment*>& peer_node, const std::atomic_uintmax_t& peer_hzd_node_id, Segment* curr, Segment* old) {
        auto node = peer_node.load(std::memory_order_acquire);
        if (node->id < curr->id) {
            if (!peer_node.compare_exchange_strong(node, curr, std::memory_order_seq_cst, std::memory_order_seq_cst)) {
                if (node->id < curr->id) {
                    curr = node;
                }
            }
            curr = check(peer_hzd_node_id, curr, old);
        }
        return curr;
    }

    FindCellResult findCell(const std::atomic<Segment*>& ptr, Handle& thread_handle, std::intmax_t idx) {
        auto curr = ptr.load(std::memory_order_relaxed);
        for (long unsigned int j = curr->id; j < idx / NODE_SIZE; ++j) {
            auto next = curr->next.load(std::memory_order_relaxed);
            if (next == nullptr) {
                auto tmp = thread_handle.spare_node;

                if (tmp == nullptr) {
                    tmp = new Segment();
                    thread_handle.spare_node = tmp;
                }
                tmp->id = j + 1;
                if (curr->next.compare_exchange_strong(next, tmp, std::memory_order_release, std::memory_order_acquire)) {
                    next = tmp;
                    thread_handle.spare_node = nullptr;
                }
            }
            curr = next;
        }
        return {curr->cells[idx & NODE_SIZE], *curr};
    }


    class Queue {
    private:
        static constexpr auto PATIENCE = std::size_t{10};
        static constexpr auto NO_HAZARD = std::numeric_limits<std::uintmax_t>::max();

        void cleanUp(Handle& th);

        bool enq_fast(void* elem, Handle& thread_handle, std::intmax_t& id);
        void enq_slow(void* elem, Handle& thread_handle, std::intmax_t id);
        void* help_enq(Cell& c, Handle& theead_handle, std::intmax_t node_id);

        void* deq_fast(Handle& th, std::intmax_t& id);
        void* deq_slow(Handle& th, std::intmax_t id);
        void help_deq(Handle& th, Handle& ph);

        alignas(128) std::atomic_intmax_t m_enq_idx{1};
        alignas(128) std::atomic_intmax_t m_deq_idx{1};
        alignas(128) std::atomic_intmax_t m_help_idx{0};

        std::atomic<Segment*> m_head;
        std::deque<Handle*> m_handles;
        std::size_t m_max_threads;

    public:
        explicit Queue(std::size_t max_threads = 128);
        ~Queue() noexcept;
        void enqueue(void* elem, std::size_t thread_id);
        void* dequeue(std::size_t thread_id);

        Queue(const Queue&)                  = delete;
        Queue(Queue&&)                       = delete;
        const Queue& operator=(const Queue&) = delete;
        const Queue& operator=(Queue&&)      = delete;
    };

    Queue::Queue(std::size_t max_threads) : m_handles{}, m_max_threads(max_threads)
    {
        assert(max_threads > 0 && "Max_threads must be at least 1");
        Segment* node = new Segment();
        this->m_head.store(node, std::memory_order_relaxed);
        for (long unsigned int i = 0; i < max_threads; ++i) {
            this->m_handles.push_back(new Handle(node, max_threads));
            // this->m_handles.emplace_back(node, max_threads);
        }

        for (long unsigned int i = 0; auto& handle : this->m_handles) {
            auto next = i == max_threads - 1
                ? this->m_handles[0]
                : this->m_handles[i + 1];
            handle->next = next;
            handle->enq_help_handle = next;
            handle->deq_help_handle = next;
            i+=1;
        }
    }

    Queue::~Queue() {
        auto curr = this->m_head.load(std::memory_order_relaxed);
        while (curr != nullptr) {
            auto tmp = curr;
            curr = curr->next.load(std::memory_order_relaxed);
            delete tmp;
        }
        for (auto& handle : this->m_handles) {
            delete handle->spare_node;
        }
    }


    void Queue::enqueue(void* elem, std::size_t thread_id) {
        auto th = this->m_handles[thread_id];
        th->hzd_node_id.store(th->tail_node_id, std::memory_order_relaxed);
        std::intmax_t id = 0;
        bool success = false;

        for (long unsigned int patience = 0; patience < PATIENCE; ++patience) {
            if ((success = this->enq_fast(elem, *th, id))) {
                break;
            }
        }

        if (!success) {
            this->enq_slow(elem, *th, id);
        }

        th->tail_node_id = th->tail.load(std::memory_order_relaxed)->id;
        th->hzd_node_id.store(NO_HAZARD, std::memory_order_release);
    }

    void* Queue::dequeue(std::size_t thread_id) {
        auto th = this->m_handles[thread_id];
        th->hzd_node_id.store(th->head_node_id, std::memory_order_relaxed);
        std::intmax_t id = 0;
        void* res = nullptr;

        for (long unsigned int patience = 0; patience < PATIENCE; ++patience) {
            if ((res = this->deq_fast(*th, id)) != top_ptr<void>()) {
                break;
            }
        }

        if (res == top_ptr<void>()) {
            res = this->deq_slow(*th, id);
        }
        if (res != nullptr) {
            this->help_deq(*th, *th->deq_help_handle);
            th->deq_help_handle = th->deq_help_handle->next;
        }

        th->head_node_id = th->head.load(std::memory_order_relaxed)->id;
        th->hzd_node_id.store(NO_HAZARD, std::memory_order_release);

        if (th->spare_node == nullptr) {
            this->cleanUp(*th);
            th->spare_node = new Segment();
        }

        return res;
    }

    void Queue::cleanUp(Handle& th) {
        auto oid = this->m_help_idx.load(std::memory_order_acquire);
        auto new_node = th.head.load(std::memory_order_relaxed);

        if (oid == -1) {
            return;
        }

        if ((new_node->id - oid) < (this->m_max_threads * 2)) {
            return;
        }

        if (!this->m_help_idx.compare_exchange_strong(oid, -1, std::memory_order_acquire, std::memory_order_relaxed)) {
            return;
        }

        auto lDi = this->m_deq_idx.load(std::memory_order_relaxed);
        auto lEi = this->m_enq_idx.load(std::memory_order_relaxed);

        while (lEi < lDi && !this->m_enq_idx.compare_exchange_weak(lEi, lDi + 1, std::memory_order_relaxed, std::memory_order_relaxed));

        auto old_node = this->m_head.load(std::memory_order_relaxed);
        auto ph = &th;
        auto i = 0;

        do {
            new_node = check(ph->hzd_node_id, new_node, old_node);
            new_node = update(ph->tail, ph->hzd_node_id, new_node, old_node);
            new_node = update(ph->head, ph->hzd_node_id, new_node, old_node);

            th.peer_handles[i++] = ph;
            ph = ph->next;
        } while (new_node->id > oid && ph != &th);

        while (new_node->id > oid && --i >= 0) {
            new_node = check(th.peer_handles[i]->hzd_node_id, new_node, old_node);
        }

        const std::atomic_intmax_t nid{new_node->id.load()};
        if (nid <= oid) {
            this->m_help_idx.store(oid, std::memory_order_release);
        } else {
            this->m_head.store(new_node, std::memory_order_relaxed);
            this->m_help_idx.store(nid, std::memory_order_release);

            while (old_node != new_node) {
                auto tmp = old_node->next.load(std::memory_order_relaxed);
                delete old_node;
                old_node = tmp;
            }
        }
    }

    bool Queue::enq_fast(void* elem, Handle& thread_handle, std::intmax_t& id) {
        const auto i = this->m_enq_idx.fetch_add(1, std::memory_order_seq_cst);
        auto [cell, curr] = findCell(thread_handle.tail, thread_handle, i);
        thread_handle.tail.store(&curr, std::memory_order_relaxed);

        void* expected = nullptr;
        if (cell.val.compare_exchange_strong(expected, elem, std::memory_order_relaxed, std::memory_order_relaxed)) {
            return true;
        } else {
            id = i;
            return false;
        }
    }

    void Queue::enq_slow(void* elem, Handle& thread_handle, std::intmax_t id) {
        auto& enq = thread_handle.enq_req;
        enq.val.store(elem, std::memory_order_relaxed);
        enq.id.store(id, std::memory_order_release);

        std::intmax_t i;
        do {
            i = this->m_enq_idx.fetch_add(1, std::memory_order_relaxed);
            auto [cell, _ignore] = findCell(thread_handle.tail, thread_handle, i);
            EnqReq* expected = nullptr;
            if (
                cell.enq_req.compare_exchange_strong(expected, &enq, std::memory_order_seq_cst, std::memory_order_seq_cst)
                && cell.val.load(std::memory_order_relaxed) != top_ptr<EnqReq>()
                ) {
                if (enq.id.compare_exchange_strong(id, -i, std::memory_order_relaxed, std::memory_order_relaxed)) {
                    id = -1;
                }
                break;
            }
        } while (enq.id.load(std::memory_order_relaxed) > 0);

        id = -enq.id.load(std::memory_order_relaxed);
        auto [cell, curr] = findCell(thread_handle.tail, thread_handle, id);
        thread_handle.tail.store(&curr, std::memory_order_relaxed);

        if (id > i) {
            auto lEi = this->m_enq_idx.load(std::memory_order_relaxed);
            while (
                   lEi <= id
                   && !this->m_enq_idx.compare_exchange_strong(lEi, id + 1, std::memory_order_relaxed, std::memory_order_relaxed)
                   );

        }

        cell.val.store(elem, std::memory_order_relaxed);
    }

    void* Queue::help_enq(Cell& cell, Handle& thread_handle, std::intmax_t node_id) {
        auto res = cell.val.load(std::memory_order_acquire);
        if (res != top_ptr<void>() && res != nullptr) {
            return res;
        }

        if (
            res == nullptr
            && !cell.val.compare_exchange_strong(res, top_ptr<void>(), std::memory_order_seq_cst, std::memory_order_seq_cst)) {
            if (res != top_ptr<void>()) {
                return res;
            }
        }

        auto enq = cell.enq_req.load(std::memory_order_relaxed);
        if (enq == nullptr) {
            auto ph = thread_handle.enq_help_handle;
            auto pe = &ph->enq_req;
            auto id = pe->id.load(std::memory_order_relaxed);

            if (thread_handle.Ei != 0 && thread_handle.Ei != id) {
                thread_handle.Ei = 0;
                thread_handle.enq_help_handle = ph->next;
                pe = &ph->enq_req;
                id = pe->id;
            }

            if (id > 0 && id <= node_id
                && !cell.enq_req.compare_exchange_strong(enq, pe, std::memory_order_relaxed, std::memory_order_relaxed)
                && enq != pe
                ) {
                thread_handle.Ei = id;
            } else {
                thread_handle.Ei = 0;
                thread_handle.enq_help_handle = ph->next;
            }


            if (enq == nullptr
                && cell.enq_req.compare_exchange_strong(enq, top_ptr<EnqReq>(), std::memory_order_relaxed, std::memory_order_relaxed)) {
                enq = top_ptr<EnqReq>();

            }

        }

        if (enq == top_ptr<EnqReq>()) {
            return (this->m_enq_idx.load(std::memory_order_relaxed) <= node_id ? nullptr : top_ptr<void>());
        }

        auto enq_id = enq->id.load(std::memory_order_acquire);
        const auto enq_val = enq->val.load(std::memory_order_acquire);

        if (enq_id > node_id)  {
            if (cell.val.load(std::memory_order_relaxed) == top_ptr<void>()
                && this->m_enq_idx.load(std::memory_order_relaxed) <= node_id) {
                return nullptr;
            }
        } else {
            if ((enq_id > 0 && enq->id.compare_exchange_strong(enq_id, -node_id + 1, std::memory_order_relaxed, std::memory_order_relaxed))
                || (enq_id == -node_id && cell.val.load(std::memory_order_relaxed) == top_ptr<void>())) {
                auto lEi = this->m_enq_idx.load(std::memory_order_relaxed);
                while (lEi <= node_id && !this->m_enq_idx.compare_exchange_strong(lEi, node_id + 1, std::memory_order_relaxed, std::memory_order_relaxed));
                cell.val.store(enq_val, std::memory_order_relaxed);
            }
        }

        return cell.val.load(std::memory_order_relaxed);
    }

    void* Queue::deq_fast(Handle& th, std::intmax_t& id) {
        const auto i = this->m_deq_idx.fetch_add(1, std::memory_order_seq_cst);
        auto [cell, curr] = findCell(th.head, th, i);
        th.head.store(&curr, std::memory_order_relaxed);
        void* res = this->help_enq(cell, th, i);
        DeqReq* cd = nullptr;

        if (res == nullptr) {
            return nullptr;
        }

        if (res != top_ptr<void>() && cell.deq_req.compare_exchange_strong(cd, top_ptr<DeqReq>(), std::memory_order_relaxed, std::memory_order_relaxed)) {
            return res;
        }

        id = i;

        return top_ptr<void>();
    }

    void* Queue::deq_slow(Handle& th, std::intmax_t id) {
        auto& deq = th.deq_req;
        deq.id.store(id, std::memory_order_release);
        deq.idx.store(id, std::memory_order_release);

        this->help_deq(th, th);

        const auto i = -1 * deq.idx.load(std::memory_order_relaxed);
        auto [cell, curr] = findCell(th.head, th, i);
        th.head.store(&curr, std::memory_order_relaxed);
        auto res = cell.val.load(std::memory_order_relaxed);

        return res == top_ptr<void>() ? nullptr : res;
    }

    void Queue::help_deq(Handle& th, Handle& ph) {
        auto& deq = ph.deq_req;
        auto idx = deq.idx.load(std::memory_order_acquire);
        const auto id = deq.id.load(std::memory_order_relaxed);

        if (idx < id) {
            return;
        }

        const auto hzd_node_id = ph.hzd_node_id.load(std::memory_order_relaxed);
        th.hzd_node_id.store(hzd_node_id, std::memory_order_seq_cst);
        idx = deq.idx.load(std::memory_order_relaxed);

        auto i = id + 1;
        auto old_val = id;
        auto new_val = 0;

        while(true) {
            for(; idx == old_val && new_val == 0; ++i) {
                auto [cell, _ignore] = findCell(ph.head, th, i);

                auto lDi = this->m_deq_idx.load(std::memory_order_relaxed);
                while (lDi <= i && !this->m_deq_idx.compare_exchange_strong(lDi, i + 1, std::memory_order_relaxed, std::memory_order_relaxed));

                auto res = this->help_enq(cell, th, i);
                if  (res == nullptr || (res != top_ptr<void>() && cell.deq_req.load(std::memory_order_relaxed) == nullptr)) {
                    new_val = i;
                } else {
                    idx = deq.idx.load(std::memory_order_acquire);
                }
            }
            if (new_val != 0) {
                if (deq.idx.compare_exchange_strong(idx, new_val, std::memory_order_release, std::memory_order_acquire)) {
                    idx = new_val;
                }

                if (idx >= new_val) {
                    new_val = 0;
                }
            }

            if (idx < 0 || deq.id.load(std::memory_order_relaxed) != id) {
                break;
            }

            auto [cell, _ignore] = findCell(ph.head, th, idx);
            DeqReq* cd = nullptr;

            if (cell.val.load(std::memory_order_relaxed) == top_ptr<void>()
                || cell.deq_req.compare_exchange_strong(cd, &deq, std::memory_order_relaxed, std::memory_order_relaxed)
                || cd == &deq) {
                deq.idx.compare_exchange_strong(idx, -idx, std::memory_order_relaxed, std::memory_order_relaxed);
                break;
            }

            old_val = idx;
            if (idx >= i) {
                i = idx + 1;
            }
        }
    }



}
#endif /* _YMC_QUEUE_HPP_ */
