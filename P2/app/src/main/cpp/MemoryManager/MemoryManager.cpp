#include "MemoryManager.h"

/*
struct chunk{
	std::string pid;
	int address;
	unsigned int size;
};
chunk(std::string pid, int address, unsigned int size){
*/

chunk::chunk(){
		this->pid = -1;
		this->address = 0;
		this->size = 0;
};
chunk::chunk(int pid, int address, unsigned int size){
		this->pid = pid;
		this->address = address;
		this->size = size;
};

//Constructor; sets native word size (for alignment) and default allocator function for finding a memory hole.
MemoryManager::MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator){
	this->wordSize = wordSize;
	this->allocator = allocator;
	this->memLimit = 0;
	this->size = 0;
	this->global_pid = 0;
	this->listCopy = nullptr;
	
}

//Releases all memory allocated by this object without leaking memory.
MemoryManager::~MemoryManager(){
	if(memBlock != nullptr)
		delete [] memBlock;
	
	if(listCopy != nullptr)
		delete [] listCopy;

}

void MemoryManager::printMem(){
	std::cout << "\t\t=========================================" << std::endl;
	for(auto& m : mem){
		std::cout << "\t\t= pid: " << m.pid << " addr: " << m.address << " size: " << m.size << "\t\t=" << std::endl;
	}
	std::cout << "\t\t=========================================" << std::endl << std::endl;
}

//Instantiates a new block of memory of requested size, no larger than 65536. Hint: you may use new char[...].
void MemoryManager::initialize(size_t sizeInWords){
	
	//error handling
	if(sizeInWords > 65536){
		std::cout << "ERROR: size exceeds 65536\n\n";
	}

	//prime our vector with a hole
	chunk n;
	n.address = 0;
	n.size = sizeInWords; //or just wordSize??
	mem.push_back(n);


	this->memLimit = sizeInWords * wordSize;
	// 1 byte per char, sizeInWords words, wordSize words
	memBlock = new char[sizeInWords * wordSize];
	
	
	return;
}

//Releases memory block acquired during initialization.
void MemoryManager::shutdown(){

	if(memBlock != nullptr)
		delete [] memBlock;
	if(listCopy != nullptr)
		delete [] listCopy;

	this->size = 0;
	this->memLimit = 0;
	mem.clear();
	holes.clear();

	
	return;
}

//Allocates a block of memory. If no memory is available or size is invalid, return nullptr.
void *MemoryManager::allocate(size_t sizeInBytes){
	int sizeInWords = std::ceil((double)sizeInBytes / wordSize);
	if(sizeInBytes + this->size > this->memLimit){
		std::cout << "ERROR: Too large for memory. \n";
		return nullptr;
	}
	int location = this->allocator(sizeInWords, this->getList());
	if(location == -1){
		std::cout << "ERROR: Does not fit in any holes. \n";
		return nullptr;
	}
	//add the chunk's size to memMananger's size
	//add it to the mem vector
	//if necessary/possible, merge w hole to left or right

	this->size += sizeInBytes;

	//we wanna make sure we go to the hole
	int i = 0;
	//this->printMem();
	while(location != mem[i].address){
		i++;
	}
	chunk c;
	c.pid = global_pid;
	global_pid++;
	c.address = location;
	c.size = sizeInWords;
	mem[i].address += sizeInWords;
	mem[i].size -= sizeInWords;
	if(mem[i].size == 0){
		mem.erase(mem.begin() + i);
	}
	mem.insert(mem.begin() + i, c);


	char* addr = (char*)this->getMemoryStart() + location*wordSize;
	return static_cast<void*>(addr);
}

void MemoryManager::fixHoles(int i){
	//there's an element to the left
	if(i - 1 >= 0){
		if(mem[i - 1].pid == -1){
			mem[i - 1].size += mem[i].size;
			mem[i].size = 0;
			//delete mem[i]
			mem.erase(mem.begin() + i);
			//move left one to check right
			i -= 1;
		}
	}
	if(i + 1 <= mem.size()){
		if(mem[i + 1].pid == -1){
			mem[i].size += mem[i + 1].size;
			mem[i + 1].size = 0;
			//delete mem[i + 1]
			mem.erase(mem.begin() + i + 1);
		}
	}

	return;
}

//Frees the memory block within the memory manager so that it can be reused.
void MemoryManager::free(void *address){
	int location = (int)((char *)address - (char *)this->getMemoryStart())/wordSize;
	int i = -1;
	//find address to be freed
	for(int j = 0; j < mem.size(); j++){
		if(mem[j].address == location){
			i = j;
		}
	}
	//no process found at address
	if(i == -1){
		return;
	}

	mem[i].pid = -1;
	this->fixHoles(i);
	return;
}

//Changes the allocation algorithm to identifying the memory hole to use for allocation.
void MemoryManager::setAllocator(std::function<int(int, void *)> allocator){
	this->allocator = allocator;
	return;
}

//Uses standard POSIX calls to write hole list to filename as text, returning -1 on error and 0 if successful.
int MemoryManager::dumpMemoryMap(char *filename){
	
	//posix calls to make a file and open it
	int newFile = creat(filename, 0777);
	newFile = open(filename, O_RDWR);
	if(newFile == -1){
		return -1;
	}

	uint8_t* listy = (uint8_t*)this->getList();
	int len = ((int)listy[1] << 8) + (int)listy[0];
	len /= 4;
	listy += 2;
	int strLength = 0;
	//std::cout << "length is: " << len;

	for(int i = 0; i < len; i++){
		char* str = new char[100];
		memset(str, 0x00, 100);

		//ith hole is 4 bytes from the last one
		int ithHole = i*4;
		int addy = ((int)listy[ithHole+1] << 8) + (int)listy[ithHole];
		int size = ((int)listy[ithHole+3] << 8) + (int)listy[ithHole+2];

		//if either is 0, only 1 digit, else log_10(member) + 1 digits
		int addyDigits = 1;
		if(addy > 0){
			addyDigits += (int)log10((double) addy);
		}
		int sizeDigits = 1;
		if(size > 0){
			sizeDigits += (int)log10((double) size);
		}
		
		if(i < len - 1){
			//prints to a string instead of to the terminal window
			sprintf(str, "[%i, %i] - ", addy, size);
			//7 chars predefined plus however many in each int
			strLength = 7 + addyDigits + sizeDigits;
		}
		else{
			sprintf(str, "[%i, %i]", addy, size);
			//4 chars predefined plus however many in each int
			strLength = 4 + addyDigits + sizeDigits;
		}
		//posix call to write this hole's string to the file created earlier
		write(newFile, str, strLength);

		delete [] str;
	}

	close(newFile);
	
    return 0;

}

//Returns a byte-stream of information (in binary) about holes for use by the allocator function (little-Endian).
void *MemoryManager::getList(){
	holes.clear();
	
	for(auto& m : mem){
		if(m.pid == -1){
			holes.push_back((uint16_t)m.address);	//convert offset to words
			holes.push_back((uint16_t)m.size);		//convert size to words
		}
	}
	
	uint16_t sz = (uint16_t)holes.size();
	//very inefficient but who cares for now??
	holes.insert(holes.begin(), (uint16_t)(sz*2));	//put size of bitstream at the beginning, 2 bytes per hole member

	/*for(int i = 0; i < holes.size(); i++){
		stream[i] = holes[i];
		
		//std::cout << " element at " << i << " is " << (uint16_t)holes[i] << std::endl;
	}*/
	if(listCopy == nullptr)
		listCopy = new uint16_t[memLimit];

	for(int i = 0; i < holes.size(); i++){
		listCopy[i] = holes[i];
	}
	return static_cast<void*>(listCopy);
	//return static_cast<void*>(holes.data());
}


//Returns a bit-stream of bits representing whether words are used (1) or free (0). The first two bytes are the size of the bitmap (little-Endian); the rest is the bitmap, word-wise.
void *MemoryManager::getBitmap(){
	for(int i = 0; i < 65536; i++){
		bitmap[i] = (uint16_t)1;
	}
	uint16_t* listy = (uint16_t*)getList();
	int len = *(int*)listy;
	len /= 4;
	listy++;
	int addy;
	int sz;
	//printMem();
	for(int i = 0; i < len*2; i++){
		//std::cout << " len is " << len << " and i is " << i << std::endl;
		addy = listy[i];
		i++;
		sz = listy[i];
		//std::cout << "addy is " << addy << " and sz is " << sz << "\n";
		for(int k = addy; k < addy + sz; k++){
			bitmap[k] = (uint16_t)0;
		}
	}
	uint16_t* bitmapCopy = new uint16_t[memLimit];
	bitmapCopy[0] = len + 1;
	int j;
	std::cout << "What bitmap should look like in big Endian but I can't get it to work: \n";
	for(j = 1; j < memLimit/wordSize + 1; j++){
		bitmapCopy[j] = bitmap[j - 1];
		std::cout << bitmapCopy[j];
		if(j % 8 == 0){
			std::cout << std::endl;
		}
	}
	while(j % 8 != 1){
		std::cout << bitmapCopy[j];
		j++;
	}
	std::cout << std::endl;

	return bitmapCopy;
}

//Returns the word size used for alignment.
unsigned MemoryManager::getWordSize(){
	return this->wordSize;
}

//Returns the byte-wise memory address of the beginning of the memory block.
void *MemoryManager::getMemoryStart(){
	char* addr = memBlock;
	return static_cast<void*>(addr);
}

//Returns the byte limit of the current memory block.
unsigned MemoryManager::getMemoryLimit(){
	return this->memLimit;
}

int MemoryManager::freeMem(){
	int ret = 0;
	//adds space in each hole
	for(int i = 0; i < mem.size(); i++){
		if(mem[i].pid == -1){
			ret += mem[i].size;
		}
	}

	return ret;
}
int MemoryManager::usedMem(){
	int ret = 0;
	//adds space in each program
	for(int i = 0; i < mem.size(); i++){
		if(mem[i].pid != -1){
			ret += mem[i].size;
		}
	}

	return ret;
}

//Returns word offset of hole selected by the best fit memory allocation algorithm, and -1 if there is no fit.
int bestFit(int sizeInWords, void *list){
	
	uint16_t bitstreamLength = *(uint16_t*)(list);
	int num = (int)bitstreamLength/4; //2 bytes per hole field * 2 hole fields (start & len)
	
	uint16_t* holes = (uint16_t*)list + 1; 

	int bestFitty = std::numeric_limits<int>::max();	//worst possible fit
	int ret = -1;
	for(int i = 0; i < num; i++){
		//std::cout << " hole is at " << holes[2*i] << " and hole len is " << holes[2*i + 1] << std::endl;
		if(holes[2*i + 1] >= sizeInWords && holes[2*i + 1] < bestFitty){
			bestFitty = holes[2*i + 1];
			ret = holes[2*i];
		}
	}

	//no suitable hole found
	if(bestFitty == std::numeric_limits<int>::max()){
		return -1;
	}

	//ret is address of the bestFit hole
	return ret;
	
}

//Returns word offset of hole selected by the worst fit memory allocation algorithm, and -1 if there is no fit.
int worstFit(int sizeInWords, void *list){
	uint16_t bitstreamLen = *(static_cast<uint16_t*>(list));
	//number of holes
	int num = (int)bitstreamLen/4; //2 bytes per hole field * 2 hole fields (start & len)
	uint16_t* holes = (uint16_t*) (list) + 1; //2 is 13

	int worstFitty = -1;
	int ret = -1;
	for(int i = 0; i < num; i++){
		//std::cout << " hole is at " << holes[2*i] << " and hole len is " << holes[2*i + 1] << std::endl;
		if(holes[2*i + 1] >= sizeInWords && holes[2*i + 1] > worstFitty){
			worstFitty = holes[2*i + 1];
			ret = holes[2*i];
		}
	}
	if(worstFitty == -1){
		return -1;
	}
	//std::cout << "bye\n\n";
	return ret;
}


// int main(){
// 	/*std::cout << "hi\n\n";
// 	MemoryManager* mem = new MemoryManager(4, bestFit);
// 	mem->printMem();
// 	mem->initialize(24);
// 	mem->printMem();
// 	//std::cout << " 7 " << std::endl;
// 	uint16_t* vec = static_cast<uint16_t*>(mem->getList());
// 	mem->printMem();
// 	mem->allocate(8);
// 	mem->printMem();
// 	char* c = (char*)mem->allocate(2);
// 	mem->printMem();
// 	mem->free(c);
// 	mem->printMem();
// 	//std::cout << " 8 " << std::endl;
// 	//std::cout << vec[0] << " rofl " << std::endl;
// 	//std::cout << vec[1] << " rofl " << std::endl;
// 	//std::cout << vec[2] << " rofl " << std::endl;
// 	//std::cout << vec[3] << " rofl " << std::endl;
// 	//std::cout << vec[4] << " rofl " << std::endl;
// 	mem->allocate(120);
// 	mem->printMem();
// 	std::cout << mem->getMemoryStart() << "lol\n";
// 	delete mem;*/

// 	//minitest code:
// 	int wordSize = 4;
//     MemoryManager memoryManager(wordSize, bestFit);
//     memoryManager.initialize(26);
//     void* testA = memoryManager.allocate(wordSize * 10);
//     memoryManager.printMem();
// 	void* testB = memoryManager.allocate(wordSize * 2);
// 	memoryManager.printMem();
//     void* testC = memoryManager.allocate(wordSize * 2);
//     memoryManager.printMem();
//     void* testD = memoryManager.allocate(wordSize * 6);
//     memoryManager.printMem();
//     memoryManager.free(testA);
//     memoryManager.printMem();
//     memoryManager.free(testC);
//     memoryManager.printMem();
// 	/*std::vector<uint16_t> test;
// 	test.push_back(12);
// 	test.push_back(13);
// 	test.push_back(1);
// 	test.push_back(15);
// 	test.push_back(2);
// 	test.push_back(11);
// 	test.push_back(1);
// 	for(auto& t : test){
// 		std::cout << +t << std::endl;
// 	}
// 	std::cout << worstFit(1, (void*) test.data());
// 	std::cout << bestFit(2, (void*) test.data());*/
	
// 	//~mem;
// 	return 0;
// }
