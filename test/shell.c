#include "syscall.h"

int
main()
{
    SpaceId newProc;
    OpenFileId input = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char prompt[2], ch, buffer[60], filename[60];
    int i, j, k, argc;

    prompt[0] = '-';
    prompt[1] = '-';

    while( 1 )
    {
	Write(prompt, 2, output);

	i = 0;
	j = 0;
	argc = 1; // executable filename is always an argument
	
	do {
	
	    Read(&buffer[i], 1, input);
	    if (buffer[i] == ' ') {
	        argc++;
	    }
	    if (argc == 1 && buffer[i] != '\n') {
	        filename[j++] = buffer[i];
	    }

	} while( buffer[i++] != '\n' );

	buffer[--i] = '\0';
	filename[j] = '\0';

	if( i > 0 ) {
	    if (filename[0] == '&') {
	        Exec(filename + 1, buffer + 1);
	    } else {
		    newProc = Exec(filename, buffer);
		    Join(newProc);
	    }
	}
    }
}

