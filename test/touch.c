#include "syscall.h"

int main()
{
    int i, argc;
    int arg_len;
    GetNArgs(&argc);
    char* argv[argc];

    for (i = 0; i < argc; i++) {
        GetArgN(i, argv[i], &arg_len);
    }

    OpenFileId fd;
    Create(argv[1]);
    Exit(0);
}
