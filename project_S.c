#include <omp.h>
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

char *textData[MAXTEXTS];
int textLength[MAXTEXTS];

char *patternData[MAXPATTERNS];
int patternLength[MAXPATTERNS];

int controlData[800][3];

// File input/output

void outOfMemory()
{
	fprintf (stderr, "Out of memory\n");
	exit (0);
}

void readFromFile(FILE *f, char **data, int *length)
{
	int ch;
	int allocatedLength;
	char *result;
	int resultLength = 0;

	allocatedLength = 0;
	result = NULL;

	ch = fgetc(f);
	while (ch >= 0)
	{
		resultLength++;
		if (resultLength > allocatedLength)
		{
			allocatedLength += 10000;
			result = (char *)realloc(result, sizeof(char) * allocatedLength);
			if (result == NULL)
				outOfMemory();
		}
		result[resultLength-1] = ch;
		ch = fgetc(f);
	}
	*data = result;
	*length = resultLength;
}


int readTexts()
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


int readPatterns()
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

int readControl()
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

	sprintf (fileName, "result_S.txt");
	
	f = fopen(fileName, "a+");	
	if (f == NULL)
		return;
	fprintf(f, buf);
	fclose(f);
	
	buf[0] = '\0';
}

void writeToBuffer(char buf[], int textIndex, int patternIndex, int matchPoint)
{
	if (strlen(buf) > BUFSIZE - 20)
		writeOutputFile(buf);
	
	sprintf(buf+strlen(buf), "%d %d %d\n", textIndex, patternIndex, matchPoint);	
}

// Mode 0 - find the first occurrence of the pattern
int findFirstInstance(int textIndex, int patternIndex)
{
	int i,j,k, lastI;
	
	i=0;
	j=0;
	k=0;
	lastI = textLength[textIndex] - patternLength[patternIndex];
     
	while (i<=lastI && j<patternLength[patternIndex])
	{
		if (textData[textIndex][k] == patternData[patternIndex][j])
		{
			k++;
			j++;
		}
		else
		{
			i++;
			k=i;
			j=0;
		}
	}
	if (j == patternLength[patternIndex])
		return i;
	else
		return -1;
}

// Mode 1 - find all occurrences of the pattern
void findAllInstances(int textIndex, int patternIndex, char buf[])
{
	int i,j,k, lastI;
	int found = 0;
	i=0;
	j=0;
	k=0;
	lastI = textLength[textIndex] - patternLength[patternIndex];
     
	while (i<=lastI)
	{
		if (textData[textIndex][k] == patternData[patternIndex][j])
		{
			k++;
			j++;
			
			if (j == patternLength[patternIndex])
			{
				found = 1;
				writeToBuffer(buf, textIndex, patternIndex, i);
				
				i++;
				k=i;
				j=0;
			}
		}
		else
		{
			i++;
			k=i;
			j=0;
		}
	}
	
	if (!found)
		writeToBuffer(buf, textIndex, patternIndex, -1);

}

void processData(int mode, int textIndex, int patternIndex, char buf[])
{
	if (mode == 0)
		writeToBuffer(buf, textIndex, patternIndex, findFirstInstance(textIndex, patternIndex));
	else
		findAllInstances(textIndex, patternIndex, buf);

}

int main(int argc, char **argv)
{
	int numberOfTests = readControl();
	readTexts();
	readPatterns();
	
	char buf[BUFSIZE];
	sprintf(buf, "");
	
	int i;
	for (i = 0; i < numberOfTests; i++)
		processData(controlData[i][0], controlData[i][1], controlData[i][2], buf);
		
	writeOutputFile(buf);
}
