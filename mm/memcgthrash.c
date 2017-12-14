#include <linux/cgroup.h>
#include <linux/memcgthrash.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/memcontrol.h>

//default thrashing tolerant period (Here is 20s), make it configurable to users
static const unsigned long default_curr_num=20;


//should be called under lock protection
void mem_cgroup_thrash_buffer_clear(struct mem_cgroup_thrash* cg_thrash)
{
 int i;
 for(i=0;i<MEM_CGROUP_MAX_THRASH_BUFFER;i++){
  cg_thrash->pgmj_buffers[i]=0;
  cg_thrash->pgev_buffers[i]=0;
 }
 cg_thrash->index=0; 
}



/**
* mem_cgroup_thrash_init() - Initialize mem_cgroup_thrash control structure
* @cg_thrash structure to be initialized
*
  mem alloc should be done @mem_cgroup_alloc  
* This function should be called on the creation of mem_cgroup
*/

void mem_cgroup_thrash_init(struct mem_cgroup_thrash* cg_thrash)
{
 cg_thrash->num  =default_curr_num;
 cg_thrash->index=0;
 cg_thrash->detec_jiffies=jiffies;
 mem_cgroup_thrash_buffer_clear(cg_thrash);
 spin_lock_init(&cg_thrash->sr_lock);   
}


/*
*add pagemjfault and pageeviction event to lsit
*pgma comes from PGMAJFAULT@memcg1_events
*pgev comes from inactive_age@lruvec 
*/
void mem_cgroup_thrash_add(struct mem_cgroup* memcg, unsigned long pgmj, unsigned long pgev){
  struct mem_cgroup_thrash *cg_thrash=memcg_to_cg_thrash(memcg);
  unsigned short memcg_id = mem_cgroup_id(memcg);
  spin_lock(&cg_thrash->sr_lock);
  //wait for 1s for each pgma and pgev add
  if (time_before(jiffies, cg_thrash->detec_jiffies)){
   spin_unlock(&cg_thrash->sr_lock);
   return; 
  }

  //if kswapd has slept for more than buffer size, clear buffer
  if (!time_before(jiffies, cg_thrash->detec_jiffies+MEM_CGROUP_MAX_THRASH_BUFFER*HZ))
      mem_cgroup_thrash_buffer_clear(cg_thrash);
 
  if (cg_thrash->index == MEM_CGROUP_MAX_THRASH_BUFFER)
       cg_thrash->index = 0;
  cg_thrash->pgmj_buffers[cg_thrash->index] = pgmj;
  cg_thrash->pgev_buffers[cg_thrash->index] = pgev;
  cg_thrash->index++; 
  cg_thrash->detec_jiffies = jiffies + HZ;
  printk("cg_thrash %d add: %d  %d",memcg_id,pgmj,pgev);

  spin_unlock(&cg_thrash->sr_lock);

}
