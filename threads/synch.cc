// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(const char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List<Thread*>;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	    queue->Append(currentThread);		// so go to sleep
	    currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    interrupt->SetLevel(oldLevel);		// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = queue->Remove();
    if (thread != NULL) {	   // make thread ready, consuming the V immediately
	    scheduler->ReadyToRun(thread);
    }
    value++;
    interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(const char* debugName)
{
    s = new Semaphore("lock semaphore", 1);
    aux = new Semaphore("aux semaphore", 1);
    owner = NULL;
}

Lock::~Lock()
{
    delete s;
    delete aux;
}

void Lock::Acquire()
{
    int currentThreadPriority;
    int ownerPriority;
    
    ASSERT(!isHeldByCurrentThread());
    
    aux->P();
    
    if (owner != NULL) {
        currentThreadPriority = currentThread->getPriority();
        ownerPriority = currentThread->getPriority();
        if (ownerPriority < currentThreadPriority) {
            owner->setPriority(currentThreadPriority);
            scheduler->Move(owner, ownerPriority);
        }
    }
    
    aux->V();
    
    s->P();
    owner = currentThread;
}

void Lock::Release()
{
    ASSERT(isHeldByCurrentThread());
    
    if (currentThread->getPriority() != currentThread->getInitialPriority()) {
        currentThread->setPriority(currentThread->getInitialPriority());
        scheduler->Move(currentThread, currentThread->getPriority());
    }
    
    owner = NULL;
    s->V();
}

bool Lock::isHeldByCurrentThread()
{
    return currentThread == owner;
}

ConditionThread::ConditionThread() {
    t = currentThread;
    s = new Semaphore("Condition Thread Semaphore",0);
};

ConditionThread::~ConditionThread() {
    delete s;
};

Condition::Condition(const char* debugName, Lock* conditionLock) { }
Condition::~Condition() { }

void Condition::Wait(Lock* lock)
{
    ConditionThread* ct = new ConditionThread();
    queue.Append(ct);
    lock->Release();
    ct->s->P();
    lock->Acquire();
    delete ct;
}

void Condition::Signal(Lock* lock)
{
    if (!queue.IsEmpty()) {
        ConditionThread* ct2 = queue.Remove();
        ct2->s->V();
    }
}

void Condition::Broadcast(Lock* lock)
{
    while (!queue.IsEmpty()) {
        ConditionThread* ct2 = queue.Remove();
        ct2->s->V();
    }
}

Port::Port()
{
    lock = new Lock("PortLock");
    sCondition = new Condition("SenderCondition",lock);
    rCondition = new Condition("ReceiverCondition",lock);
    bufferEmpty = true;
}

Port::~Port()
{
    delete lock;
    delete sCondition;
    delete rCondition;
}

void Port::Send(int msg)
{
    lock->Acquire();
    
    while (!bufferEmpty) {
        sCondition->Wait(lock);
    }
    
    buffer = msg;
    bufferEmpty = false;
    
    rCondition->Signal(lock);
    
    lock->Release();
    
}

int Port::Receive()
{
    lock->Acquire();
    
    while (bufferEmpty) {
        rCondition->Wait(lock);
    }
    
    int msg = buffer;
    
    bufferEmpty = true;
    
    lock->Release();
    
    return msg;
}
