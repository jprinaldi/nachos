#include "synchconsole.h"

static void ReadCallback(void* arg) {
    SynchConsole* synch_console = (SynchConsole*) arg;
    synch_console->ReadAvailable();
}

static void WriteCallback(void* arg) {
    SynchConsole* synch_console = (SynchConsole*) arg;
    synch_console->WriteDone();
}

SynchConsole::SynchConsole(const char* read_file, const char* write_file) {
    console = new Console(read_file, write_file, ReadCallback, WriteCallback, this);
    read_lock = new Lock("SynchConsole read lock");
    write_lock = new Lock("SynchConsole write lock");
    read_semaphore = new Semaphore("SynchConsole read semaphore", 0);
    write_semaphore = new Semaphore("SynchConsole write semaphore", 0);
}

SynchConsole::~SynchConsole() {
    delete console;
    delete read_lock;
    delete write_lock;
    delete read_semaphore;
    delete write_semaphore;
}

char SynchConsole::GetChar() {
    read_lock->Acquire();
    read_semaphore->P();
    char c = console->GetChar();
    read_lock->Release();
    return c;
}

void SynchConsole::PutChar(const char c) {
    write_lock->Acquire();
    console->PutChar(c);
    write_semaphore->P();
    write_lock->Release();
}

void SynchConsole::ReadAvailable() {
    read_semaphore->V();
}

void SynchConsole::WriteDone() {
    write_semaphore->V();
}
