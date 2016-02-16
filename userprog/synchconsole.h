#ifndef USERPROG_SYNCHCONSOLE_H_
#define USERPROG_SYNCHCONSOLE_H_

#include "console.h"
#include "synch.h"

class SynchConsole {
 public:
    SynchConsole(const char* readFile, const char* writeFile);
    ~SynchConsole();
    char GetChar();
    void PutChar(char c);
    void ReadAvailable();
    void WriteDone();

 private:
    Console* console;
    Lock *readLock, *writeLock;
    Semaphore *readSemaphore, *writeSemaphore;
};

#endif  // USERPROG_SYNCHCONSOLE_H_
