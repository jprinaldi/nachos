// exception.cc
//  Entry point into the Nachos kernel from user programs.
//  There are two kinds of things that can cause control to
//  transfer back to here from user code:
//
//  syscall -- The user code explicitly requests to call a procedure
//  in the Nachos kernel.  Right now, the only function we support is
//  "Halt".
//
//  exceptions -- The user code does something that the CPU can't handle.
//  For instance, accessing memory that doesn't exist, arithmetic errors,
//  etc.
//
//  Interrupts (which can also cause control to transfer from user
//  code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

void readStrFromUsr(int userAddress, char *outString) {
    int value;
    int i = 0;

    do {
        if (!machine->ReadMem(userAddress + i, 1, &value)) {
            ASSERT(machine->ReadMem(userAddress + i, 1, &value));
        }
        outString[i++] = value;
    } while (value);
}

void readBuffFromUsr(int userAddress, char *outBuffer, int byteCount) {
    int value;

    for (int i = 0; i < byteCount; i++) {
        if (!machine->ReadMem(userAddress + i, 1, &value)) {
            ASSERT(machine->ReadMem(userAddress + i, 1, &value));
        }
        outBuffer[i] = value;
    }

    outBuffer[byteCount] = 0;
}

void writeStrToUsr(char *str, int userAddress) {
    int i = 0;

    do {
        if (!machine->WriteMem(userAddress + i, 1, str[i])) {
            ASSERT(machine->WriteMem(userAddress + i, 1, str[i]));
        }
    } while (str[i++]);
}

void writeBuffToUsr(char *str, int userAddress, int byteCount) {
    for (int i = 0; i < byteCount; i++) {
        if (!machine->WriteMem(userAddress + i, 1, str[i])) {
            ASSERT(machine->WriteMem(userAddress + i, 1, str[i]));
        }
    }

    if (!machine->WriteMem(userAddress + byteCount, 1, 0)) {
        ASSERT(machine->WriteMem(userAddress + byteCount, 1, 0));
    }
}

void IncrementProgramCounter() {
    int programCounter = machine->ReadRegister(PCReg);
    machine->WriteRegister(PrevPCReg, programCounter);
    programCounter = machine->ReadRegister(NextPCReg);
    machine->WriteRegister(PCReg, programCounter);
    machine->WriteRegister(NextPCReg, programCounter + 4);
}

void PrepareProcess(void* arg) {
    char* filename = reinterpret_cast<char*>(arg);
    OpenFile* executable = fileSystem->Open(reinterpret_cast<char*>(filename));
    if (executable == NULL) {
        DEBUG('c', "Could not open file %s\n", filename);
        machine->WriteRegister(2, -1);
        delete[] filename;
        return;
    }
    delete[] filename;
    AddrSpace* addressSpace = new AddrSpace(executable);
    #ifndef DEMAND_PAGING
    delete executable;
    #endif
    currentThread->space = addressSpace;
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();
    machine->Run();
}

//----------------------------------------------------------------------
// ExceptionHandler
//  Entry point into the Nachos kernel.  Called when a user program
//  is executing, and either does a syscall, or generates an addressing
//  or arithmetic exception.
//
//  For system calls, the following is the calling convention:
//
//  system call code -- r2
//      arg1 -- r4
//      arg2 -- r5
//      arg3 -- r6
//      arg4 -- r7
//
//  The result of the system call, if any, must be put back into r2.
//
//  And don't forget to increment the pc before returning. (Or else you'll
//  loop making the same system call forever!
//
//  "which" is the kind of exception.  The list of possible exceptions
//  are in machine.h.
//----------------------------------------------------------------------

void Halt() {
    DEBUG('a', "Shutdown, initiated by user program.\n");
    interrupt->Halt();
}

void Create() {
    int filenameAddr = machine->ReadRegister(4);
    int fileSize = 0;
    char* filename = new char[128];
    readStrFromUsr(filenameAddr, filename);

    if (fileSystem->Create(filename, fileSize)) {
        DEBUG('c', "File '%s' created successfully.\n", filename);
    } else {
        DEBUG('c', "Could not create file '%s'.", filename);
    }

    delete filename;
}

void Read() {
    int bufferAddress = machine->ReadRegister(4);
    int size = machine->ReadRegister(5);
    OpenFileId fileDescriptor = machine->ReadRegister(6);
    char* buffer = new char[128];
    int bytesReadCount;

    switch (fileDescriptor) {
        case ConsoleOutput:
            bytesReadCount = -1;  // error
            break;
        case ConsoleInput:
            for (bytesReadCount = 0; bytesReadCount < size; bytesReadCount++) {
                buffer[bytesReadCount] = synchConsole->GetChar();
            }
            break;
        default:
            OpenFile* openFile = currentThread->GetFile(fileDescriptor);
            bytesReadCount = openFile->Read(buffer, size);
            break;
    }

    machine->WriteRegister(2, bytesReadCount);
    writeBuffToUsr(buffer, bufferAddress, bytesReadCount);
    delete[] buffer;
}

void Write() {
    int bufferAddress = machine->ReadRegister(4);
    int size = machine->ReadRegister(5);
    OpenFileId fileDescriptor = machine->ReadRegister(6);
    char buffer[128];

    readBuffFromUsr(bufferAddress, buffer, size);

    switch (fileDescriptor) {
        case ConsoleInput:
            break;
        case ConsoleOutput:
            for (int i = 0; i < size; i++) {
                synchConsole->PutChar(buffer[i]);
            }
            break;
        default:
            OpenFile* openFile = currentThread->GetFile(fileDescriptor);
            openFile->Write(buffer, size);
            break;
    }
}

void Open() {
    int filenameAddress = machine->ReadRegister(4);
    char* filename = new char[128];
    readStrFromUsr(filenameAddress, filename);
    OpenFile* openFile = fileSystem->Open(filename);
    delete[] filename;
    if (!openFile) {
        DEBUG('c', "Could not open file: %s\n", filename);
        machine->WriteRegister(2, -1);
    } else {
        OpenFileId fileDescriptor = currentThread->AddFile(openFile);
        machine->WriteRegister(2, fileDescriptor);
        DEBUG('c', "Opened file: %d\n", fileDescriptor);
    }
}

void Close() {
    OpenFileId fileDescriptor = machine->ReadRegister(4);
    currentThread->RemoveFile(fileDescriptor);
}

void Exit() {
    int exitStatus = machine->ReadRegister(4);
    currentThread->setExitStatus(exitStatus);
    currentThread->Finish();
}

void Join() {
    SpaceId pid = machine->ReadRegister(4);
    Thread* thread = processTable->GetProcess(pid);
    if (thread) {
        machine->WriteRegister(2, thread->getExitStatus());
        thread->Join();
    } else {
        DEBUG('c', "Could not find process with id %d\n", pid);
        machine->WriteRegister(2, -1);
    }
}

void Exec() {
    int filenameAddress = machine->ReadRegister(4);
    int argvAddress = machine->ReadRegister(5);
    char* filename = new char[128];  // size?
    char* argv = new char[128];
    readStrFromUsr(filenameAddress, filename);
    readStrFromUsr(argvAddress, argv);

    char arg[128];

    Thread* thread = new Thread(filename, true);
    if (thread == NULL) {
        DEBUG('c', "Could not create thread");
        machine->WriteRegister(2, -1);
        delete[] filename;
        delete[] argv;
        return;
    }

    SpaceId pid = processTable->GetPID(thread);

    int i = 0, k = 0;
    do {
        if (argv[i] == ' ' || argv[i] == '\0') {
            arg[k] = '\0';
            k = 0;
            userProgramArgs[pid].push_back(arg);
        } else {
            arg[k++] = argv[i];
        }
    } while (argv[i++] != '\0');

    machine->WriteRegister(2, pid);

    thread->Fork(PrepareProcess, reinterpret_cast<void*>(filename));

    delete[] argv;
}

void GetArgN() {
    int argIndex = machine->ReadRegister(4);
    int argAddress = machine->ReadRegister(5);
    int argLenAddress = machine->ReadRegister(6);
    SpaceId pid = processTable->GetPID(currentThread);
    char* arg = (char*)userProgramArgs[pid].at(argIndex).c_str();
    writeStrToUsr(arg, argAddress);
    machine->WriteMem(argLenAddress, 4, strlen(arg));
}

void GetNArgs() {
    int nAddress = machine->ReadRegister(4);
    SpaceId pid = processTable->GetPID(currentThread);
    std::vector<std::string> args = userProgramArgs[pid];
    machine->WriteMem(nAddress, 4, args.size());
}

void
ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2);

    if (which == SyscallException) {
        switch (type) {
            case SC_Halt:
                Halt();
                break;
            case SC_Create:
                Create();
                break;
            case SC_Read:
                Read();
                break;
            case SC_Write:
                Write();
                break;
            case SC_Open:
                Open();
                break;
            case SC_Close:
                Close();
                break;
            case SC_Exit:
                Exit();
                break;
            case SC_Join:
                Join();
                break;
            case SC_Exec:
                Exec();
                break;
            case SC_GetArgN:
                GetArgN();
                break;
            case SC_GetNArgs:
                GetNArgs();
                break;
            default:
                printf("Unexpected user mode exception %d %d\n", which, type);
                ASSERT(false);
                break;
        }
        IncrementProgramCounter();
    } else if (which == PageFaultException) {
        int badVirtualAddress = machine->ReadRegister(BadVAddrReg);
        int virtualPageNumber = badVirtualAddress / PageSize;
        int i;

        TranslationEntry* entry =
            currentThread->space->GetPage(virtualPageNumber);

        for (i = 0; i < TLBSize; i++) {
            if (!machine->tlb[i].valid) {
                break;
            }
        }

        if (i == TLBSize) {
            i = rand() % TLBSize;
        }

        machine->tlb[i].virtualPage = entry->virtualPage;
        machine->tlb[i].physicalPage = entry->physicalPage;
        machine->tlb[i].valid = true;
        machine->tlb[i].use = entry->use;
        machine->tlb[i].dirty = entry->dirty;
        machine->tlb[i].readOnly = entry->readOnly;
    } else if (which == ReadOnlyException) {
        ASSERT(false);
    } else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(false);
    }
}
