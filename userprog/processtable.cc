#include "processtable.h"

ProcessTable::ProcessTable() {
    table = new Thread*[MAX_NUM_PROCESSES];

    for (SpaceId pid = 0; pid < MAX_NUM_PROCESSES; pid++) {
        table[pid] = NULL;
    }
}

ProcessTable::~ProcessTable() {
    delete[] table;
}

SpaceId ProcessTable::AddProcess(Thread* thread) {
    for (int pid = 0; pid < MAX_NUM_PROCESSES; pid++) {
        if (table[pid] == NULL) {
            table[pid] = thread;
            return pid;
        }
    }
    // process_table full
    return -1;
}

Thread* ProcessTable::GetProcess(SpaceId pid) {
    if (pid >= 0 && pid < MAX_NUM_PROCESSES) {
        return table[pid];
    } else {
        return NULL;
    }
}

SpaceId ProcessTable::GetPID(Thread* thread) {
    for (int pid = 0; pid < MAX_NUM_PROCESSES; pid++) {
        if (table[pid] == thread) {
            return pid;
        }
    }
    return -1;
}

void ProcessTable::RemoveProcess(SpaceId pid) {
    table[pid] = NULL;
}
