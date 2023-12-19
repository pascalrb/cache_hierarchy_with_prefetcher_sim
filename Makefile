CC = g++
#OPT = -O3
OPT = -g
WARN = -Wall
CFLAGS = $(OPT) $(WARN) $(INC) $(LIB)

# List all your .cc/.cpp files here (source files, excluding header files)
CACHESIM_SRC = cachesim.cpp

# List corresponding compiled object files here (.o files)
CACHESIM_OBJ = cachesim.o
 
#################################

# default rule

all: cachesim
	@echo "my work is done here..."


# rule for making cachesim

cachesim: $(CACHESIM_OBJ)
	$(CC) -o sim $(CFLAGS) $(CACHESIM_OBJ) -lm
	@echo "----------- DONE WITH cachesim -----------"


# generic rule for converting any .cpp file to any .o file

.cpp.o:
	$(CC) $(CFLAGS) -c $*.cpp


# type "make clean" to remove all .o files plus the cachesim binary

clean:
	rm -f *.o sim


# type "make clobber" to remove all .o files (leaves cachesim binary)

clobber:
	rm -f *.o


