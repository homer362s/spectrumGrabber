#include <ansi_c.h>
#include <windows.h>

// This function assumes a fixed width field for all fields.
// Field length is the length of a single field including the comma
// numCols and numRows are the number of columns and rows before transposing
void transposeText(char *filename, int fieldLen, int numCols, int numRows)
{  
	long textLength = fieldLen * numCols * numRows;
	int nrows = 0, ncols = 0;
	
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
