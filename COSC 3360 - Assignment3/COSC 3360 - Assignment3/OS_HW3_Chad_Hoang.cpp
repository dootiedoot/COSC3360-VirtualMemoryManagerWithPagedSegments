#include <iostream>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <string.h>
#include <vector>
#include <sstream>
#include <math.h>

using namespace std;

/*
	Assignment 3
	COSC 3360 - Fundamentals of Operating Systems
	University of Houston
	Chad Hoang
	1176413
*/

#pragma region ----------------------------		Variables	----------------------------
int mainMemoryMaxSize;				/* total_number_of_page_frames (in main memory) */
int segmentLength;					/* maximum segment length (in number of pages) */
int pageSize;						/* page size (in number of bytes) */
int pageFramePerProcess;			/* number_of_page_frames_per_process for FIFO, LRU, LFU and OPT, or delta (window size) for the Working Set algorithm */
int	lookAheadwindow;				/* lookahead window size for OPT, 0 for others (which do not use this value) */
int windowMin;
int windowMax;
int totalProcesses;					/* total number of processes */

typedef struct Address
{
	int segment;
	int page;
	int displacement;
}Address;

typedef struct Frame
{
	bool isAllocated;
	int processID;
	int pageID;
	int segmentID;
	int frameID;
	Frame(int id)
	{
		frameID = id;
	}
}Frame;

typedef struct Page
{
	bool isAllocated;
	int frameID;
	int pageID;
	Page(int id)
	{
		pageID = id;
		frameID = 0;
		isAllocated = false;
	}
}Page;

typedef struct PageTable
{
	vector<Page> pages;
}PageTable;

typedef struct Segment
{
	int segmentID;
	PageTable pageTable;
	Segment(int id)
	{
		segmentID = id;
	}
}Segment;

typedef struct SegmentTable
{
	vector<Segment> segments;
}SegmentTable;

typedef struct AddressSpace
{
	vector<SegmentTable> segmentTables;		//	Array that contains all the segment tables in this address space
	vector<Page> LastPagesUsed;
}AddressSpace;

typedef struct Process						//	Structure of the process
{
	int processID;							/* process_id followed by total number of page frames on disk */
	int totalPageFramesOnDisk;
	AddressSpace addressSpace;				//	AddressSpace that contains of all the addresses 
}Process;
Process *processes;							//	Array of all the structures of the processes

vector<Frame> framesInMainMemory;			//	Array of frames currently in main memory
vector<Frame> framesInDiskTable;			//	Array of frames stored in the disk table

vector<string> instructions;				// Array of the instructions in order
#pragma endregion

#pragma region ----------------------------		Methods		----------------------------
void ReadFromFile(string FileName);
void ClearForNextPageReplacement();
void BuildMainMemory(int maxFrameSize);
void AllocateIntoDisk(Process *pProcess);
void FIFO_PageReplacement();
void LRU_PageReplacement();
void LFU_PageReplacement();
#pragma endregion

#pragma region ----------------------------		Main		----------------------------
int main(int argc, char* argv[])
{
	cout << endl;

	//	Read, Evaluate, and Assign variables based in the input .txt file supplied by command argument
	ReadFromFile(argv[1]);

	//	Display statistics
	cout << "\n-------------------------	Parameters	------------------------\n" << endl;
	cout << "Total pages: \t\t\t" << mainMemoryMaxSize << endl;
	cout << "Max segment length: \t\t" << segmentLength << endl;
	cout << "Page size: \t\t\t" << pageSize << endl;
	cout << "Page frame per process: \t" << pageFramePerProcess << endl;
	cout << "Look ahead window: \t\t" << lookAheadwindow << endl;
	cout << "Window Min: \t\t\t" << windowMin << endl;
	cout << "Window Max: \t\t\t" << windowMax << endl;
	cout << "Total processes: \t\t" << totalProcesses << endl;

	/*for (size_t i = 0; i < totalProcesses; i++)
		cout << "Process " << processes[i].processID << " has " << processes[i].totalPageFramesOnDisk << " total page frames on disk." << endl;*/

	cout << "\n-------------------------	FIFO Page Replacement	------------------------\n" << endl;
	FIFO_PageReplacement();

	ClearForNextPageReplacement();

	cout << "\n-------------------------	LRU Page Replacement	------------------------\n" << endl;
	LRU_PageReplacement();

	ClearForNextPageReplacement();

	cout << "\n-------------------------	LFU Page Replacement	------------------------\n" << endl;
	//LFU_PageReplacement();

	ClearForNextPageReplacement();

	cout << "\n-------------------------	OPT Page Replacement	------------------------\n" << endl;


	ClearForNextPageReplacement();

	cout << "\n-------------------------	WS Page Replacement	------------------------\n" << endl;

}
#pragma endregion

#pragma region ReadFromFile(): Read, Evaluate, and Assign variables based in the input .txt file supplied by command argument
void ReadFromFile(string FileName)
{
	//	input the string argument of the input file
	fstream inputFile(FileName);
	
	//	Evaluate the input file
	if (inputFile.is_open())
	{
		string currentLine;

		//	Fetch next line & cache totalPages
		getline(inputFile, currentLine);
		mainMemoryMaxSize = stoi(currentLine);

		//	Fetch next line & cache maxSegmentLength
		getline(inputFile, currentLine);
		segmentLength = stoi(currentLine);

		//	Fetch next line & cache pageSize
		getline(inputFile, currentLine);
		pageSize = stoi(currentLine);
		
		//	Fetch next line & cache pageFramePerProcess
		getline(inputFile, currentLine);
		pageFramePerProcess = stoi(currentLine);

		//	Fetch next line & cache lookAheadwindow
		getline(inputFile, currentLine);
		lookAheadwindow = stoi(currentLine);

		//	Fetch next line & cache windowMin
		getline(inputFile, currentLine);
		windowMin = stoi(currentLine);

		//	Fetch next line & cache windowMax
		getline(inputFile, currentLine);
		windowMax = stoi(currentLine);

		//	Fetch next line & cache totalProcesses
		getline(inputFile, currentLine);
		totalProcesses = stoi(currentLine);

		//	Fetch next line & add to instruction array
		while (getline(inputFile, currentLine))
			instructions.push_back(currentLine);	
	}
}
#pragma endregion

#pragma region ClearForNextPageReplacement()
void ClearForNextPageReplacement()
{
	framesInMainMemory.clear();
	framesInDiskTable.clear();
	delete[] processes;
	processes = NULL;
}
#pragma endregion

#pragma region BuildMainMemory(int maxFrameSize)
void BuildMainMemory(int maxFrameSize)
{
	//framesInMainMemory.clear();
	for (int i = 0; i < maxFrameSize; i++)
	{
		framesInMainMemory.push_back(*(new Frame(i)));
	}
}
#pragma endregion

#pragma region AllocateIntoDisk(Process *pProcess)
void AllocateIntoDisk(Process *pProcess)
{
	if (pProcess->addressSpace.segmentTables.size() == 0)
	{
		pProcess->addressSpace.segmentTables.push_back(*(new SegmentTable()));
	}
	for (int i = 0; i < pProcess->totalPageFramesOnDisk; i++)
	{
		int page = i % segmentLength;
		int segment = i / segmentLength;
		if (segment >= pProcess->addressSpace.segmentTables[0].segments.size())
		{
			pProcess->addressSpace.segmentTables[0].segments.push_back(*(new Segment(segment)));
		}
		if (page >= pProcess->addressSpace.segmentTables[0].segments[segment].pageTable.pages.size())
		{
			pProcess->addressSpace.segmentTables[0].segments[segment].pageTable.pages.push_back(*(new Page(page)));
		}

		Frame *pFrame = new Frame(i + framesInDiskTable.size());
		pFrame->isAllocated = true;
		pFrame->pageID = page;
		pFrame->segmentID = segment;
		pFrame->processID = pProcess->processID;
		
		framesInDiskTable.push_back(*pFrame);
	}
}
#pragma endregion

#pragma region FIFO_PageReplacement()
void FIFO_PageReplacement()
{
	//	Initialize the main memory 
	BuildMainMemory(mainMemoryMaxSize);

	//	Initalize processes array
	processes = new Process[totalProcesses];

	for (size_t i = 0; i < totalProcesses; i++)
	{
		istringstream iss(instructions[i]);

		//	Cache processID
		string str;
		iss >> str;
		processes[i].processID = stoi(str);

		//	Cache process totalPageFramesOnDisk
		iss >> str;
		processes[i].totalPageFramesOnDisk = stoi(str);
		AllocateIntoDisk(&processes[i]);
	}
	
	//	Loop through each instruction and perform parse
	for (size_t j = totalProcesses; j < instructions.size(); j++)
	{
		string instruction = instructions[j];
		
		for (size_t i = 0; i < totalProcesses; i++)
		{
			string processIDtoString = to_string(processes[i].processID);
			if (instruction.find(processIDtoString) != string::npos)
			{
				//	Shave currentLine so it only contains the address string
				string addressString = instruction.erase(0, processIDtoString.length() + 1);

				//	Convert address string to an int
				char *cstr = new char[addressString.length() + 1];
				strcpy(cstr, addressString.c_str());
				char *pLine;
				unsigned int addressToInt = (unsigned int)strtol(cstr, &pLine, 0);
				if (((int)addressToInt) < 0) 
				{
					continue;
				}
				//	Parse address for our Displacement, Page, and Segment
				int displacement = addressToInt & ((1 << (int)log2(pageSize)) - 1);
				int pageID = (addressToInt >> (int)log2(pageSize)) & ((1 << (int)log2(segmentLength)) - 1);
				int segmentID = addressToInt >> ((int)log2(pageSize) + (int)log2(segmentLength));

				printf("Attempting page access with Process %d, Segment %d, Page %d, Segment Length %d.\n", i, segmentID, pageID, processes[i].addressSpace.segmentTables[0].segments.size());
				//	Attempt to page access
				Page *pPageAccessing = &processes[i].addressSpace.segmentTables[0].segments[segmentID].pageTable.pages[pageID];

				Frame *pFrame = NULL;
				bool PageFault = false;
				//	if the accessing page is allocated, page fault
				if (pPageAccessing->isAllocated == false)
				{
					PageFault = true;

					cout << "\033[0;31m * Page fault \033[0m" << endl;
				}
				//	hit
				else if (!(framesInMainMemory[pPageAccessing->frameID].processID == processes[i].processID &&
					framesInMainMemory[pPageAccessing->frameID].pageID == pageID &&
					framesInMainMemory[pPageAccessing->frameID].segmentID == segmentID))
				{
					PageFault = true;
					cout << "\033[0;33m * Page fault (Was Replaced) \033[0m" << endl;
				}

				if (PageFault)
				{
					if (processes[i].addressSpace.LastPagesUsed.size() < pageFramePerProcess)
					{
						//	look for empty space in the main memory
						for (int k = 0; k < framesInMainMemory.size(); k++)
						{
							if (framesInMainMemory[k].isAllocated == false)
							{
								pFrame = &framesInMainMemory[k];
								cout << "\033[0;32m Found unallocated frame in main memory. \033[0m" << endl;
								break;
							}
						}
						if (pFrame == NULL)
						{
							printf("\033[0;31m *ERROR*, pFrame==NULL \033[0m\n");
							break;
						}
					}
					else //Replace Page
					{
						pFrame = &framesInMainMemory[processes[i].addressSpace.LastPagesUsed[0].frameID];
						processes[i].addressSpace.LastPagesUsed.pop_back();
						printf("\033[0;32m Replaced frame %d...\033[0m\n", pFrame->frameID);
					}
					//printf("ALlocating to Frame(%d)\n", pFrame->frameID);
					pFrame->isAllocated = true;

					pPageAccessing->frameID = pFrame->frameID;

					pFrame->processID = processes[i].processID;
					pFrame->pageID = pageID;
					pFrame->segmentID = segmentID;
					//printf("Setting Page(%d) isAllocated To True\n", pPageAccessing->pageID);
					pPageAccessing->isAllocated = true;
					processes[i].addressSpace.LastPagesUsed.push_back(*pPageAccessing);
				}
				else
				{
					printf("\033[0;32m Page Found. \033[0m\n");
				}

				break;
			}
		}
	}
}
#pragma endregion

#pragma region LRU_PageReplacement()
void LRU_PageReplacement()
{
	//	vector that keeps track of the pages in chronological order from oldest begin to newest end
	vector<Page*> LRU_Array;
	
	//	Initialize the main memory 
	BuildMainMemory(mainMemoryMaxSize);

	//	Initalize processes array
	processes = new Process[totalProcesses];

	for (size_t i = 0; i < totalProcesses; i++)
	{
		istringstream iss(instructions[i]);

		//	Cache processID
		string str;
		iss >> str;
		processes[i].processID = stoi(str);

		//	Cache process totalPageFramesOnDisk
		iss >> str;
		processes[i].totalPageFramesOnDisk = stoi(str);
		AllocateIntoDisk(&processes[i]);
	}

	//	Loop through each instruction and perform parse
	for (size_t j = totalProcesses; j < instructions.size(); j++)
	{
		string instruction = instructions[j];

		for (size_t i = 0; i < totalProcesses; i++)
		{
			string processIDtoString = to_string(processes[i].processID);
			if (instruction.find(processIDtoString) != string::npos)
			{
				//	Shave currentLine so it only contains the address string
				string addressString = instruction.erase(0, processIDtoString.length() + 1);

				//	Convert address string to an int
				char *cstr = new char[addressString.length() + 1];
				strcpy(cstr, addressString.c_str());
				char *pLine;
				unsigned int addressToInt = (unsigned int)strtol(cstr, &pLine, 0);

				if (((int)addressToInt) < 0)
					continue;

				//	Parse address for our Displacement, Page, and Segment
				int displacement = addressToInt & ((1 << (int)log2(pageSize)) - 1);
				int pageID = (addressToInt >> (int)log2(pageSize)) & ((1 << (int)log2(segmentLength)) - 1);
				int segmentID = addressToInt >> ((int)log2(pageSize) + (int)log2(segmentLength));

				printf("Attempting page access with Process %d, Segment %d, Page %d, Segment Length %d.\n", i, segmentID, pageID, processes[i].addressSpace.segmentTables[0].segments.size());
				//	Attempt to page access
				Page *pPageAccessing = &processes[i].addressSpace.segmentTables[0].segments[segmentID].pageTable.pages[pageID];

				Frame *pFrame = NULL;
				bool PageFault = false;
				//	if the accessing page is allocated, page fault
				if (pPageAccessing->isAllocated == false)
				{
					PageFault = true;

					cout << "\033[0;31m * Page fault \033[0m" << endl;
				}
				//	hit
				else if (!(framesInMainMemory[pPageAccessing->frameID].processID == processes[i].processID &&
					framesInMainMemory[pPageAccessing->frameID].pageID == pageID &&
					framesInMainMemory[pPageAccessing->frameID].segmentID == segmentID))
				{
					PageFault = true;
					cout << "\033[0;33m * Page fault (Was Replaced) \033[0m" << endl;
				}

				if (PageFault)
				{
					if (LRU_Array.size() < pageFramePerProcess)
					{
						//	look for empty space in the main memory
						for (int k = 0; k < framesInMainMemory.size(); k++)
						{
							if (framesInMainMemory[k].isAllocated == false)
							{
								pFrame = &framesInMainMemory[k];
								cout << "\033[0;32m Found unallocated frame in main memory. \033[0m" << endl;
								
								break;
							}
						}
						if (pFrame == NULL)
						{
							printf("\033[0;31m *ERROR*, pFrame==NULL \033[0m\n");
							break;
						}
					}
					else //Replace Page
					{
						//	set frame pointer to head
						pFrame = &framesInMainMemory[LRU_Array[0]->frameID];

						//	remove head from LRU ist
						LRU_Array.erase(LRU_Array.begin());

						printf("\033[0;32m Replaced frame %d...\033[0m\n", pFrame->frameID);
					}

					//printf("ALlocating to Frame(%d)\n", pFrame->frameID);
					pFrame->isAllocated = true;
					pFrame->processID = processes[i].processID;
					pFrame->pageID = pageID;
					pFrame->segmentID = segmentID;
					
					//	set page to know what frame
					pPageAccessing->frameID = pFrame->frameID;
					pPageAccessing->isAllocated = true;

					i--;
					continue;
				}
				else
				{
					//	Search if the page already exist, remove it from head
					for (size_t l = 0; l < LRU_Array.size(); l++)
						if (LRU_Array.front()->pageID == LRU_Array[l]->pageID
							&& LRU_Array.front()->frameID == LRU_Array[l]->frameID)
						{
							LRU_Array.erase(LRU_Array.begin());
							break;
						}

					//	Push back the new page pointer
					LRU_Array.push_back(pPageAccessing);
					printf("\033[0;32m Page Found. \033[0m\n");
					cout << LRU_Array.size() << endl;
				}

				break;
			}
		}
	}
}
#pragma endregion

#pragma region LFU_PageReplacement()
void LFU_PageReplacement()
{
	//	Initialize the main memory 
	BuildMainMemory(mainMemoryMaxSize);

	//	Initalize processes array
	processes = new Process[totalProcesses];

	for (size_t i = 0; i < totalProcesses; i++)
	{
		istringstream iss(instructions[i]);

		//	Cache processID
		string str;
		iss >> str;
		processes[i].processID = stoi(str);

		//	Cache process totalPageFramesOnDisk
		iss >> str;
		processes[i].totalPageFramesOnDisk = stoi(str);
		AllocateIntoDisk(&processes[i]);
	}

	//	Loop through each instruction and perform parse
	for (size_t j = totalProcesses; j < instructions.size(); j++)
	{
		string instruction = instructions[j];

		for (size_t i = 0; i < totalProcesses; i++)
		{
			string processIDtoString = to_string(processes[i].processID);
			if (instruction.find(processIDtoString) != string::npos)
			{
				//	Shave currentLine so it only contains the address string
				string addressString = instruction.erase(0, processIDtoString.length() + 1);

				//	Convert address string to an int
				char *cstr = new char[addressString.length() + 1];
				strcpy(cstr, addressString.c_str());
				char *pLine;
				unsigned int addressToInt = (unsigned int)strtol(cstr, &pLine, 0);
				if (((int)addressToInt) < 0)
				{
					continue;
				}
				//	Parse address for our Displacement, Page, and Segment
				int displacement = addressToInt & ((1 << (int)log2(pageSize)) - 1);
				int pageID = (addressToInt >> (int)log2(pageSize)) & ((1 << (int)log2(segmentLength)) - 1);
				int segmentID = addressToInt >> ((int)log2(pageSize) + (int)log2(segmentLength));

				printf("Attempting page access with Process %d, Segment %d, Page %d, Segment Length %d.\n", i, segmentID, pageID, processes[i].addressSpace.segmentTables[0].segments.size());
				//	Attempt to page access
				Page *pPageAccessing = &processes[i].addressSpace.segmentTables[0].segments[segmentID].pageTable.pages[pageID];

				Frame *pFrame = NULL;
				bool PageFault = false;
				//	if the accessing page is allocated, page fault
				if (pPageAccessing->isAllocated == false)
				{
					PageFault = true;

					cout << "\033[0;31m * Page fault \033[0m" << endl;
				}
				//	hit
				else if (!(framesInMainMemory[pPageAccessing->frameID].processID == processes[i].processID &&
					framesInMainMemory[pPageAccessing->frameID].pageID == pageID &&
					framesInMainMemory[pPageAccessing->frameID].segmentID == segmentID))
				{
					PageFault = true;
					cout << "\033[0;33m * Page fault (Was Replaced) \033[0m" << endl;
				}

				if (PageFault)
				{
					if (processes[i].addressSpace.LastPagesUsed.size() < pageFramePerProcess)
					{
						//	look for empty space in the main memory
						for (int k = 0; k < framesInMainMemory.size(); k++)
						{
							if (framesInMainMemory[k].isAllocated == false)
							{
								pFrame = &framesInMainMemory[k];
								cout << "\033[0;32m Found unallocated frame in main memory. \033[0m" << endl;
								break;
							}
						}
						if (pFrame == NULL)
						{
							printf("\033[0;31m *ERROR*, pFrame==NULL \033[0m\n");
							break;
						}
					}
					else //Replace Page
					{
						pFrame = &framesInMainMemory[processes[i].addressSpace.LastPagesUsed[0].frameID];
						processes[i].addressSpace.LastPagesUsed.pop_back();
						printf("\033[0;32m Replaced frame %d...\033[0m\n", pFrame->frameID);
					}
					//printf("ALlocating to Frame(%d)\n", pFrame->frameID);
					pFrame->isAllocated = true;

					pPageAccessing->frameID = pFrame->frameID;

					pFrame->processID = processes[i].processID;
					pFrame->pageID = pageID;
					pFrame->segmentID = segmentID;
					//printf("Setting Page(%d) isAllocated To True\n", pPageAccessing->pageID);
					pPageAccessing->isAllocated = true;
					processes[i].addressSpace.LastPagesUsed.push_back(*pPageAccessing);
				}
				else
				{
					printf("\033[0;32m Page Found. \033[0m\n");
				}

				break;
			}
		}
	}
}
#pragma endregion