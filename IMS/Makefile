CC        = g++
OBJ_FILES = main.o cell.o grid.o image.o
CFLAGS	  = -Wall -Wextra -pedantic -std=c++11
OPENCV_FLAGS = -lopencv_highgui -lopencv_core -lopencv_imgproc -lopencv_imgcodecs
all: simulator

simulator:	$(OBJ_FILES)
		$(CC) $(OBJ_FILES) -o $@ $(CFLAGS) $(OPENCV_FLAGS)

run: exp1 exp2 exp3 exp4

exp1:
	./simulator
exp2:
	./simulator 2
exp3:
	./simulator 3
exp4:
	./simulator 4

main.o: main.cpp cell.hpp grid.hpp image.hpp
	$(CC) -o $@ -c $<  $(CFLAGS) $(OPENCV_FLAGS)

clean:
	rm -f *.o simulator 

.PHONY: clean

