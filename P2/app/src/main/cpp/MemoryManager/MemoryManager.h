#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <stdint.h>
#include <vector>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>

#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

struct chunk{
public:
	int pid;
	int address;
	unsigned int size;
	chunk();
	chunk(int pid, int address, unsigned int size);
};

class MemoryManager{

private:
	unsigned wordSize;
	std::function<int(int, void *)> allocator;
	char *memBlock;
	int memLimit;
	int size;
	std::vector<chunk> mem;
	int global_pid;
	std::vector<uint16_t> holes;
	uint16_t bitmap[65536];
	uint16_t* listCopy;

public:

	MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator);
	~MemoryManager();
	void printMem();
	void initialize(size_t sizeInWords);
	void shutdown();
	void *allocate(size_t sizeInBytes);
	void fixHoles(int i);
	void free(void *address);
	void setAllocator(std::function<int(int, void *)> allocator);
	int dumpMemoryMap(char *filename);
	void *getList(); 
	void *getBitmap();
	unsigned getWordSize();
	void *getMemoryStart();
	unsigned getMemoryLimit();
	int freeMem();
	int usedMem();

};


int bestFit(int sizeInWords, void *list);
int worstFit(int sizeInWords, void *list);

#endif