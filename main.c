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
    char buffer[100];
    Dir_Create("/a");
    Dir_Create("/b");

    File_Create("/abc.txt");
    int x = File_Open("/abc.txt");
    File_Write(x, "Lorem Ipsum is simply dummy text of the printing and typesetting industry. Lorem Ipsum has been the industry's standard dummy text ever since the 1500s, when an unknown printer took a galley of type and scrambled it to make a type specimen book. It has survived not only five centuries, but also the leap into electronic typesetting, remaining essentially unchanged. It was popularised in the 1960s with the release of Letraset sheets containing Lorem Ipsum passages, and more recently with desktop publishing software like Aldus PageMaker including versions of Lorem Ip", 570);
    File_Write(x, "abcdefghijklmnopqrst", 20);
    File_Seek(x,6);
    int y = File_Read(x, buffer, 590);
    print_bitmaps();
    printf("\n%d\n", y);


    return 0;
}
