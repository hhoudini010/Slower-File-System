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
       Dir_Create("/e");
       Dir_Create("/f");
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

        char buffer [512];
       int siz = Dir_Read("/",buffer,40) ;

       for(int i = 0; i<512; i++){
            printf("%c ", buffer[i]);
        }
        printf("\n\n");



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
Dir_Read(char *path, char *buffer, int sz)
{
    printf("Dir_Read\n");

    

    int inode_sector=root_inode;
    int inode_fragment=root_fragment;
    char new_path[280],read_buffer[1024],newbuf[1024],file_name[16];
    strcpy(new_path,path);
    strcat(new_path,"/dummy");
    printf("%s\n",path);

    open_dir(new_path,&inode_sector,&inode_fragment,file_name);

    char buf[SECTOR_SIZE];
    Disk_Read(inode_sector,buf) ;

    int offset = (141 * (inode_fragment)) + 2  ;

    int size=(int)buf[offset];

    
    int start = offset + 17 ;


    //Size error handling
    int index=0,last_table,last_sector,total_size;
    char last_buf[512];
    last_sector = 0;
    last_table = start + (4*(size-1));
    for(int j = 0 ; j < 4 ; j++)
    {
       last_sector = last_sector * 10 + buf[last_table+j] ;
    }
    Disk_Read(last_sector,last_buf) ;

    total_size = ((size-1)*100*21) + (21 * last_buf[1]);
    if(total_size>sz)
    {
        printf("Buffer too small\n");
        osErrno = E_BUFFER_TOO_SMALL;
        return -1;
    }

    
    for(int i = start,count = 0 ;count < size ;i+=4,count++)
    {

        int z,inode_sect=0, inode_frag;
        int sector = 0 ,fragment;
        for(int j = 0 ; j < 4 ; j++)
        {
           sector = sector * 10 + buf[i+j] ;
        }
        Disk_Read(sector,newbuf) ;

        int m = 2;
        for(int l = 0; l<newbuf[1];l++)
        {
                
            for (z=0;z<4;z++)
            {
                inode_sect = inode_sect*10+newbuf[m++];
            }
            inode_frag = newbuf[m++];
            
            Disk_Read(inode_sect,read_buffer);

            int off = (141 * (inode_frag)) + 2  ;

            char name[16];

            for(int k=2;k<=17;k++)
            {
                buffer[index++]=read_buffer[off+k];
            }

            for(int k=3 ;k>=0;k--)
            {
                buffer[index+k]=inode_sect%10 + '0';
                inode_sect = inode_sect/10;
            }
            index = index+4;
            buffer[index++]=inode_frag +'0';
        }

        for(index;index<sz;index++)
        {
            buffer[index]= '\0';
        }
    }    
    
    return total_size;
}

int find_and_delete_last(int *frag, int *sec, int n_tables, char *pinode_content, int offset,char *myinode, int sec_no_wr, int myinode_secno)
{
    int remem_ret_val = 0 ;


    printf("find_and_delete_last\n");
    printf(" n_tables = %d\n",n_tables );
    int start = offset + 17 ;
    char buf[SECTOR_SIZE] ;

    start += 4*(n_tables-1) ;

    printf("start = %d\n",start);

    int tab_num = 0 ;
    for(int i = start ; i < start + 4 ; i++)
        tab_num = tab_num * 10 + myinode[i];

    printf("tab_num = %d\n", tab_num);
    //Gets the last table.
    Disk_Read(tab_num,buf) ;

    int nu_items_table = buf[1] ;
    if(nu_items_table == 1)
        remem_ret_val = 1 ;


    //Item to be deleted is present in last table and last table has only one entry.
    if(sec_no_wr == tab_num && nu_items_table == 1)
    {
        //printf("I'm in this\n");
        for(int i = 0 ; i < 7; i++)
            buf[i] = '\0' ;

        myinode[offset] = (int)myinode[offset] - 1 ;
        for(int i = start; i < start + 4; i++)
            myinode[i] = '\0' ;

        Disk_Write(tab_num,buf) ;
        for(int i = 0 ; i < 4; i++)
            myinode[start + i] = '\0' ;

        Disk_Write(myinode_secno,myinode) ;

        FS_Sync() ;

        return 2 ;
    }

    nu_items_table-- ;

    int start_read = 2 + (nu_items_table * 5) ;

    printf("Start_read : %d\n",start_read );


    int del_sector = 0;
    int del_fragment ;
    int i,j ;

    for(i=0,j=start_read;i<4;i++,j++)
    {   
        del_sector = del_sector * 10 + buf[j];
    }

    del_fragment = (int)buf[j] ;

    //Clear table if there was only one entry.

    if(nu_items_table == 0)
    {
        *frag = del_fragment ;
        *sec = del_sector ;
        for(int j = 0 ; j < 7; j++)
            buf[j] = '\0' ;
        myinode[offset] = (int)myinode[offset] - 1 ;
        for(int i = start; i < start + 4; i++)
            myinode[i] = '\0' ;

        Disk_Write(tab_num,buf) ;
        Disk_Write(myinode_secno,myinode) ;
        FS_Sync() ;
        return 1 ;
    }


    //If there are multiple entries

        for(int j = start_read; j < start_read + 5; j++)
            buf[j] = '\0' ;
        buf[1] = (int)buf[1] - 1 ;


      Disk_Write(tab_num,buf) ;
     FS_Sync() ;

    //  //....................................//
    //  printf("Printer code..............................................\n");
    //  char bufs[512];
    //     Disk_Read(134,bufs);
    //     for(int i = 0; i<512; i++){
    //         printf("%d ",bufs[i]);
    //     }
    //     printf("\n");

    // printf("End ............................................................\n");


    *frag = del_fragment ;
    *sec = del_sector ;

    if(remem_ret_val)
        return 1 ;
    else
        return 0 ;

}

int check_in_table_delete(char *tab, int cinode, int cfragment, int n_tables, char *pinode_content, int offset, char *myinode, int sec_no_wr, int myinode_secno )
{
    printf("check_in_table_delete\n");
    char temp[5] ;
    int frag,sec ;
    int k = 0,j ;

    int start = 2 ;

    for(int i = 0 ; i < tab[1] ; i++)
    {
        k = 3;
        sec = 0;
        for(j = start ; j < start + 4; j++)
            sec = sec * 10 + tab[j] ;
        frag = tab[j];
        start+=5 ;

        if(frag == cfragment && sec == cinode)
        {
            // for(j = start; j < start + 4; j++)
            //     tab[j] = '\0' ;
            // tab[start+4] = '\0' ;

            int ret_val = find_and_delete_last(&frag, &sec, n_tables, pinode_content, offset,myinode,sec_no_wr, myinode_secno) ;
            printf("frag =%d , sect = %d\n",frag,sec );

            if(ret_val == 2)
                return 3 ;


            Disk_Read(sec_no_wr,tab) ;

            start-=5 ;

            int temp_arr[4] ;
            for(int j =0 ; j < 4; j++)
            {
                temp_arr[j] = sec%10 ;
                sec/=10 ;
            }

            for(j = start ; j < start + 4; j++)
                tab[j] = temp_arr[k--] ;

            tab[j] = frag ;

            Disk_Write(sec_no_wr,tab) ;
            FS_Sync() ;

            if(ret_val)
                return 2 ;

            return 1 ;
        }
    }

    return 0 ;
}

int search_in_pointer(int dir_inode,int dir_fragment, int cinode, int cfragment)
{
        printf("search_in_pointer\n");
        char buf[SECTOR_SIZE], newbuf[SECTOR_SIZE] ;
        Disk_Read(dir_inode,buf) ;

        int tab, remember_pointer_nos = 0, remember_table_size = 0 ;

        int offset = (141 * dir_fragment) + 2  ;

         int start = offset + 17 ;
         int size = (int)buf[offset] ;

         if(size == 1)
            remember_pointer_nos = 1 ;

        for(int i = start,count = 0 ;count < size ;i+=4)
        {

            tab = 0 ;
            for(int j = 0 ; j < 4 ; j++)
           {
               tab = tab * 10 + buf[i+j] ;
            }
           
           // printf("Hey val %d\n", tab);
            Disk_Read(tab,newbuf) ;

            if(newbuf[1] == 1)
                remember_table_size = 1 ;

            //Delete the whole table
            if(remember_table_size == 1 && remember_pointer_nos == 1)
            {
                for(int j = 0; j < 7; j++)
                    newbuf[j] = '\0' ;

                Disk_Write(tab,newbuf) ;

                for(int j = 0 ; j < 4; j++)
                    buf[i+j] = '\0' ;

                buf[offset] = '\0' ;
                Disk_Write(dir_inode,buf) ;

                FS_Sync() ;

                 printf("Delete Successful.\n");
                 return 1 ;

            }



            int st = check_in_table_delete(newbuf, cinode, cfragment, size, buf, offset,buf, tab, dir_inode) ;
            if(st)
            {
                if(st == 3)
                {
                    printf("Delete Successful\n");
                    return 1 ;
                }

               printf("Delete Successful.\n");
               return 1 ;
                
            }
            else
                count++ ;
       }

       return 0 ;
}

int
Dir_Unlink(char *path)
{
    printf("Dir_Unlink\n");

    if(strcmp(path,"/") == 0)
    {
        osErrno = E_ROOT_DIR ;
        return -1 ;
    }

    int pinode = root_inode ;
    int pfragment = root_fragment ;

    int cinode = root_inode ;
    int cfragment = root_fragment ;

    char file_name[16] ;
    open_dir(path,&pinode,&pfragment,file_name) ;

    //char file_name[16];
    char new_path[280] ;
    strcpy(new_path,path) ;
    strcat(new_path,"/dummy") ;


    open_dir(new_path,&cinode,&cfragment,file_name) ;

    char buf[SECTOR_SIZE] ;
    Disk_Read(cinode,buf) ;

    int offset = (141*cfragment) + 2 ;
    int size = buf[offset] ;

    printf("size = %d\n",size);

    if(size)
    {
        osErrno = E_DIR_NOT_EMPTY ;
        return -1 ;
    }

    //Delete the inodes.
    char bufs[SECTOR_SIZE] ;
    Disk_Read(cinode,bufs) ;

    int offsets = (141 * cfragment) + 2  ;
    for(int i = offsets; i < offsets + 141; i++)
        bufs[i] = '\0' ;
    bufs[1] = (int)bufs[1] - 1 ;
    if(bufs[1] == '\0')
        bufs[0] = '\0' ;
    Disk_Write(cinode,bufs) ;
    FS_Sync() ;

    int st = search_in_pointer(pinode,pfragment,cinode,cfragment) ;

    if(st)
        printf("Yes\n");
    else
        printf("No\n");


    return 0;
}
