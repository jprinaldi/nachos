#include "syscall.h"

int main()
{
    int i, argc;
    int arg_len;
    GetNArgs(&argc);
    char* argv[argc];
    char ch[1];

    for (i = 0; i < argc; i++) {
        GetArgN(i, argv[i], &arg_len);
    }
    
    if (argc > 1) {
        OpenFileId file_descriptor = Open(argv[1]);
        if (file_descriptor != -1) {
            while (Read(ch, 1, file_descriptor)) {
                Write(ch, 1, ConsoleOutput);
            }     
            Close(file_descriptor);
        }
    }
    
    Exit(0);
}
