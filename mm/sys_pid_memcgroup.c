#include<linux/kernel.h>
#include<linux/memcontrol.h>
#include<linux/sched.h>


asmlinkage int sys_pid_memcgroup(int pid){

 struct task_struct *p_task;
 int memcg_id=-1;

for_each_process(p_task) 
{
   if(p_task->pid == pid){
     memcg_id=mem_cgroup_id_from_task(p_task);
     printk("print pid: %d memcg_id %d", pid, memcg_id);
     break;
   }else{
    continue;
   }

 }
printk("memcg_id not found");
return memcg_id;
}
