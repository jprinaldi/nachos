#include "syscall.h"

int main()
{
    OpenFileId fd;
    char* filename = "testfile";
    char ch[1];
    Create(filename);
    fd = Open(filename);
    Write("the quick brown fox jumps over the lazy dog", 43, fd);
    Close(fd);
    fd = Open(filename);
    while (Read(ch, 1, fd)) {
        Write(ch, 1, ConsoleOutput);
    }
    Write("\n", 1, ConsoleOutput);
    Close(fd);
    Exit(0);
}
