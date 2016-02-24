// system.h
//  All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include <vector>
#include <map>
#include <string>

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"

// Initialization and cleanup routines

// Initialization, called before anything else
extern void Initialize(int argc, char **argv);

// Cleanup, called when Nachos is done.
extern void Cleanup();

extern Thread *currentThread;  // the thread holding the CPU
extern Thread *threadToBeDestroyed;  // the thread that just finished
extern Scheduler *scheduler;  // the ready list
extern Interrupt *interrupt;  // interrupt status
extern Statistics *stats;  // performance metrics
extern Timer *timer;  // the hardware alarm clock

#ifdef USER_PROGRAM
#include "machine.h"
#include "synchconsole.h"
#include "processtable.h"
#include "bitmap.h"

extern Machine* machine;  // user program memory and registers
extern SynchConsole* synchConsole;
extern ProcessTable* processTable;
extern BitMap* freeList;
extern std::map<SpaceId, std::vector<std::string> > userProgramArgs;
#endif

#ifdef FILESYS_NEEDED  // FILESYS or FILESYS_STUB
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
#endif

#ifdef PAGING
extern CoreMapEntry* coreMap;
extern List<int>* loadedPages;
#endif

#endif  // SYSTEM_H
