#ifndef __LINUX_MEMCGTHRASH_H
#define __LINUX_MEMCGTHRASH_H
#include <linux/cgroup.h>
#include <linux/mutex.h>
#include <linux/types.h>
//addded by wei
//used to detect cgroup thrashing in kernel.

#define MEM_CGROUP_MAX_THRASH_BUFFER 60

struct mem_cgroup_thrash{
    //pointer to the circular buffer
    int index;
    //valid length of the circular buffer, must bee less than MEM_CGROUP_MAX_THRASH_BUFFER
    int num;
    //protect and avoids concurrent access
    struct spinlock sr_lock;
    unsigned long detec_jiffies;
    unsigned long pgmj_buffers[MEM_CGROUP_MAX_THRASH_BUFFER];
    unsigned long pgev_buffers[MEM_CGROUP_MAX_THRASH_BUFFER];

};


#ifdef CONFIG_MEMCG

extern void mem_cgroup_thrash_init(struct mem_cgroup_thrash* cg_thrash);
extern void mem_cgroup_thrash_add(struct mem_cgroup_thrash* cg_thrash, unsigned long pgmj, unsigned long pgev);
extern void mem_cgroup_thrash_on(struct mem_cgroup_thrash* cg_thrash);

#endif
#endif
