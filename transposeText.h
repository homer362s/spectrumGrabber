#ifndef __transposeText_H__
#define __transposeText_H__

void transposeText(char *filename, int fieldLen, int numCols, int numRows);
void combineFiles(char *newFilename, char **filenames, char* sep, int nFiles, int fieldLen, unsigned long bufferLen);

#endif  /* ndef __transposeText_H__ */
