#include "syscall.h"

int main()
{
    int i, argc;
    int arg_len;
    GetNArgs(&argc);
    char argv[argc][100];
    char ch[1];

    for (i = 0; i < argc; i++) {
        GetArgN(i, argv[i], &arg_len);
    }
    
    if (argc > 2) {
        OpenFileId source_file = Open(argv[1]);
        if (source_file == -1) {
            // Could not open source file
            Exit(-1);
        }
        OpenFileId target_file = Open(argv[2]);
        if (target_file < 0) {
            Create(argv[2]);
            target_file = Open(argv[2]);
        }
        while (Read(ch, 1, source_file)) {
            Write(ch, 1, target_file);
        }     
        Close(source_file);
        Close(target_file);
    }
    
    Exit(0);
}
