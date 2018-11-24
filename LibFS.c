#include "LibFS.h"
#include "LibDisk.h"

// global errno value here
int osErrno;

int Dir_Create(char *) ;

//TODO - Check error
void zero_init(){
    char buff[SECTOR_SIZE];
    for(int j = 0; j < SECTOR_SIZE; j++){
        buff[j] = '\0';
    }
    for (int i = 0; i < 10000; i++){
        Disk_Write(i, buff);
    }
}

void get_name(char *path,char *fname)
{
    int i,j ;

    for( i = strlen(path)-1; i>=0; i--)
    {
        if(path[i] == '/')
            break ;
    }
    // char fname[16] ;

    for(i,j=0; i < strlen(path); i++)
    {
        fname[j++] = path[i] ;
    }

    fname[j]='\0' ;
}

int checkvalid(char *fname)
{
    for(int i = 0 ; i < strlen(fname); ++i)
        if(!((fname[i] >= 'a' && fname[i] <= 'z') || (fname[i] >= '0' && fname[i] <= '9') || fname[i] == '.' || fname[i] == '-' || fname[i] == '_'|| fname[i] == '/'))
            return 0 ;
    return 1 ;
}   


//TODO - error handling
int
make_inode (int sector_number, int mode, char *name){
    char buff[SECTOR_SIZE];
    int offset, number_of_inodes, i;
    Disk_Read(sector_number, buff);

    if (buff[0] != 'i'){
        buff[0] = 'i';
        buff[1] = 1;
        offset = 2;
    }
    else if(buff[0] == 'i'){
        if(buff[1] < 3) {
            number_of_inodes = buff[1];
            offset = (141 * number_of_inodes) + 2;
            buff[1] += 1;
        }
    }

    buff[offset] = 0;
    for(i = 0; i < 16; i++){
        buff[offset + i + 2] = name[i];
    }
    buff[offset + i + 2] = '\0';
    if(mode)
        buff[offset + 1] = 'd';
    else
        buff[offset + 1] = 'f';
    Disk_Write(sector_number, buff);
    FS_Sync();

    if(buff[1] == 3){
        int bitmap_sector = (sector_number/(SECTOR_SIZE * 7)) + 1;
        int sector_index = (sector_number / 7);
        int sector_offset = (sector_number % 7);
        char bitmap_buffer[SECTOR_SIZE];
        Disk_Read(bitmap_sector, bitmap_buffer);
        int val = bitmap_buffer[sector_index];
        int new_val = val + pow(2, sector_offset);
        bitmap_buffer[sector_index] = new_val;
        Disk_Write(bitmap_sector, bitmap_buffer);
        FS_Sync();
    }
    return 0;
}

int
FS_Sync()
{
    printf("FS_Sync\n");
    if(Disk_Save(disk_path) == -1){
        printf("Disk save error\n");
        osErrno = E_GENERAL;
        return -1 ;
    }

    return 0;
}
//TODO - error handling in below function
//creates a bitmap if new disk is initialized with system info.

int
init_bitmaps(){
    char buff[512];
    int read_status = Disk_Read(1, buff);
    if(read_status == -1){
        printf("Init Bitmap: Error");
        return -1;
    }
    for(int i = 0; i < 19; i++){
        buff[i] = 127;
    }

    Disk_Write(1,buff);

    FS_Sync();
    return 0;
}

//TODO : Error Handling, Remove cout
//search for empty sector

int
find_sector(){

    int empty_sector = 0,flag;
    flag = 0;
    for (int i = 1; i <= 3; ++i)
    {
        char buf[SECTOR_SIZE] ;
        Disk_Read(i,buf) ;

        for(int j = 0 ; j < 512; j++)
        {
            int bit_sector = (int)buf[j] ;
            for(int k = 0 ; k < 7 ; k++)
            {
                if(bit_sector % 2 == 0)
                {
                    flag = 1;
                    break;

                }
                bit_sector = bit_sector>>1;
                empty_sector++;
            }
            if(flag)
                break;
        }
        if(flag)
            break;
    }

    printf("empty_sector = %d\n",empty_sector);
    // cout<<"empty_sector = "<<empty_sector<<"\n";
    return empty_sector;

}

int
FS_Boot(char *path)
{
    printf("FS_Boot %s\n", path);


    // disk_path = new char[strlen(path)] ;
    disk_path = (char*)malloc(strlen(path));
    strcpy(disk_path,path);

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
        zero_init();
        //Creating new disk image.
        int write_status = Disk_Write(0,MAGIC_NUMBER) ;
        if(write_status == -1 && diskErrno == E_MEM_OP)
        {
            printf("Write failed.\n");
            osErrno = E_GENERAL ;
            return -1 ;
       }
       init_bitmaps();
       Dir_Create("/") ;

       if(FS_Sync() == -1) 
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
    /*    create a new inode
        Mode bit 0 - file
        Mode bit 1 - directory */
    int sector_number = find_sector();
    make_inode(sector_number, 0, file);
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

    char buf[SECTOR_SIZE] ;
    int empty_sector;
    printf("Dir_Create %s\n", path);

    empty_sector = find_sector();
    char fname[16];
    get_name(path,fname);

    printf("File Name = %s\n",fname );
    int st = checkvalid(fname) ;

    if(st)
        printf("valid\n");
    else
        printf("invalid\n");
    make_inode(empty_sector, 1, fname);
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
