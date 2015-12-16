#ifndef USERPROG_SYNCHCONSOLE_H_
#define USERPROG_SYNCHCONSOLE_H_

#include "console.h"
#include "synch.h"

class SynchConsole {
 public:
    SynchConsole(const char* read_file, const char* write_file);
    ~SynchConsole();
    char GetChar();
    void PutChar(char c);
    void ReadAvailable();
    void WriteDone();

 private:
    Console* console;
    Lock *read_lock, *write_lock;
    Semaphore *read_semaphore, *write_semaphore;
};

#endif  // USERPROG_SYNCHCONSOLE_H_
