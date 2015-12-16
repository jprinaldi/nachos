#ifndef USERPROG_PROCESSTABLE_H_
#define USERPROG_PROCESSTABLE_H_

#include "thread.h"
#include "syscall.h"

#define MAX_NUM_PROCESSES 128

class ProcessTable {
 public:
    ProcessTable();
    ~ProcessTable();
    SpaceId AddProcess(Thread* thread);
    Thread* GetProcess(SpaceId id);
    SpaceId GetPID(Thread* thread);
    void RemoveProcess(SpaceId id);
 private:
    Thread** table;
};

#endif  // USERPROG_PROCESSTABLE_H_
