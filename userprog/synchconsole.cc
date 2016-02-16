#include "synchconsole.h"

static void ReadCallback(void* arg) {
    SynchConsole* synchConsole = (SynchConsole*) arg;
    synchConsole->ReadAvailable();
}

static void WriteCallback(void* arg) {
    SynchConsole* synchConsole = (SynchConsole*) arg;
    synchConsole->WriteDone();
}

SynchConsole::SynchConsole(const char* readFile, const char* writeFile) {
    console = new Console(readFile, writeFile, ReadCallback, WriteCallback, this);
    readLock = new Lock("SynchConsole read lock");
    writeLock = new Lock("SynchConsole write lock");
    readSemaphore = new Semaphore("SynchConsole read semaphore", 0);
    writeSemaphore = new Semaphore("SynchConsole write semaphore", 0);
}

SynchConsole::~SynchConsole() {
    delete console;
    delete readLock;
    delete writeLock;
    delete readSemaphore;
    delete writeSemaphore;
}

char SynchConsole::GetChar() {
    readLock->Acquire();
    readSemaphore->P();
    char c = console->GetChar();
    readLock->Release();
    return c;
}

void SynchConsole::PutChar(const char c) {
    writeLock->Acquire();
    console->PutChar(c);
    writeSemaphore->P();
    writeLock->Release();
}

void SynchConsole::ReadAvailable() {
    readSemaphore->V();
}

void SynchConsole::WriteDone() {
    writeSemaphore->V();
}
