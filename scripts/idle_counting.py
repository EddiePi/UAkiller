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
    start=time.time()
    f=open(IDLE_BIT,"wb",BUFSIZE)
    while True:
        try:
            f.write(struct.pack("Q",pow(2,64)-1))
        except IOError as err:
            if err.errno == errno.ENXIO:
                break
            raise
    f.close()
    end=time.time()
    print "set idle: ",str(end-start)

def get_dile():
    start=time.time()
    with open(IDLE_BIT,"rb",BUFSIZE) as f:
        while f.read(BUFSIZE): pass
    end=time.time()
    print "get idle: ", str(end-start)




def page_counting():
    f_flags  = open(FLAG,"rb",BUFSIZE)
    f_cgroup = open(CGROUP,"rb",BUFSIZE)

    ##undate the idle flag
    with open(IDLE_BIT,"rb",BUFSIZE) as f:
        while f.read(BUFSIZE): pass


    idlememcg={}
    totamemcg={}
    start=time.time()
    while True:
        s1=f_flags.read(8)
        s2=f_cgroup.read(8)
        if not s1 or not s2:
            break

        flags,=struct.unpack("Q",s1)
        cgino,=struct.unpack("Q",s2)
        idle = (flags >> 25) & 1

        ##check flags        
        unevictable = (flags >> 18) & 1
        huge = (flags >> 22) & 1

        totamemcg[cgino]=totamemcg.get(cgino,0)+1
        if idle and not unevictable:
            idlememcg[cgino]=idlememcg.get(cgino,0) + 1
    
    f_flags.close()
    f_cgroup.close()
    end=time.time()
    print "counting: ",str(end-start)
    return idlememcg,totamemcg

            



if __name__=="__main__":
    set_idle()

    time.sleep(3)

    idlememcg,totamemcg=page_counting()

    for dirn,subdirs,files in os.walk(CGROUP_MOUNT+"/memory/docker"):
        ino=os.stat(dirn)[stat.ST_INO]
        print "total "+dirn+" "+str(totamemcg[ino])
        print "idle  "+dirn+" "+str(idlememcg[ino])
