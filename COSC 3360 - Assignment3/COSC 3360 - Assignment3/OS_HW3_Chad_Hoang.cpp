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

// Variables
int totalPages;						/* total_number_of_page_frames (in main memory) */
int maxSegmentLength;				/* maximum segment length (in number of pages) */
int pageSize;						/* page size (in number of bytes) */
int pageFramePerProcess;			/* number_of_page_frames_per_process for FIFO, LRU, LFU and OPT, or delta (window size) for the Working Set algorithm */
int	lookAheadwindow;				/* lookahead window size for OPT, 0 for others (which do not use this value) */
int windowMin;
int windowMax;
int totalProcesses;					/* total number of processes */
struct Process						//	Structure of the process
{
	int processID;					/* process_id followed by total number of page frames on disk */
	int totalPageFramesOnDisk;
};
Process *processes;					//	Array that contains all the structures of the processes
int *processInstructionsOrder;		//	Array containing order of processIDs for address instruction list
string *addressInstructions;		//	Array containing order of adddress instructions

//	Methods
#pragma region Method initalization
void ReadFromFile(string FileName);

int main(int argc, char* argv[])
{
	cout << endl;

	//	Initalize instruction arrays
	int totalInstructions = 100;
	processInstructionsOrder = new int[totalInstructions];
	addressInstructions = new string[totalInstructions];

	//	Read, Evaluate, and Assign variables based in the input .txt file supplied by command argument
	ReadFromFile(argv[1]);

	cout << "Total pages: \t\t\t" << totalPages << endl;
	cout << "Max segment length: \t\t" << maxSegmentLength << endl;
	cout << "Page size: \t\t\t" << pageSize << endl;
	cout << "Page frame per process: \t" << pageFramePerProcess << endl;
	cout << "Look ahead window: \t\t" << lookAheadwindow << endl;
	cout << "Window Min: \t\t\t" << windowMin << endl;
	cout << "Window Max: \t\t\t" << windowMax << endl;
	cout << "Total processes: \t\t" << totalProcesses << endl;

	for (size_t i = 0; i < totalProcesses; i++)
		cout << "Process " << processes[i].processID << " has " << processes[i].totalPageFramesOnDisk << " total page frames on disk." << endl;

	for (int i = 0; i < totalInstructions; ++i)
		cout << "Process " << processInstructionsOrder[i] << " has address instruction " << addressInstructions[i] << endl;
}

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
		int instructionCounter = 0;
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
					processInstructionsOrder[instructionCounter] = processes[i].processID;
					//cout << instructionCounter << endl;
					//cout << sizeof(addressInstructions) / sizeof(addressInstructions[0]) << endl;
					//cout << addressInstruction << endl;
					addressInstructions[instructionCounter] = addressInstruction;


					break;
				}
			}

			instructionCounter++;
		}
	}
}
#pragma endregion