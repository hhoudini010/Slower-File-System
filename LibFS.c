#include "LibFS.h"
#include "LibDisk.h"

// global errno value here
int osErrno;


int
FS_Sync(char *path)
{
    printf("FS_Sync\n");
    if(Disk_Save(path) == -1){
        printf("Disk save error\n");
        osErrno = E_GENERAL;
        return -1 ;
    }

    return 0;
}


int 
FS_Boot(char *path)
{
    printf("FS_Boot %s\n", path);

    // oops, check for errors
    if (Disk_Init() == -1) {
	printf("Disk_Init() failed\n");
	osErrno = E_GENERAL;
	return -1;
    }

    // do all of the other stuff needed...

    int load_status = Disk_Load(path) ;

    if(load_status == -1 && diskErrno == E_INVALID_PARAM){
        printf("Invalid Parameter.\n");
        osErrno =  E_GENERAL ;
       return -1 ;
    }

    //If disk image does not exist, create a new image
    if(load_status == -1 && diskErrno == E_OPENING_FILE)
    {
        //Creating new disk image.
      int write_status = Disk_Write(0,MAGIC_NUMBER) ;
      if(write_status == -1 && diskErrno == E_MEM_OP)
       {
            printf("Write failed.\n");
            osErrno = E_GENERAL ;
            return -1 ;
       }
       if(FS_Sync(path) == -1) 
            return -1 ;
    }

    //Disk image exists.
    else 
    {
       char buf[512];
       int read_status = Disk_Read(0,buf) ;
       if(read_status == -1 && diskErrno == E_MEM_OP)
       {
            printf("Read failed.\n");
            osErrno = E_GENERAL ;
            return -1 ;
       }
       //Checks if the first sector contains the magic number. If no, the disk is corrupted.
       if(strcmp(buf,MAGIC_NUMBER)==0)
            printf("Success\n");
        else{
            printf("Disk Corrupted\n");
            return -1 ;
        }

    }

    printf("Boot Complete\n");


    return 0;
}

int
File_Create(char *file)
{
    printf("FS_Create\n");
    return 0;
}

int
File_Open(char *file)
{
    printf("FS_Open\n");
    return 0;
}

int
File_Read(int fd, void *buffer, int size)
{
    printf("FS_Read\n");
    return 0;
}

int
File_Write(int fd, void *buffer, int size)
{
    printf("FS_Write\n");
    return 0;
}

int
File_Seek(int fd, int offset)
{
    printf("FS_Seek\n");
    return 0;
}

int
File_Close(int fd)
{
    printf("FS_Close\n");
    return 0;
}

int
File_Unlink(char *file)
{
    printf("FS_Unlink\n");
    return 0;
}


// directory ops
int
Dir_Create(char *path)
{
    printf("Dir_Create %s\n", path);
    return 0;
}

int
Dir_Size(char *path)
{
    printf("Dir_Size\n");
    return 0;
}

int
Dir_Read(char *path, void *buffer, int size)
{
    printf("Dir_Read\n");
    return 0;
}

int
Dir_Unlink(char *path)
{
    printf("Dir_Unlink\n");
    return 0;
}
