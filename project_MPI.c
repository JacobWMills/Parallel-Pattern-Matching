// Jacob Mills

#include "mpi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

////////////////////////////////////////////////////////////////////////////////
// Program main
////////////////////////////////////////////////////////////////////////////////
#define INPUTFOLDER "inputs"
#define MAXTEXTS 20
#define MAXPATTERNS 20
#define BUFSIZE 100000
#define MASTER 0
#define JOBSIZE 768

// Reading/writing files

void outOfMemory()
{
	fprintf (stderr, "Out of memory\n");
	exit (0);
}

void readFromFile (FILE *f, char **data, int *length)
{
	int ch;
	int allocatedLength;
	char *result;
	int resultLength = 0;

	allocatedLength = 0;
	result = NULL;

	ch = fgetc (f);
	while (ch >= 0)
	{
		resultLength++;
		if (resultLength > allocatedLength)
		{
			allocatedLength += 10000;
			result = (char *) realloc (result, sizeof(char)*allocatedLength);
			if (result == NULL)
				outOfMemory();
		}
		result[resultLength-1] = ch;
		ch = fgetc(f);
	}
	*data = result;
	*length = resultLength;
}

int readTexts(char **textData, int *textLength)
{
	int i;
	for (i = 0; i < MAXTEXTS; i++)
	{
		FILE *f;
		char fileName[1000];
	#ifdef DOS
		sprintf (fileName, "%s\\text%d.txt", INPUTFOLDER, i);
	#else
		sprintf (fileName, "%s/text%d.txt", INPUTFOLDER, i);
	#endif
		f = fopen(fileName, "r");
		if (f == NULL)
			return i;
		readFromFile (f, &textData[i], &textLength[i]);
		fclose (f);
	}
	return MAXTEXTS;
}

int readPatterns(char **patternData, int *patternLength)
{
	int i;
	for (i = 0; i < MAXPATTERNS; i++)
	{
		FILE *f;
		char fileName[1000];

	#ifdef DOS
		sprintf (fileName, "%s\\pattern%d.txt", INPUTFOLDER, i);
	#else
		sprintf (fileName, "%s/pattern%d.txt", INPUTFOLDER, i);
	#endif
		f = fopen (fileName, "r");
		if (f == NULL)
			return i;
		readFromFile (f, &patternData[i], &patternLength[i]);
		fclose (f);
	}
	return MAXPATTERNS;
}

int readControl(int controlData[][3])
{
	FILE *f;
	char fileName[1000];

#ifdef DOS
	sprintf (fileName, "%s\\control.txt", INPUTFOLDER);
#else
	sprintf (fileName, "%s/control.txt", INPUTFOLDER);
#endif
	f = fopen (fileName, "r");
	if (f == NULL)
		return 0;
	
	int r;
	int i = 0;

	while ((r = fscanf(f, "%d %d %d", &controlData[i][0], &controlData[i][1], &controlData[i][2])) != EOF)
	{
		if (r == 3)
			i++;
	}
		
	fclose (f);
	
	return i;
}

void writeOutputFile(char buf[])
{
	FILE *f;
	char fileName[1000];

	sprintf (fileName, "result_MPI.txt");
	
	f = fopen(fileName, "a+");	
	if (f == NULL)
		return;
	fprintf(f, buf);
	fclose(f);
	
	buf[0] = '\0';
}

// Only master should call this
void writeToBuffer(char buf[], int textIndex, int patternIndex, int matchPoint)
{
	if (strlen(buf) > BUFSIZE - 20)
		writeOutputFile(buf);
	
	sprintf(buf+strlen(buf), "%d %d %d\n", textIndex, patternIndex, matchPoint);	
}

// Find pattern at given text position and the positions following for lengthToCheck
int checkTextForPatternAtPosition(char text[], char pattern[], int textIndex, int textLength, int patternLength, int lengthToCheck, int results[JOBSIZE])
{
	int i;
	int found = 0;
		
	for (i = 0; i < lengthToCheck; i++)
	{
		if (textLength - (textIndex + i) < patternLength)
			break;
		int k = textIndex + i;
		int j = 0;
			
		while (text[k] == pattern[j] && j < patternLength)
		{
			k++;
			j++;
		}
		
		if (j == patternLength)
		{
			results[found++] = textIndex + i;
		}
	}
	
	if (found < JOBSIZE)
		results[found] = -1; // Use this to indicate to master where array results end.
		
	return found;
}

void masterLoop(int numtasks, int numberOfTests, int controlData[][3], int* textLength, int* patternLength, char* buf)
{
	// Loop through each test
	int testNumber;
	for (testNumber = 0; testNumber < numberOfTests; testNumber++)
	{
		int mode = controlData[testNumber][0];
		int textIndex = controlData[testNumber][1];
		int patternIndex = controlData[testNumber][2];
	
		// If text is shorter than pattern, it's not gonna be found.
		if (textLength[textIndex] < patternLength[patternIndex])
		{
			sprintf(buf+strlen(buf), "%d %d %d\n", textIndex, patternIndex, -1);
			continue;
		}
		
		int found = -1;
		int activeThreads = 0;
		
		int i;			
		
		int checkData[4] = {textIndex, patternIndex, 0, JOBSIZE}; // {text#, pattern#, text index to check, length of text to check}
		
		// Send a job to all the threads to start off with
		for (i = 1; i < numtasks; i++)
		{
			if (checkData[2] <= textLength[textIndex] - patternLength[patternIndex]) // Unless the text is shorter than the number of threads
			{
				MPI_Send(&checkData, 4, MPI_INT, i, 0, MPI_COMM_WORLD); // A
				checkData[2]+=checkData[3]; 
				activeThreads++;
			}
			else
				break;
		}
		int done = 0;
	
		// Loop over each character in the text until the test is complete
		while (!done && activeThreads > 0) 
		{
			int results[JOBSIZE];
			MPI_Status s;
			MPI_Recv(&results, JOBSIZE, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &s); // B
			activeThreads--;
			
			// If a thread has found an occurrence of the pattern
			if (results[0] != -1)
			{
				// Mode 0 - find first instance
				if (mode == 0)
				{
					found = results[0];
					// Wait for remaining active threads to return, in case they find an earlier result
					while (activeThreads > 0)
					{
						MPI_Recv(&results, JOBSIZE, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &s); // B
						
						if (results[0] != -1 )
						{
							//int minVal = min(results);
							int minVal = results[0];
							if (minVal < found)
								found = minVal;
						}
						activeThreads--;
					}
					
					// Record the result and exit this test by setting done
					sprintf(buf+strlen(buf), "%d %d %d\n", textIndex, patternIndex, found);
					done = 1;
				}
				// Mode 1 - find all instances
				else
				{
					// Record the result and set found
					found = results[0];
					int i = 0;
					while (i < JOBSIZE && results[i] != -1)
						sprintf(buf+strlen(buf), "%d %d %d\n", textIndex, patternIndex, results[i++]);
				}
			}
			// Thread did not find pattern (mode 0) and/or still text left to be analysed (mode 0/1) 
			if (!done && checkData[2] <= textLength[textIndex] - patternLength[patternIndex])
			{					
				MPI_Send(&checkData, 4, MPI_INT, s.MPI_SOURCE, 0, MPI_COMM_WORLD); // A 
				checkData[2] += JOBSIZE;
				activeThreads++;
			}
			
		}	
		// If no recorded found, pattern not present.
		if (found == -1)
		{
			sprintf(buf+strlen(buf), "%d %d %d\n", textIndex, patternIndex, -1);
		}
	}
	
	// All tests complete - tell slaves to stop
	int done = 1;
	MPI_Request request;
	int i;
	for (i = 1; i < numtasks; i++)
	{
		MPI_Isend(&done, 1, MPI_INT, i, 1, MPI_COMM_WORLD, &request); // X
	}
}

void slaveLoop(int rank, char** textData, char** patternData, int* textLength, int* patternLength)
{
	// Set up the receive for the flag from MASTER declaring the end
	int done = 0;
	int doneFlag = 0;
	MPI_Request request;
	MPI_Status status;
	MPI_Irecv(&done, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD, &request); // X
	
	while (!doneFlag) 
	{
		// Await new work from master
		MPI_Request r;
		MPI_Status s;
		int newWorkFlag = 0;
		int checkData[4];
		MPI_Irecv(&checkData, 4, MPI_INT, MASTER, 0, MPI_COMM_WORLD, &r); // A
		while(!newWorkFlag)
		{
			MPI_Test(&request, &doneFlag, &status); // X
			if (doneFlag)
				return;
		
			MPI_Test(&r, &newWorkFlag, &s); // A
		}
		
		int results[JOBSIZE];
		int numFound = checkTextForPatternAtPosition(textData[checkData[0]], patternData[checkData[1]], checkData[2], textLength[checkData[0]], patternLength[checkData[1]], checkData[3], results);
		
		MPI_Send(&results, JOBSIZE, MPI_INT, MASTER, 0, MPI_COMM_WORLD); // B
		
		// Check if master says we're done 
		MPI_Test(&request, &doneFlag, &status); // X
	}
}

int main(int argc, char**argv)
{
	// Read in the text pattern and control for each thread (aim of keeping bcasts etc to as few as possible)
	char *textData[MAXTEXTS];
	int textLength[MAXTEXTS];

	char *patternData[MAXPATTERNS];
	int patternLength[MAXPATTERNS];

	int controlData[800][3];
	
	int numberOfTests = readControl(controlData); 
	int numberOfTexts = readTexts(textData, textLength); 
	int numberOfPatterns = readPatterns(patternData, patternLength);
	
	int rank;
	int numtasks;
	
	char buf[BUFSIZE];
	sprintf(buf, "");
	
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		
	if (rank == MASTER)
	{
		masterLoop(numtasks, numberOfTests, controlData, textLength, patternLength, buf);
		writeOutputFile(buf);
	}
	else
	{
		slaveLoop(rank, textData, patternData, textLength, patternLength);
	}
		
	MPI_Finalize();
}