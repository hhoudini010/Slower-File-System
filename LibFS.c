#include "LibFS.h"
#include "LibDisk.h"

// global errno value here
int osErrno;

int Dir_Create(char *) ;

//TODO - Check error
void
zero_init(){
    char buff[SECTOR_SIZE];
    for(int j = 0; j < SECTOR_SIZE; j++){
        buff[j] = '\0';
    }
    for (int i = 0; i < 10000; i++){
        Disk_Write(i, buff);
    }
}

int
find_free_space(int return_array[2]){

    char buff[SECTOR_SIZE];
    int i, table_sector, offset, actual_offset;

    Disk_Read(4, buff);
    for(i = 0; i < 64; i += 8){
        if(buff[i] == 0){
            table_sector = 4;
            offset = i;
            actual_offset = i;
            return_array[0] = table_sector;
            return_array[1] = offset;
            return_array[2] = actual_offset;
            return 0;
        }
    }

    Disk_Read(5, buff);
    for(i = 0; i < 64; i += 8){
        if(buff[i] == 0){
            table_sector = 5;
            offset = i;
            actual_offset = 64 + i;
            return_array[0] = table_sector;
            return_array[1] = offset;
            return_array[2] = actual_offset;
            return 0;
        }
    }

    Disk_Read(6, buff);
    for(i = 0; i < 64; i += 8){
        if(buff[i] == 0){
            table_sector = 6;
            offset = i;
            actual_offset = 128 + i;
            return_array[0] = table_sector;
            return_array[1] = offset;
            return_array[2] = actual_offset;
            return 0;
        }
    }

    Disk_Read(7, buff);
    for(i = 0; i < 64; i += 8){
        if(buff[i] == 0){
            table_sector = 7;
            offset = i;
            actual_offset = 192 + i;
            return_array[0] = table_sector;
            return_array[1] = offset;
            return_array[2] = actual_offset;
            return 0;
        }
    }
    printf("Find Free Space: Can't Open Any More Files\n");
    return -1;
}

int
make_open_file_table(int sector_number, int fragment){

    char buff[SECTOR_SIZE];
    int info_array[3] = {0, 0, 0};
    int i, table_sector, offset, actual_offset;
    int status = find_free_space(info_array);

    table_sector = info_array[0];
    offset = info_array[1];
    actual_offset = info_array[2];
    Disk_Read(table_sector, buff);

    buff[offset] = 1;
    for(i = 4; i >= 1; i--){
        buff[offset + i] = sector_number % 10;
        sector_number /= 10;
    }

    buff[offset + 5] = fragment;
    buff[offset + 6] = 0;
    buff[offset + 7] = 0;

    Disk_Write(table_sector, buff);
    FS_Sync();

    return (actual_offset/8);
}

int
get_name(char *path,char *fname)
{
    int i,j ;

    for( i = strlen(path)-1; i>=0; i--)
    {
        if(path[i] == '/')
            break ;
    }

    for(i+=1,j=0; i < strlen(path); i++)
    {
        fname[j++] = path[i] ;
        if(j == 16)
            return 0;
    }
    if(j==0)
        return 0;
    fname[j]='\0' ;

    return 1;

}

int
checkvalid(char *fname)
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
    //Todo - change this 
    //size of the file/directory.
    if(mode)
        buff[offset] = 0;
    else
        buff[offset] = 5;

    for(i = 0; i < strlen(name); i++){
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
    return buff[1]-1;
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
    char buff[SECTOR_SIZE];
    int read_status = Disk_Read(1, buff);
    if(read_status == -1){
        printf("Init Bitmap: Error");
        return -1;
    }

    buff[0] = 127;
    buff[1] = 1;
    Disk_Write(1, buff);

    FS_Sync();
    return 0;
}

//TODO : Error Handling, Remove cout
//search for empty sector

int
find_sector(int min_sector){

    int empty_sector = 0,flag;
    flag = 0;
    min_sector++;
    for (int i = 1; i <= 3; ++i)
    {
        char buf[SECTOR_SIZE] ;
        Disk_Read(i,buf) ;

        for(int j = 0 ; j < 512; j++)
        {
            int bit_sector = (int)buf[j] ;
            for(int k = 0 ; k < 7 ; k++)
            {
                if(bit_sector % 2 == 0 && empty_sector>=min_sector)
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
       Dir_Create("/");
       Dir_Create("/a");
       Dir_Create("/a/b");
       Dir_Create("/a/b/c");
       File_Create("/a/b/c/abc.txt");
       File_Create("/a/b/c/abcd.txt");
       int x = File_Open("/a/b/c/abc.txt");
       File_Open("/a/b/c/abcd.txt");
       File_Close(x);

       //Todo - Delete this
       char buf[SECTOR_SIZE];
        Disk_Read(4, buf);
        for(int i = 0; i<512; i++){
            printf("%d ", buf[i]);
        }
        printf("\n\n");
        Disk_Read(7, buf);
        for(int i = 0; i<512; i++){
            printf("%d ", buf[i]);
        }
        printf("\n\n");
        Disk_Read(8, buf);
        for(int i = 0; i<512; i++){
            printf("%d ", buf[i]);
        }
        printf("\n\n");
        Disk_Read(9, buf);
        for(int i = 0; i<512; i++){
            printf("%d ", buf[i]);
        }
        printf("\n\n");
        Disk_Read(10, buf);
        for(int i = 0; i<512; i++){
            printf("%d ", buf[i]);
        }
        printf("\n\n");
        Disk_Read(11, buf);
        for(int i = 0; i<512; i++){
            printf("%d ", buf[i]);
        }
        printf("\n\n");
        Disk_Read(12, buf);
        for(int i = 0; i<512; i++){
            printf("%d ", buf[i]);
        }
        printf("\n\n");

       int siz = Dir_Size("/") ;

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
        if(strcmp(buf, MAGIC_NUMBER) == 0)
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
File_Create(char *path)
{
    printf("FS_Create\n");
    /*    create a new inode
        Mode bit 0 - file
        Mode bit 1 - directory */
    int sector_number = find_sector(0);
    char file_name[16];
    int inode = root_inode, fragment = root_fragment;
    int status = open_dir(path, &inode, &fragment, file_name);
    get_name(path,file_name);
    if(status == -1){
        printf("File Create: Something Wrong in Path\n");
    }
    //check if file already exists.
    char buf[SECTOR_SIZE], newbuf[SECTOR_SIZE];
    Disk_Read(inode, buf) ;

    int tab ;
    int count;
    int offset = (141 * (fragment)) + 2  ;

    int start = offset + 17 ;
    int size = (int)buf[offset] ;

    for(int i = start, count = 0 ; count < size ; i += 4){
        tab = 0 ;
        for(int j = 0 ; j < 4 ; j++){
            tab = tab * 10 + buf[i+j] ;
        }

        Disk_Read(tab, newbuf) ;

        int st = checkintable(newbuf, &inode, &fragment, file_name) ;
        if(st){
            printf("File Create: File Already Exists.\n");
            osErrno = E_CREATE;
            return -1;
        }
        else
            count++ ;
    }
    if(count >= size){
        printf("File Create: Success, No Such File/Directory Found\n");
    }
    int frag = make_inode(sector_number, 0, file_name);

    create_dir(path, root_inode, root_fragment, sector_number, frag);
    return 0;
}

int
File_Open(char *path)
{
    int arr[3];
    if(find_free_space(arr) == -1){
        printf("File Open: Too Many Open Files.\n");
        osErrno = E_TOO_MANY_OPEN_FILES;
        return -1;
    }
    char new_path[256];
    strcpy(new_path, path);
    strcat(new_path, "/dummy");

    char fname[16];
    int inode = root_inode, fragment = root_fragment;

    int status = open_dir(new_path, &inode, &fragment, fname);

    if(status == -1){
        printf("File Open: No such File Exist\n");
        osErrno = E_NO_SUCH_FILE;
        return -1;
    }

    int fd = make_open_file_table(inode, fragment);
    return fd;
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

    int i, table_sector, offset;
    char buff[SECTOR_SIZE];

    if(fd < 64){
        table_sector = 4;
        offset = (fd*8);
    }
    else if(fd > 63 && fd < 128){
        table_sector = 5;
        offset = (fd*8) - 64;
    }
    else if(fd > 127 && fd < 192){
        table_sector = 6;
        offset = (fd*8) - 128;
    }
    else if(fd > 191 && fd < 256){
        table_sector = 7;
        offset = (fd*8) - 192;
    }

    Disk_Read(table_sector, buff);
    if(buff[offset] == 0){
        printf("File Close: File Not Open\n");
        osErrno = E_BAD_FD;
        return -1;
    }

    for(i = offset; i < 8; i++){
        buff[i] = '\0';
    }
    Disk_Write(table_sector, buff);

    return 0;
}

int
File_Unlink(char *file)
{
    printf("FS_Unlink\n");
    return 0;
}

int check_name_exists(int sec, int frag, char *file_name)
{
    printf("check_name_exists\n");
    char buf[SECTOR_SIZE] ;
    char fname[16] ;
    int j = 0 ;

    Disk_Read(sec,buf) ;

    if(buf[0] =='d')
        return 0 ;
    int offset = (141*frag) + 2 ;

    for(int i = offset + 2;buf[i] != '\0' || i<offset + 17; i++)
        fname[j++] = buf[i] ;
    fname[j] = '\0' ;
    if(strcmp(fname,file_name) == 0)
        return 1 ;
    printf("0\n");
    return 0 ;

}

int checkintable(char *tab, int *dir_inode, int *dir_fragment, char *fname)
{
    printf("checkintable\n");
    char temp[5] ;
    int frag, sec ;
    int k = 0, j ;

    int start = 2 ;

    for(int i = 0 ; i < tab[1] ; i++)
    {
        k = 0;
        sec = 0;
        for(j = start ; j < start + 4; j++)
            sec = sec * 10 + tab[j] ;
        frag = tab[j];
        start+=5 ;
        if(check_name_exists(sec,frag,fname)) 
        {
            *dir_inode = sec ;
            *dir_fragment = frag ;
            return 1 ;
        }
    }
    return 0 ;
}


int
open_dir(char *path, int *dir_inode, int *dir_fragment, char* fname)
{
    char new_path[256], file_name[16];
    int i, j = 0, flag, offset, file_inode, file_fragment;
    flag = 0;
    for(i = 1; path[i]!='\0'; i++)
    {
        if(path[i] == '/')
        {
            file_name[i-1] = '\0';
            flag = 1;
        }
        if(flag)
            new_path[j++]=path[i];
        if(!flag)
        {
            file_name[i-1] = path[i];
        }
    }
    new_path[j] = '\0';
    if(!flag)
    {
        file_name[i] = '\0';
        strcpy(fname, file_name);
        return 0;
    }
    else
    {
        char buf[SECTOR_SIZE], newbuf[SECTOR_SIZE] ;
        Disk_Read(*dir_inode, buf) ;

        int tab ;
        int count;
        int offset = (141 * (*dir_fragment)) + 2  ;

        int start = offset + 17 ;
        int size = (int)buf[offset] ;

        for(int i = start, count = 0 ; count < size ; i += 4)
        {
            tab = 0 ;
            for(int j = 0 ; j < 4 ; j++)
           {
               tab = tab * 10 + buf[i+j] ;
           }

            Disk_Read(tab, newbuf) ;
            int st = checkintable(newbuf, dir_inode, dir_fragment,file_name) ;
            if(st)
            {
                open_dir(new_path, dir_inode, dir_fragment, fname) ;
                return 0;
            }
            else
                count++ ;
        }
        if(count >= size){
            printf("Open Directory: No Such File/Directory Found\n");
            return -1;
        }
    }
    return 0;
}


//store location of inode in file table of parent
void
create_dir(char *path, int dir_inode, int dir_sector, int file_inode,int file_fragment)
{
    printf("Change_dir\n");
    char file_name[16];
    int i,flag,offset;
    flag = 0;
    open_dir(path,&dir_inode,&dir_sector,file_name);
    if(!flag)
    {
        //file_name to be created
        int empty_sector = 0,sector_select;
        char buff[SECTOR_SIZE],buffer[SECTOR_SIZE];
        Disk_Read(dir_inode, buff);
        offset = (141 * dir_sector) + 2;
        if(buff[offset] == 0)
        {
            printf("Empty directory\n");

            do
            {
                empty_sector = find_sector(empty_sector);
                printf("Sector = %d\n",empty_sector);
                Disk_Read(empty_sector, buffer);

            }while(buffer[0] == 'i');
            sector_select = empty_sector;

            buffer[0] = 'd';
            buffer[1] = 1;
            for(i=3;i>=0;i--)
            {
                buffer[i+2] = file_inode%10;
                file_inode = file_inode/10;
            }
            buffer[6] = file_fragment;

            buff[offset]++;

            for(i=3;i>=0;i--)
            {
                buff[offset+i+17] = sector_select%10;
                sector_select /= 10;
            }

            Disk_Write(empty_sector,buffer);
            Disk_Write(dir_inode,buff);
            FS_Sync();


            int bitmap_sector = (empty_sector/(SECTOR_SIZE * 7)) + 1;
            int sector_index = (empty_sector / 7);
            int sector_offset = (empty_sector % 7);
            char bitmap_buffer[SECTOR_SIZE];
            Disk_Read(bitmap_sector, bitmap_buffer);
            int val = bitmap_buffer[sector_index];
            int new_val = val + pow(2, sector_offset);
            bitmap_buffer[sector_index] = new_val;
            Disk_Write(bitmap_sector, bitmap_buffer);
            FS_Sync();
        }
        else
        {
            printf("Non-Empty directory\n");

            sector_select = buff[offset] ;
            int internal_offset,sector_num=0;

            internal_offset = offset + 17 + ((sector_select-1)*4);

            for(i=0;i<4;i++)
            {
                sector_num =sector_num*10 + buff[internal_offset+i];
            }
            Disk_Read(sector_num, buffer);
            if(buffer[1]<100)
            {
                internal_offset = (5*buffer[1]);
                for(i=3;i>=0;i--)
                {
                    buffer[internal_offset + i + 2] = file_inode%10;
                    file_inode = file_inode/10;
                }
                buffer[internal_offset + 6] = file_fragment;

                buffer[1]++;
                
                Disk_Write(sector_num,buffer);
                Disk_Write(dir_inode,buff);
                FS_Sync();
            }
            else
            {
                do
                {
                    empty_sector = find_sector(empty_sector);
                    printf("Sector = %d\n",empty_sector);
                    Disk_Read(empty_sector, buffer);

                }while(buffer[0] == 'i');
                sector_select = empty_sector;

                buffer[0] = 'd';
                buffer[1] = 1;
                for(i=3;i>=0;i--)
                {
                    buffer[i+2] = file_inode%10;
                    file_inode = file_inode/10;
                }
                buffer[6] = file_fragment;

                internal_offset = 17 + (4*buff[offset]);
                for(i=3;i>=0;i--)
                {
                    buff[offset+i+internal_offset] = sector_select%10;
                    sector_select /= 10;
                }

                buff[offset]++;
                Disk_Write(empty_sector,buffer);
                Disk_Write(dir_inode,buff);
                FS_Sync();

            }

        }

    }
    else
    {
        // open the file

    }
}



// directory ops
int
Dir_Create(char *path)
{
    printf("Dir_Create %s\n", path);

    char buf[SECTOR_SIZE] ;
    char fname[16];
    int empty_sector, file_sector, file_fragment;

    empty_sector = find_sector(0);

    //Incase of root directory
    if(strcmp(path,"/") == 0)
    {
        root_inode = empty_sector;
        root_fragment = make_inode(empty_sector, 1, path);
        return 0;
    }

    if(get_name(path, fname) == 0)
    {
        printf("Directory name\n");
        return 0;
    }
    printf("File Name = %s\n",fname );

    if(!checkvalid(fname))
    {
        printf("Invalid file name\n");
        return 0;
    }

    file_fragment = make_inode(empty_sector, 1, fname);

    create_dir(path, root_inode, root_fragment, empty_sector, file_fragment);

    Disk_Read(8, buf);
    int offset = (141 * file_fragment) + 2;
    for(int i = 0; i<512; i++){
        printf("%d ", buf[i]);
    }
    printf("\n");

    return 0;

}

int get_Size(int inode_sector,int inode_fragment)
 {
    int count_size=0;

    char buf[SECTOR_SIZE], newbuf[SECTOR_SIZE] ;
         Disk_Read(inode_sector,buf) ;

    int offset = (141 * (inode_fragment)) + 2  ;

    int size=(int)buf[offset];

    char type=buf[offset+1];

    if(type=='f')
        return (size)*512 ;

    if(size==0)
        return 0;

    int start = offset + 17 ;
    if(type=='d')
    {

          for(int i = start,count = 0 ;count < size ;i+=4)
         {

            int tab = 0 ;
            for(int j = 0 ; j < 4 ; j++)
           {
               tab = tab * 10 + buf[i+j] ;
            }
           
           // printf("Hey val %d\n", tab);
            Disk_Read(tab,newbuf) ;

            //getfromtable(newbuf,&inode_sector,&inode_fragment) ;




        int frag1,sec1 ;
        int k1 = 0,j1 ;

        int start1 = 2 ;

        for(int i1 = 0 ; i1 < newbuf[1] ; i1++)
        {
            k1 = 0;
            sec1 = 0;
            for(j1 = start1 ; j1 < start1 + 4; j1++)
                sec1 = sec1 * 10 + newbuf[j1] ;
            frag1 = newbuf[j1];
            start1+=5 ;
             count_size+= get_Size(sec1,frag1) ;

        }
           
            count++ ;
       }
    }

    return count_size ;
 }   

int Dir_Size(char *path) 
{
    char new_path[280];
   printf("Dir_Size %s\n", path); 

   char file_name[16];

   int dir_inode=root_inode;

   int dir_sector=root_fragment;

   strcpy(new_path,path);
   strcat(new_path,"/dummy");
   printf("%s\n",path);

   open_dir(new_path,&dir_inode,&dir_sector,file_name);

   printf("dir inode = %d,%d\n",dir_inode,dir_sector);


   int siz = get_Size(dir_inode,dir_sector) ;

   return siz ;

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
