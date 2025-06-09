# 1.项目介绍
本项目是基于 C++ 实现的自定义内存池框架，旨在提高内存分配和释放的效率

内存池包含三级缓存机制
- 线程本地缓存（ThreadCache）：每个线程维护自己的内存块链表，减少线程间的锁竞争，提高内存分配效率
- 中心缓存（CentralCache）：用于管理多个线程共享的内存块，支持批量分配和回收，优化内存利用率
- 页面缓存（PageCache）：负责从操作系统申请和释放大块内存，支持内存块的合并和分割，减少内存碎片
- 自旋锁和原子操作：在多线程环境下使用自旋锁和原子操作，确保线程安全的同时减少锁的开销
![framework](https://github.com/user-attachments/assets/aea9e649-a851-47c2-90e7-8d3b212a8d20)

# 2.项目编译
- `cd Memory_Pool`
- `mkdir build && cd build`
- `cmake ..`
- `make`
# 3.项目测试
- `./unitTest`

![unitTest](https://github.com/user-attachments/assets/528607b1-5ae4-4725-bfd6-f3df785a7147)

- `./performTest`

![performanceTest](https://github.com/user-attachments/assets/497189f2-ccb9-4c13-827a-ecf89f8d67ab)

