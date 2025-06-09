#pragma once
#include<mutex>
#include"Common.h"

namespace pbc_memoryPool
{
    
    class CentralCache{
    public:
        static CentralCache& getInstance()
        {
            static CentralCache instance;
            return instance;
        }

        void* fetchRange(size_t index, size_t batchNum);
        void returnRange(void* start, size_t size, size_t index);
        // 从页缓存获取内存
        void* fetchFromPageCache(size_t size);
    private:

        CentralCache()
        {
            for(auto& ptr : centralFreeList_)
            {
                ptr.store(nullptr, std::memory_order_relaxed);
            }

            for(auto& lock : locks_)
            {
                lock.clear();
            }
        }

    private:

        // 中心缓存的自由链表
        std::array<std::atomic<void*>, FREE_LIST_SIZE> centralFreeList_;

        // 用于同步的自旋锁
        std::array<std::atomic_flag, FREE_LIST_SIZE> locks_;
    };

} // namespace pbc_memoryPool
