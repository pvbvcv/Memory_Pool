#include<sys/mman.h>
#include<cstring>
#include"../include/PageCache.h"

namespace pbc_memoryPool
{

    void* PageCache::allocateSpan(size_t numPages)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = freeSpans_.lower_bound(numPages);
        if (it != freeSpans_.end())
        {
            Span* span = it->second;

            // 将取出的span从原有的空闲链表freeSpans_[it->first]中移除
            if(span->next)
            {
                freeSpans_[it->first] = span->next;
            }else{
                freeSpans_.erase(it);
            }

            // 如果span大于需要的numPages则进行分割
            if(span->numPages > numPages)
            {
                Span* newSpan = new Span;
                newSpan->pageAddr = static_cast<char*>(span->pageAddr) + 
                                    numPages * PAGE_SIZE;          
                // *reinterpret_cast<void**>(newSpan->pageAddr) = nullptr;
                newSpan->numPages = span->numPages - numPages;
                newSpan->next = nullptr;

                // 将超出部分放回空闲Span*列表头部
                auto& list = freeSpans_[newSpan->numPages];
                newSpan->next = list;
                list = newSpan;

                span->numPages = numPages;
            }

            // 记录span信息用于回收
            spanMap_[span->pageAddr] = span;
            return span->pageAddr;
        }

        // 没有合适的span，向系统申请
        void* memory = systemAlloc(numPages);
        if(!memory) return nullptr;

        // 创建新的span
        Span* span = new Span;
        span->pageAddr = memory;
        span->numPages = numPages;
        span->next = nullptr;

        spanMap_[span->pageAddr] = span;
        return memory;
    }

    void PageCache::deallocateSpan(void* ptr, size_t numPages)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = spanMap_.find(ptr);
        if(it == spanMap_.end()) return;

        Span* span = it->second;

        // 尝试合并相邻的span
        void* nextAddr = static_cast<char*>(ptr) + numPages * PAGE_SIZE;
        auto nextIt = spanMap_.find(nextAddr);
        
        if(nextIt != spanMap_.end()){
            Span* nextspan = nextIt->second;

            // 1. 首先检查nextSpan是否在空闲链表中
            bool found = false;
            auto& nextlist = freeSpans_[nextspan->numPages];

            // 检查是否是头节点
            if(nextlist == nextspan)
            {
                found = true;
                nextlist = nextlist -> next;
            }else if(nextlist)
            {
                Span* prev = nextlist;
                while(prev->next){
                    if(prev->next == nextspan){
                        found = true;
                        prev->next = nextspan->next;
                        break;
                    }
                    prev = prev->next;
                }
            }

            // 2. 只有在找到nextSpan的情况下才进行合并
            if(found)
            {
                span->numPages += nextspan->numPages;
                spanMap_.erase(nextAddr);
                delete nextspan;
            }
        }
        // 将合并后的span通过头插法插入空闲列表
        auto& list = freeSpans_[span->numPages];
        span->next = list;
        list = span;
    }

    void* PageCache::systemAlloc(size_t numPages)
    {
        size_t size = numPages * PAGE_SIZE;

        // 使用mmap分配内存
        void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if(ptr == MAP_FAILED) return nullptr;

        // 清零内存
        memset(ptr, 0, size);
        return ptr;
    }

}