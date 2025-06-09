#pragma once
#include"Common.h"


namespace pbc_memoryPool
{
    // 线程本地缓存
    class ThreadCache
    {
    public:
        static ThreadCache* getInstance()
        {
            static thread_local ThreadCache instance;
            return &instance;
        }

        void* allocate(size_t size);
        void deallocate(void* ptr, size_t size);

    private:
        ThreadCache() = default;
        // 从中心缓存获取内存
        void* fetchFromCentralCache(size_t index);
        // 归还内存到中心缓存
        // 注意，size虽为内存大小， 但此处理解为指示需要返回至上一级缓存的当前缓存的链表位置更为妥当
        void returnToCentralCache(void* start, size_t size);
        // 计算批量获取内存块的数量
        size_t getBatchNum(size_t size);
        // 判断是否需要归还内存给中心缓存
        bool shouldReturnToCentralCache(size_t index);

        
        // 每个线程的自由链表数组
        std::array<void*, FREE_LIST_SIZE> freeList_;
        std::array<size_t, FREE_LIST_SIZE> freeListSize_; // 统计自由链表大小
    };
} // namespace pbc_memoryPool
