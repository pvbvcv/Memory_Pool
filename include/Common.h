#pragma once
#include<cstddef>
#include<array>
#include<atomic>

namespace pbc_memoryPool
{
    // 对齐数和大小定义
    constexpr size_t ALIGNMENT_BYTE = 8;
    constexpr size_t MAX_BYTES = 256 * 1024;
    constexpr size_t FREE_LIST_SIZE = MAX_BYTES / ALIGNMENT_BYTE;
    
    //大小类管理
    class Sizemanager{
    public:
        static size_t getindex(size_t size_byte)
        {
            // 确保size_byte至少为ALIGNMENT_BYTE
            size_byte = std::max(size_byte, ALIGNMENT_BYTE);
            // 向上取整后-1
            return (size_byte + ALIGNMENT_BYTE - 1) / ALIGNMENT_BYTE -1;
        }

        static size_t roundUp(size_t size_byte)
        {
            // 将 bytes 向上舍入到一个对齐的边界
            return (size_byte + ALIGNMENT_BYTE - 1) & ~(ALIGNMENT_BYTE - 1);
        }
    };
}