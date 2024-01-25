#ifndef _Memory_Management_Pool_HPP_
#define _Memory_Management_Pool_HPP_

#include <atomic>
#include <vector>


// We need a memory management tool. This tool must provide the
// following features:
// - Memory reclamation using hazard pointers
// - Object pooling
// - Object recycling

// To provide these features, the class must offer a set of methods
// for memory reclamation and to get the object from the pool:

// - void protect(T* value, int thread_id): this method protects the pointer from being deleted by others threads.
// - boolean retire(T* value, int thread_id): This method tries retire a pointer if possible. This means that the pointer will be deleted only if it is not used by another threads and it is safe delete it

template <typename T>
class MemoryManagementPool {
private:
    static const int HP_THREADS_MAX = 64;
    static const int HP_K_MAX = 4;
    static const int CL_PAD = 64 / sizeof(std::atomic<T*>);
    // static const int HP_THRESHOLD_R = CL_PAD / 4;

    std::atomic<T*>* hp[HP_THREADS_MAX];
    std::vector<T*> retiredList[HP_THREADS_MAX * CL_PAD];

    const int max_HP;
    const int max_Threads;
    const std::size_t threshold;

public:
    MemoryManagementPool(int max_HP = HP_K_MAX, int max_threads = HP_THREADS_MAX) :
        max_HP(max_HP), max_Threads(max_threads), threshold (2 * (max_threads * HP_K_MAX)) {
        for (int ith = 0; ith < HP_THREADS_MAX; ith++) {
            hp[ith] = new std::atomic<T*>[CL_PAD * 2];
            for (int ihp = 0; ihp < HP_K_MAX; ihp++) {
                hp[ith][ihp].store(nullptr, std::memory_order_relaxed);
            }
        }
    }

    ~MemoryManagementPool() {
        for (int ith = 0; ith < HP_THREADS_MAX; ith++) {
            delete[] hp[ith];
            for (unsigned irl = 0; irl < retiredList[ith * CL_PAD].size(); irl++) {
                delete retiredList[ith * CL_PAD][irl];
            }
        }
    }

    void clear(const int thread_id) {
        for (int ihp = 0; ihp < max_HP; ihp++) {
            hp[thread_id][ihp].store(nullptr, std::memory_order_release);
        }
    }

    void clearOne(int hp_idx, const int thread_id) {
        hp[thread_id][hp_idx].store(nullptr, std::memory_order_release);
    }

    T* protect(int hp_idx, const std::atomic<T*>& atom, const int thread_id) {
        T* n = nullptr;
        T* ret;
        while ((ret = atom.load()) != n) {
            hp[thread_id][hp_idx].store(ret, std::memory_order_seq_cst);
            n = ret;
        }
        return ret;
    }

    T* protectPointer(int hp_idx, T* pointer, const int thread_id) {
        hp[thread_id][hp_idx].store(pointer);
        return pointer;
    }

    bool retire(T* ptr, const int thread_id) {
        retiredList[thread_id * CL_PAD].push_back(ptr);
        if (retiredList[thread_id * CL_PAD].size() < threshold) return false;
        for (unsigned irl = 0; irl < retiredList[thread_id * CL_PAD].size();) {
            auto obj = retiredList[thread_id * CL_PAD][irl];
            bool canDelete = true;
            for (int tid = 0; tid < max_Threads && canDelete; tid++) {
                for (int hp_idx = max_HP - 1; hp_idx >= 0; hp_idx--) {
                    if (hp[tid][hp_idx].load() == obj) {
                        canDelete = false;
                        break;
                    }
                }
            }
            if (canDelete) {
                retiredList[thread_id * CL_PAD].erase(retiredList[thread_id * CL_PAD].begin() + irl);
                delete obj;
                continue;
            }
            irl++;
        }
        return true;
    }

};

#endif
