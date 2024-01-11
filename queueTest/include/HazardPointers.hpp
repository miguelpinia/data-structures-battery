#ifndef _HazardPointers_HPP_
#define _HazardPointers_HPP_

#include <atomic>
#include <vector>
#include <list>
#include <iostream>
#include <map>
#include <unordered_set>

template <typename T, int K>
struct HPRec {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    std::atomic<T*> HP[K];
    std::atomic<HPRec*> next;
    std::list<T*> rlist;
    std::atomic<int> rcount;
};

template <typename T, int K>
class HazardPointers {
private:
    std::atomic<HPRec<T, K>*> Head;
    std::atomic<int> H{0};

public:
    HPRec<T, K>* allocate() {
        for(HPRec<T, K>* hprec = Head.load(); hprec != nullptr; hprec = hprec->next.load()) {
            if (hprec->flag.test()) continue;
            if (hprec->flag.test_and_set()) continue;
            return hprec;
        }

        int oldcount;
        do {
            oldcount = H.load();
        } while (!H.compare_exchange_weak(oldcount, oldcount + K));

        std::atomic<HPRec<T, K>*> hprec{H};
        std::atomic<HPRec<T, K>*> oldhead{nullptr};
        do {
            oldhead.store(Head);
            hprec.load()->next.store(oldhead);
        } while (!Head.compare_exchange_weak(oldhead, hprec));
        return hprec.load();
    }

    void retireHPRec(HPRec<T, K>* hprec) {
        for(int i = 0; i < K; i++) {
            hprec->HP[i] = nullptr;
        }
        hprec->flag.clear();
    }

    void retireNode(T* node, HPRec<T, K>* myHPRec) {
        myHPRec->list.push_front(node);
        myHPRec->rcount.fetch_add(1);
        HPRec<T, K>* head = Head.load();
        if (myHPRec->rcount >= H + 2) {
            scan(head);
            helpScan(myHPRec);
        }
    }

    void helpScan(HPRec<T, K>* myHPRec) {
        for (HPRec<T, K>* hprec = Head.load(); hprec != nullptr; hprec->next.load()) {
            if (hprec->flag.test()) continue;
            if (hprec->flag.test_and_set()) continue;
            while (hprec->rcount > 0) {
                T* node = hprec->rlist.pop_front();
                hprec->rcount.fetch_add(-1);
                myHPRec->rlist.push_back(node);
                myHPRec->rcount++;
                HPRec<T, K>* head = Head.load();
                if (myHPRec->rcount >= H + 2)
                    scan(head);
            }
            hprec->flag.clear();
        }
    }

    void scan(HPRec<T, K>* head, HPRec<T, K>* myHPRec) {
        std::unordered_set<T*> plist;
        HPRec<T, K>* hprec = head;
        while (hprec != nullptr) {
            for (int i = 0; i < K; i++) {
                T* hptr = hprec->HP[i].load();
                if (hptr != nullptr) {
                    plist.insert(hptr);
                }
            }
            hprec = hprec->next.load();
        }

        std::list<T*> tmplist;
        while (!myHPRec->rlist.empty()) {
            tmplist.push_front(myHPRec->rlist.front());
            myHPRec->rlist.pop_front();
        }
        myHPRec->rcount.store(0);
        T* node = tmplist.front();
        while (node != nullptr)  {
            if (plist.contains(node)) {
                myHPRec->rlist.push_front(node);
                myHPRec->rcount++;
            } else {
                // prepareForReuse(node);
                delete node;
            }
            tmplist.pop_front();
            node = tmplist.front();
        }
    }
};

#endif
