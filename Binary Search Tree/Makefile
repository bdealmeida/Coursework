all: BSTree_main.o BSTree.o
	c++ -o BSTree.out BSTree_main.o BSTree.o

BSTree_main.o: BSTree_main.cpp BSTree.h
	c++ -std=c++11 -c BSTree_main.cpp -o BSTree_main.o

BSTree.o: BSTree.cpp BSTree.h
	c++ -std=c++11 -c BSTree.cpp -o BSTree.o

clean:
	rm *.o
