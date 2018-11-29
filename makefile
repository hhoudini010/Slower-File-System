CC = gcc
DEPS = LibDisk.h LibFS.h
OBJ = LibDisk.o LibFS.o main.o
%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

slfs: $(OBJ)
	$(CC) -o $@ $^ -lm

clean:
	rm -f $(OBJ) disk_file

