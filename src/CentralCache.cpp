#include<thread>
#include<cassert>
#include"../include/PageCache.h"
#include"../include/CentralCache.h"

namespace pbc_memoryPool
{
    // 每次从PageCache获取span大小（以页为单位）
    static const size_t SPAN_PAGES = 8;

    void* CentralCache::fetchRange(size_t index, size_t batchNum)
    {
        if(index > FREE_LIST_SIZE || batchNum == 0) return nullptr;

        // 自旋锁保护
        while(locks_[index].test_and_set(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }

        void* result = nullptr;
        try
        {
            result = centralFreeList_[index].load(std::memory_order_relaxed);

            if(!result)
            {
                // 中心缓存为空，从页缓存获取新的内存块
                size_t size = (index + 1) * ALIGNMENT_BYTE;
                result = fetchFromPageCache(size);

                if(!result){
                    locks_[index].clear(std::memory_order_release);
                    return nullptr;
                }

                // 将从PageCache获取的内存块切分成小块
                char* start = static_cast<char*>(result);
                size_t totalBlocks = (SPAN_PAGES * PageCache::PAGE_SIZE) / size;
                size_t allocBlocks = std::min(totalBlocks, batchNum);

                // 构建返回给ThreadCache的内存块链表
                if(allocBlocks > 1)
                {
                    // 确保至少有两个块才构建链表
                    // 构建链表
                    void* current = result;
                    void* next = current;
                    for(size_t i = 1; i < allocBlocks; ++i)
                    {
                        current = start + (i - 1) * size;
                        next = start + i * size;
                        *reinterpret_cast<void**>(current) = next;
                    }
                    *reinterpret_cast<void**>(start + (allocBlocks - 1) * size) = nullptr;
                }

                // 构建保留在CentralCache的链表
                if(totalBlocks > allocBlocks)
                {
                    void* remainstart = start + allocBlocks * size;
                    for(size_t i = allocBlocks + 1; i<totalBlocks; ++i)
                    {
                        void* current = start + (i - 1) * size;
                        void* next = start + i * size;
                        *reinterpret_cast<void**>(current) = next;
                    }

                    *reinterpret_cast<void**>(start + (totalBlocks - 1) * size) = nullptr;

                    centralFreeList_[index].store(remainstart, std::memory_order_release);
                }
            }
            else
            {
                // 中心缓存有index对应大小的内存块
                // 从现有链表中获取指定数量的块
                void* current = result;
                void* prev = nullptr;
                size_t count = 0;

                while(current && count < batchNum)
                {
                    prev = current;
                    current = *reinterpret_cast<void**>(current);
                    count++;
                }

                if(prev) // 当前centralFreeList_[index]链表上的内存块大于batchNum时需要用到 
                {
                    *reinterpret_cast<void**>(prev) = nullptr;
                }

                centralFreeList_[index].store(current, std::memory_order_release);
            }
        }
        catch(...)
        {
            locks_[index].clear(std::memory_order_release);
            throw;
        }
        
        // 释放锁
        locks_[index].clear(std::memory_order_release);
        return result;
    }

    void CentralCache::returnRange(void* start, size_t size, size_t index)
    {
        if(!start || index > FREE_LIST_SIZE) return;

        while(locks_[index].test_and_set(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }

        try
        {
            void* end = start;
            size_t count = 1;
            while(*reinterpret_cast<void**>(end) != nullptr && count < size)
            {
                end = *reinterpret_cast<void**>(end);
                count++;
            }

            // 将归还的链表连接到中心缓存的链表头部
            void* current = centralFreeList_[index].load(std::memory_order_relaxed);
            *reinterpret_cast<void**>(end) = current;
            centralFreeList_[index].store(start, std::memory_order_release);
        }
        catch(...)
        {
            locks_[index].clear();
            throw;
        }
        
        // 释放锁
        locks_[index].clear();
    }

    void* CentralCache::fetchFromPageCache(size_t size)
    {
        // 1. 计算实际需要的页数
        size_t numPages = (size + PageCache::PAGE_SIZE - 1) / PageCache::PAGE_SIZE;

        // 2. 根据大小决定分配策略
        if(size <= SPAN_PAGES * PageCache::PAGE_SIZE)
        {
            // 小于等于32KB的请求，使用固定8页
            return PageCache::getInstance().allocateSpan(SPAN_PAGES);
        }
        else
        {
            // 大于32KB的请求，按实际需求分配
            return PageCache::getInstance().allocateSpan(numPages);
        }

    }
    
} // namespace pbc_memoryPool
