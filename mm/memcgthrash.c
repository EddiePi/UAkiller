#include <linux/cgroup.h>
#include <linux/memcgthrash.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/memcontrol.h>

//default thrashing tolerant period (Here is 20s), make it configurable to users
static const unsigned long default_curr_num=5;

//called under from inside, no lock protection
void mem_cgthrash_thrash_buffer_clear(struct mem_cgroup_thrash* cg_thrash)
{
  int i;
  for(i=0;i<MEM_CGROUP_MAX_THRASH_BUFFER;i++){
  cg_thrash->pgmj_buffers[i]=0;
  cg_thrash->pgev_buffers[i]=0;
 }
 cg_thrash->index=0; 
}



//called from ouside, procted by lock
void mem_cgroup_thrash_buffer_clear(struct mem_cgroup* memcg){
  struct mem_cgroup_thrash *cg_thrash=memcg_to_cg_thrash(memcg);
  spin_lock(&cg_thrash->sr_lock);
  mem_cgthrash_thrash_buffer_clear(cg_thrash);
  spin_unlock(&cg_thrash->sr_lock);
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
 mem_cgthrash_thrash_buffer_clear(cg_thrash);
 spin_lock_init(&cg_thrash->sr_lock);   
}

/*
*add pagemjfault and pageeviction event to lsit
*pgma comes from PGMAJFAULT@memcg1_events
*pgev comes from inactive_age@lruvec 
*/
bool mem_cgroup_thrash_add(struct mem_cgroup* memcg, unsigned long pgmj, unsigned long pgev){
  struct mem_cgroup_thrash *cg_thrash=memcg_to_cg_thrash(memcg);
  unsigned short memcg_id = mem_cgroup_id(memcg);
  spin_lock(&cg_thrash->sr_lock);
  //wait for 1s for each pgma and pgev add
  if (time_before(jiffies, cg_thrash->detec_jiffies)){
   spin_unlock(&cg_thrash->sr_lock);
   return false; 
  }

  //if kswapd has slept for more than buffer size, clear buffer
  if (!time_before(jiffies, cg_thrash->detec_jiffies+MEM_CGROUP_MAX_THRASH_BUFFER*HZ))
      mem_cgthrash_thrash_buffer_clear(cg_thrash);
 
  if (cg_thrash->index == MEM_CGROUP_MAX_THRASH_BUFFER)
       cg_thrash->index = 0;
  cg_thrash->pgmj_buffers[cg_thrash->index] = pgmj;
  cg_thrash->pgev_buffers[cg_thrash->index] = pgev;
  cg_thrash->index++; 
  cg_thrash->detec_jiffies = jiffies + HZ;
  //printk("cg_thrash %d add: %d  %d",memcg_id,pgmj,pgev);

  spin_unlock(&cg_thrash->sr_lock);

  return true;

}

bool buffers_increase(int num, int index, unsigned long* buffers){

 bool thrash=true;
 unsigned long last=-1;
 
 //simple implementation for a incraseing of stastics
  for (; num>0; num--,index--) {
     if (last==-1){
        last=buffers[index];
        continue;
     }

     if (index==0){
        index=MEM_CGROUP_MAX_THRASH_BUFFER-1;  
     }

     if (last <= buffers[index]){
        thrash=false;
        //printk("break at %d last %d now %d",num,last,buffers[index]);
        break;
     }
   
     last=buffers[index];      

  }

return thrash;

}


/*
* if the memcg thrash activities are detected for this cgroup
* return true if detected, lock is not needed for read-only operation
*/

bool mem_cgroup_thrash_on(struct mem_cgroup* memcg){
  struct mem_cgroup_thrash *cg_thrash=memcg_to_cg_thrash(memcg);
  
  //index points to the next empty slots
  int index = cg_thrash->index-1;
  int num   = cg_thrash->num;
  
  bool thrash_pgmj=buffers_increase(num,index,cg_thrash->pgmj_buffers);
  bool thrash_pgev=buffers_increase(num,index,cg_thrash->pgev_buffers);

  return thrash_pgmj||thrash_pgev; 
}
