CC = g++
CFLAGS = -Wall -std=c++1z
DEPS = LibDisk.h LibFS.h
OBJ = LibDisk.o LibFS.o main.o
%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

slfs: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(OBJ)

