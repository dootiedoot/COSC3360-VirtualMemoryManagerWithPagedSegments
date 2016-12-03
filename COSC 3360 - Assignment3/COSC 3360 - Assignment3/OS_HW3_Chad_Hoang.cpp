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

using namespace std;

/*
	Assignment 3
	COSC 3360 - Fundamentals of Operating Systems
	University of Houston
	Chad Hoang
	1176413
*/

#pragma region ----------------------------		Variables	----------------------------
int totalPages;						/* total_number_of_page_frames (in main memory) */
int maxSegmentLength;				/* maximum segment length (in number of pages) */
int pageSize;						/* page size (in number of bytes) */
int pageFramePerProcess;			/* number_of_page_frames_per_process for FIFO, LRU, LFU and OPT, or delta (window size) for the Working Set algorithm */
int	lookAheadwindow;				/* lookahead window size for OPT, 0 for others (which do not use this value) */
int windowMin;
int windowMax;
int totalProcesses;					/* total number of processes */

vector<int> processInstructionsOrder;		//	Array of order of processIDs for address instruction list
vector<string> addressInstructions;			//	Array of order of adddress instructions
vector<int> addressToDisplacment;			//	Array of order of adddress instructions

typedef struct Address
{
	int segment;
	int page;
	int displacement;
}Address;
typedef struct Frame
{
	int processID;
	string address;
}Frame;
typedef struct Page
{
	Frame frame;
}Page;
typedef struct PageTable
{
	vector<Page> pages;
}PageTable;
typedef struct Segment
{
	PageTable pageTable;
}Segment;

typedef struct SegmentTable
{
	vector<Segment> segments;
}SegmentTable;
typedef struct AddressSpace
{
	vector<Address> addresses;				//	Array that contains all the addresses
	vector<SegmentTable> segmentTables;		//	Array that contains all the segment tables in this address space
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
#pragma endregion

#pragma region ----------------------------		Methods		----------------------------
void ReadFromFile(string FileName);
#pragma endregion

#pragma region ----------------------------		Main		----------------------------
int main(int argc, char* argv[])
{
	cout << endl;

	//	Read, Evaluate, and Assign variables based in the input .txt file supplied by command argument
	ReadFromFile(argv[1]);

	//	Display statistics
	cout << "Total pages: \t\t\t" << totalPages << endl;
	cout << "Max segment length: \t\t" << maxSegmentLength << endl;
	cout << "Page size: \t\t\t" << pageSize << endl;
	cout << "Page frame per process: \t" << pageFramePerProcess << endl;
	cout << "Look ahead window: \t\t" << lookAheadwindow << endl;
	cout << "Window Min: \t\t\t" << windowMin << endl;
	cout << "Window Max: \t\t\t" << windowMax << endl;
	cout << "Total processes: \t\t" << totalProcesses << endl;

	cout << endl;
	for (size_t i = 0; i < totalProcesses; i++)
		cout << "Process " << processes[i].processID << " has " << processes[i].totalPageFramesOnDisk << " total page frames on disk." << endl;

	cout << endl;
	for (int i = 0; i < totalProcesses; ++i)
	{
		for (size_t j = 0; j < processes[i].addressSpace.segmentTables.size(); j++)
		{
			for (size_t k = 0; k < processes[i].addressSpace.segmentTables[j].segments.size(); k++)
			{
				for (size_t l = 0; l < processes[i].addressSpace.segmentTables[j].segments[k].pageTable.pages.size(); l++)
				{
					cout << "Process " << processes[i].processID <<
						" segment table " << j <<
						" segment " << k <<
						" page " << l <<
						" frame " << processes[i].addressSpace.segmentTables[j].segments[k].pageTable.pages[l].frame.address << endl;
				}
			}
		}
		cout << endl;
	}
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
		totalPages = stoi(currentLine);

		//	Fetch next line & cache maxSegmentLength
		getline(inputFile, currentLine);
		maxSegmentLength = stoi(currentLine);

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

		//	Initalize processes array
		processes = new Process[totalProcesses];

		for (size_t i = 0; i < totalProcesses; i++)
		{
			//	Fetch next line & cache process parameters
			getline(inputFile, currentLine);

			istringstream iss(currentLine);

			//	Cache processID
			string str;
			iss >> str;
			processes[i].processID = stoi(str);

			//	Cache process totalPageFramesOnDisk
			iss >> str;
			processes[i].totalPageFramesOnDisk = stoi(str);
		}

		//	Fetch next line & evaluate
		while (getline(inputFile, currentLine))
		{
			//cout << "parsing: " << currentLine << endl;

			for (size_t i = 0; i < totalProcesses; i++)
			{
				string processIDtoString = to_string(processes[i].processID);
				if (currentLine.find(processIDtoString) != string::npos)
				{
					//cout << "found process ID " << processes[i].processID << endl;

					//	Shave currentLine so it only contains the address string
					string addressInstruction = currentLine.erase(0, processIDtoString.length()+1);

					//	Add corresponding processID and address to our arrays
					processInstructionsOrder.push_back(processes[i].processID);
					//cout << instructionCounter << endl;
					//cout << sizeof(addressInstructions) / sizeof(addressInstructions[0]) << endl;
					//cout << addressInstruction << endl;
					addressInstructions.push_back(addressInstruction);

					SegmentTable newSegmentTable;
					Segment newSegment;
					Page newPage;
					Frame newFrame;

					newFrame.processID = processes[i].processID;
					newFrame.address = addressInstruction;

					newPage.frame = newFrame;

					newSegment.pageTable.pages.push_back(newPage);

					newSegmentTable.segments.push_back(newSegment);

					processes[i].addressSpace.segmentTables.push_back(newSegmentTable);

					/*char *cstr = new char[addressInstruction.length() + 1];
					strcpy(cstr, addressInstruction.c_str());
					char *pLine;
					int addressToInt = strtol(cstr, &pLine, 0);
					delete[] cstr;*/

					//addressToDisplacment.push_back(addressToInt);

					break;
				}
			}
		}
	}
}
#pragma endregion