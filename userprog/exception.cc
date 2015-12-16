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

void readStrFromUsr(int user_address, char *out_string) {
    int value;
    int i = 0;

    do {
        machine->ReadMem(user_address + i, 1, &value);
        out_string[i++] = value;
    } while (value);
}

void readBuffFromUsr(int user_address, char *out_buffer, int byte_count) {
    int value;

    for (int i = 0; i < byte_count; i++) {
        machine->ReadMem(user_address + i, 1, &value);
        out_buffer[i] = value;
    }

    out_buffer[byte_count] = 0;
}

void writeStrToUsr(char *str, int user_address) {
    int i = 0;

    do {
        machine->WriteMem(user_address + i, 1, str[i]);
    } while (str[i++]);
}

void writeBuffToUsr(char *str, int user_address, int byte_count) {
    for (int i = 0; i < byte_count; i++) {
        machine->WriteMem(user_address + i, 1, str[i]);
    }

    machine->WriteMem(user_address + byte_count, 1, 0);
}

void IncrementProgramCounter() {
    int program_counter = machine->ReadRegister(PCReg);
    machine->WriteRegister(PrevPCReg, program_counter);
    program_counter = machine->ReadRegister(NextPCReg);
    machine->WriteRegister(PCReg, program_counter);
    machine->WriteRegister(NextPCReg, program_counter + 4);
}

void PrepareProcess(void* arg) {
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

void sc_halt() {
    DEBUG('a', "Shutdown, initiated by user program.\n");
    interrupt->Halt();
}

void sc_create() {
    int filenameAddr = machine->ReadRegister(4);
    int fileSize = 0;
    char* filename = new char[255];
    readStrFromUsr(filenameAddr, filename);

    if (fileSystem->Create(filename, fileSize)) {
        DEBUG('c', "File '%s' created successfully.\n", filename);
    } else {
        DEBUG('c', "Could not create file '%s'.", filename);
    }

    delete filename;
}

void sc_read() {
    int buffer_address = machine->ReadRegister(4);
    int size = machine->ReadRegister(5);
    OpenFileId file_descriptor = machine->ReadRegister(6);
    char* buffer = new char[255];
    int bytes_read;

    switch (file_descriptor) {
        case ConsoleOutput:
            bytes_read = -1;  // error
            break;
        case ConsoleInput:
            for (bytes_read = 0; bytes_read < size; bytes_read++) {
                buffer[bytes_read] = synch_console->GetChar();
            }
            break;
        default:
            OpenFile* open_file = currentThread->GetFile(file_descriptor);
            bytes_read = open_file->Read(buffer, size);
            break;
    }

    machine->WriteRegister(2, bytes_read);
    writeBuffToUsr(buffer, buffer_address, bytes_read);
    delete buffer;
}

void sc_write() {
    int buffer_address = machine->ReadRegister(4);
    int size = machine->ReadRegister(5);
    OpenFileId file_descriptor = machine->ReadRegister(6);
    char buffer[1024];

    readBuffFromUsr(buffer_address, buffer, size);

    switch (file_descriptor) {
        case ConsoleInput:
            break;
        case ConsoleOutput:
            for (int i = 0; i < size; i++) {
                synch_console->PutChar(buffer[i]);
            }
            break;
        default:
            OpenFile* open_file = currentThread->GetFile(file_descriptor);
            open_file->Write(buffer, size);
            break;
    }
}

void sc_open() {
    int filename_address = machine->ReadRegister(4);
    char* filename = new char[1024];
    readStrFromUsr(filename_address, filename);
    OpenFile* open_file = fileSystem->Open(filename);
    delete filename;
    if (!open_file) {
        DEBUG('c', "Could not open file: %s\n", filename);
        machine->WriteRegister(2, -1);
    } else {
        OpenFileId file_descriptor = currentThread->AddFile(open_file);
        machine->WriteRegister(2, file_descriptor);
        DEBUG('c', "Opened file: %d\n", file_descriptor);
    }
}

void sc_close() {
    OpenFileId file_descriptor = machine->ReadRegister(4);
    currentThread->RemoveFile(file_descriptor);
}

void sc_exit() {
    int exit_status = machine->ReadRegister(4);
    currentThread->setExitStatus(exit_status);
    currentThread->Finish();
}

void sc_join() {
    SpaceId pid = machine->ReadRegister(4);
    Thread* thread = process_table->GetProcess(pid);
    if (thread) {
        thread->Join();
        machine->WriteRegister(2, thread->getExitStatus());
    } else {
        DEBUG('c', "Could not find process with id %d\n", pid);
        machine->WriteRegister(2, -1);
    }
}

void sc_exec() {
    int filename_address = machine->ReadRegister(4);
    int argv_address = machine->ReadRegister(5);
    char* filename = new char[255];  // size?
    char* argv = new char[255];
    readStrFromUsr(filename_address, filename);
    readStrFromUsr(argv_address, argv);

    char arg[255];

    OpenFile* file = fileSystem->Open(filename);
    if (file == NULL) {
        DEBUG('c', "Could not open file %s\n", filename);
        machine->WriteRegister(2, -1);
        delete filename;
        return;
    }

    Thread* thread = new Thread(filename, true);
    if (thread == NULL) {
        DEBUG('c', "Could not create thread");
        machine->WriteRegister(2, -1);
        delete filename;
        return;
    }

    AddrSpace* address_space = new AddrSpace(file);
    if (address_space == NULL) {
        DEBUG('c', "Could not create address space for file %s\n", filename);
        machine->WriteRegister(2, -1);
        delete filename;
        return;
    }

    thread->space = address_space;

    SpaceId pid = process_table->AddProcess(thread);
    // check if process could not be added

    int i = 0, k = 0;
    do {
        if (argv[i] == ' ' || argv[i] == '\0') {
            arg[k] = '\0';
            k = 0;
            user_program_args[pid].push_back(arg);
        } else {
            arg[k++] = argv[i];
        }
    } while (argv[i++] != '\0');

    machine->WriteRegister(2, pid);

    thread->Fork(PrepareProcess, NULL);

    delete filename;
}

void sc_get_arg_n() {
    int arg_index = machine->ReadRegister(4);
    int arg_address = machine->ReadRegister(5);
    int arg_len_address = machine->ReadRegister(6);
    SpaceId pid = process_table->GetPID(currentThread);
    char* arg = (char*)user_program_args[pid].at(arg_index).c_str();
    writeStrToUsr(arg, arg_address);
    machine->WriteMem(arg_len_address, 4, strlen(arg));
}

void sc_get_n_args() {
    int n_address = machine->ReadRegister(4);
    SpaceId pid = process_table->GetPID(currentThread);
    std::vector<std::string> args = user_program_args[pid];
    machine->WriteMem(n_address, 4, args.size());
}

void
ExceptionHandler(ExceptionType which) {
    int type = machine->ReadRegister(2);

    if (which == SyscallException) {
        switch (type) {
            case SC_Halt:
                sc_halt();
                break;
            case SC_Create:
                sc_create();
                break;
            case SC_Read:
                sc_read();
                break;
            case SC_Write:
                sc_write();
                break;
            case SC_Open:
                sc_open();
                break;
            case SC_Close:
                sc_close();
                break;
            case SC_Exit:
                sc_exit();
                break;
            case SC_Join:
                sc_join();
                break;
            case SC_Exec:
                sc_exec();
                break;
            case SC_GetArgN:
                sc_get_arg_n();
                break;
            case SC_GetNArgs:
                sc_get_n_args();
                break;
            default:
                printf("Unexpected user mode exception %d %d\n", which, type);
                ASSERT(false);
                break;
        }
        IncrementProgramCounter();
    } else {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(false);
    }
}
