CC = gcc
CFLAGS = -Wall
DEPS = LibDisk.h LibFS.h
OBJ = LibDisk.o LibFS.o main.o
%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

dummyProject: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lm

clean:
	rm -f $(OBJ) disk_file

