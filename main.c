#include <stdio.h>

#include "LibFS.h"

void 
usage(char *prog)
{
    fprintf(stderr, "usage: %s <disk image file>\n", prog);
    exit(1);
}

int
main(int argc, char *argv[])
{
    if (argc != 2) {
	usage(argv[0]);
    }
    char *path = argv[1];

    
    FS_Boot(path);
    FS_Sync();

   // Dir_Size("/ab/bc");

    printf("Fun from main\n");

    // File_Create("/abcd.txt") ;
    //printf("File Open = %d\n",File_Open("/abcd.txt")) ;

    int st = File_Unlink("/abcd.txt") ;
   
    printf("%d\n", st);

    return 0;
}
