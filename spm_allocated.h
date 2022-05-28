#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

void *spm_mem_alloc(uint64_t mem_size, uint64_t mem_address)
{
    uint64_t alloc_mem_size, page_mask, page_size;
    void *mem_pointer;
    void *virt_addr;


//sysconf函数在头文件 <unistd.h> 中
    page_size = sysconf(_SC_PAGESIZE);
    alloc_mem_size = (((mem_size / page_size) + 1) * page_size);
    page_mask = (page_size - 1);


//open函数需要包含<sys/types.h> <sys/stat.h>  <fcntl.h>

    int mem_dev = open("/dev/mem", O_RDWR | O_SYNC);
    if (mem_dev == -1)
    {
        perror("Cannot open /dev/mem \n");
    }

//mman函数需要包含头文件 <sys/mman.h> <unistd.h>
    mem_pointer = mmap(NULL,
                       alloc_mem_size,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED,
                       mem_dev,
                       (mem_address & ~page_mask));
    if (mem_pointer == MAP_FAILED)
    {
        perror("Cannot MAP \n");
    }

    // printf("Memory Mapped \n");
    virt_addr = (mem_pointer + (mem_address & page_mask));

    return virt_addr;
}
