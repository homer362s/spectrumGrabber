#include <ansi_c.h>
#include <windows.h>

// This function assumes a fixed width field for all fields.
// Field length is the length of a single field including the comma
// numCols and numRows are the number of columns and rows before transposing
void transposeText(char *filename, int fieldLen, int numCols, int numRows)
{  
	long textLength = fieldLen * numCols * numRows;
	
	FILE *fp;
	FILE *tmpfp;
	
	// Determine file length
	fp = fopen(filename, "r");
	
	// Read entire file into memory
	char *text;
	text = malloc((textLength + 1) * sizeof(char));
	size_t nRead = fread(text, sizeof(char), textLength, fp);
	text[nRead] = 0;
	fclose(fp);
	
	// Open a temp file
	char tempFileName[MAX_PATH];
	GetTempFileName(".", "", 0, tempFileName);
	tmpfp = fopen(tempFileName, "w");
	
	char *field = malloc((fieldLen) * sizeof(char));
	field[fieldLen-1] = 0;
	int offset = 0;
	// Loop over writing the new rows to the file
	for(int i = 0;i<numCols;i++) {
		// Loop over each field in a column
		offset = 0 * numCols * fieldLen + i * fieldLen;
		memcpy(field, text + offset*sizeof(char), fieldLen-1);
		fprintf(tmpfp, "%s", field);
		for(int j = 1;j<numRows;j++) {
			offset = j * numCols * fieldLen + i * fieldLen;
			memcpy(field, text + offset*sizeof(char), fieldLen-1);
			fprintf(tmpfp, ",%s", field);
		}
		fprintf(tmpfp, "\n");
	}
	
	fclose(tmpfp);
	free(field);
	free(text);
	
	// Move the temp file to the old file location
	remove(filename);
	rename(tempFileName, filename);
}

void combineFiles(char *newFilename, char **filenames, char* sep, int nFiles, int fieldLen, unsigned long bufferLen)
{
	unsigned long bufferSize = bufferLen * (fieldLen + 1);
	char **bufferText = malloc(nFiles * sizeof(char**));
	long int *locs = malloc(nFiles * sizeof(long int));
	FILE *infp;
	FILE *outfp;
	// Allocate memory and set initial file locations
	outfp = fopen(newFilename, "w");
	for(int i = 0;i < nFiles;i++) {
		// If there is a file to read from make the buffer and open the file
		if(filenames[i][0]) {
			bufferText[i] = malloc((bufferSize + 1) * sizeof(char));
			locs[i] = 0;
		}
	}
	
	unsigned long buffsRead = 0;
	size_t nRead;
	size_t minRead = bufferSize;
	while(minRead == bufferSize) {
		// Read in the next buffers
		for(int i = 0;i < nFiles;i++) {
			if (filenames[i][0]) {
				infp = fopen(filenames[i], "r");
				fseek(infp, locs[i], SEEK_SET);
				nRead = fread(bufferText[i], sizeof(char), bufferSize, infp);
				locs[i] = ftell(infp);
				fclose(infp);
				bufferText[i][nRead] = 0;
				minRead = nRead < minRead ? nRead : minRead;
			}
		}
		buffsRead = buffsRead + 1;
		
		// Write to the file
		// Loop over buffer elements
		for(int i = 0;i < floor((double) minRead / (double) bufferSize * (double) bufferLen);i++) {
			// Loop over each input file
			int firstFieldWritten = FALSE;
			for(int j = 0;j < nFiles;j++) {
				if (filenames[j][0]) {
					bufferText[j][(i+1)*fieldLen + i] = 0;
					if (firstFieldWritten) {
						fprintf(outfp, ",%s", bufferText[j]+i*(fieldLen+1));
					} else {
						fprintf(outfp, "%s", bufferText[j]+i*(fieldLen+1));
						firstFieldWritten = TRUE;
					}
				} else {
					fprintf(outfp, ",");
				}
			}
			fprintf(outfp, "\n");
		}
	}
	
	// Free the buffers then close and delete the files
	fclose(outfp);
	free(locs);
	for(int i = 0;i < nFiles;i++) {
		if (filenames[i][0]) {
			free(bufferText[i]);
		}
	}
	free(bufferText);
}
