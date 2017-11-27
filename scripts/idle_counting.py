import os
import stat
import errno
import struct
import time

CGROUP_MOUNT="/sys/fs/cgroup"
BUFSIZE=8*1024*1024

IDLE_BIT="/sys/kernel/mm/page_idle/bitmap"
FLAG  ="/proc/kpageflags"
CGROUP="/proc/kpagecgroup"
PAGE_SIZE=os.sysconf("SC_PAGE_SIZE")


def set_idle():
    f=open(IDLE_BIT,"wb",BUFSIZE)
    while True:
        try:
            f.write(struct.pack("Q",pow(2,64)-1))
        except IOError as err:
            if err.errno == errno.ENXIO:
                break
            raise
    f.close()

def page_counting():
    f_flags  = open(FLAG,"rb",BUFSIZE)
    f_cgroup = open(CGROUP,"rb",BUFSIZE)

    ##undate the idle flag
    with open(IDLE_BIT,"rb",BUFSIZE) as f:
        while f.read(BUFSIZE): pass


    idlememcg={}
    while True:
        s1=f_flags.read(8)
        s2=f_cgroup.read(8)
        if not s1 or not s2:
            break

        flags,=struct.unpack("Q",s1)
        cgino,=struct.unpack("Q",s2)

        ##check flags        
        unevictable = (flags >> 18) & 1
        huge = (flags >> 22) & 1
        idle = (flags >> 25) & 1

        if idle and not unevictable:
            idlememcg[cgino]=idlememcg.get(cgino,0) + 1
    
    f_flags.close()
    f_cgroup.close()
    return idlememcg

            



if __name__=="__main__":
    time.sleep(1)
    ##clear the idle
    set_idle()
    time.sleep(60)
    ##do page counting
    idlememcg=page_counting()
    ##walk through all cgroups
    for dirn, subdirs, files in os.walk(CGROUP_MOUNT):
        ino=os.stat(dirn)[stat.ST_INO]
        print dirn+" : "+str(idlememcg.get(ino,0))


    
