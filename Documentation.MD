# Slower File System  
## `Group Name - We are too young for this!`  
`Memebrs:`  
`Rajat Dua - 2018201066`  
`Indraneel Das - 2018202019`  
`Sandeep Gupta - 2018201076`  
`Anand Sagar Sethi - 2018201100`  

### Objective:  
Building a user level lbrary libFS that implements, a good portion of file system.   
This file system links to an application through which we can access files and directories.  
This library will inturn link with a layer that implements a disk.  

### Implementation Details:
We will make a file system API that has 3 parts. Two of them are generic file system calls that deals with file access  
and a set of calls that deals with directories.  

#### File Access APIs  
 - int File_Create(char *file)  
This creates a new file of the name pointed to by _file_. If the file already exists, you should return -1 and set  
osErrno to E_CREATE.  
 - int File_Open(char *file)  
This opens a file and return an integer file descriptor, which can be used to read and write from and to the file.  
 - int File_Read(int fd, void *buffer, int size)  
This should read _size_ bytes from the file referenced by the file descriptor _fd_.  
The data should be read into the buffer pointed to by _buffer_.  
 - int File_Write(int fd, void *buffer, int size)  
This should write size bytes from buffer and write them into the file referenced by _fd_.  
 - int File_Seek(int fd, int offset)  
This should update the current location of the file pointer.  
 - int File_Close(int fd)  
This closes the file referred to by file descriptor _fd_.  
 - int File_Unlink(char *file)  
This should delete the file referenced by _file_, including removing its name from the directory it is in.  
#### Directory APIs  
 - int Dir_Create(char *path)  
This creates a new directory as named by _path_.  
 - int Dir_Size(char *path)  
This returns the number of bytes in the directory refereed to by _path_.  
 - int Dir_Read(char *path, void *buffer, int size)  
This can be used to read the contents of a directory refered by _path_. 
 - int Dir_Unlink(char *path)  
This removes a directory referred to by _path_, freeing up its inode and data blocks, and removing its entry from the parent directory.  
### Project Plan  
- 3rd - 5th November, 2018      -   Understanding and documentation.
- 6th - 10th November, 2018     -   file_create(), file_open() and file_read().
- 11th - 15th November, 2018    -   file_write(),  file_seek(), file_close() and file_unlink().
- 15th - 20th November, 2018    -   dir_create(), dir_size(), dir_read() and dir_unlink().
- 20th - 25th November, 2018    -   Testing.
- 25th - 28th November, 2018    -   Report. 