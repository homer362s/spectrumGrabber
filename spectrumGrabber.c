
#include "cbw.h" 
#include <cviauto.h>
#include <analysis.h>
#include <ansi_c.h>
#include <cvirte.h>		
#include <userint.h>
#include "spectrumGrabber.h"
#include "toolbox.h"
#include "ps6000Api.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

//#define measuredPoints 20000

int measuredPoints = 0;

uint32_t timebase = 7816;
int16_t *rawDataBuffer;   // Stores data from the scope
double *zeros;			  // Stores zeros for FFT
double *dataValues;		  // Stores specctrum data for a single measurement
double *avgSpectrum[4];	  // Stores the average spectrum magnitudes
double *avgSpectrumDisplay;	// Stores an average spectrum in display units (dBV)
float *timeValues;
float *freqValues;

int measurementInProgress = 0;
int dataReady = 0;
int userRequestedStop = 0;
int userRequestedNext = 0;

// Control arrays
int channelLeds[] = {MAINPANEL_LEDA, MAINPANEL_LEDB, MAINPANEL_LEDC, MAINPANEL_LEDD};
int channelRanges[] = {MAINPANEL_RANGEA, MAINPANEL_RANGEB, MAINPANEL_RANGEC, MAINPANEL_RANGED};
int channelCouplings[] = {MAINPANEL_COUPLINGA, MAINPANEL_COUPLINGB, MAINPANEL_COUPLINGC, MAINPANEL_COUPLINGD};
int channelCoeffs[] = {MAINPANEL_COEFFA, MAINPANEL_COEFFB, MAINPANEL_COEFFC, MAINPANEL_COEFFD};
int channelMeasFreq[] = {MAINPANEL_FREQA, MAINPANEL_FREQB, MAINPANEL_FREQC, MAINPANEL_FREQD};
int channelMeasTime[] = {MAINPANEL_TIMEA, MAINPANEL_TIMEB, MAINPANEL_TIMEC, MAINPANEL_TIMED};
char channelLabel[][2] = {"A", "B", "C", "D"};

int dacBoard;
int vgOut;
int vdOut;
static int panelHandle = 0;
int tgHandle = 0;
int16_t scopeHandle;

void picoscopeInit();
int getInputNew(char FileInput[], int *pointer, char **line);
char *fileread(char name[], char access[]);
void updateTimeAxis();
void setupScopeChannels();
int isEnabled(int handle, int control);
void gotoBiasPoint(int index);
void generateMatrix(double *VgVals, double *VdVals, int NumVgSteps, int NumVdSteps); 
void updateBiasCount();  

int main (int argc, char *argv[])
{
	int error = 0;
	float timeInterval_ns;
	
	/* initialize and load resources */
	nullChk (InitCVIRTE (0, argv, 0));
	errChk (panelHandle = LoadPanel (0, "spectrumGrabber.uir", MAINPANEL));
	
	// Allocate Memory
	GetCtrlVal(panelHandle, MAINPANEL_BINSRING, &measuredPoints);
	measuredPoints = measuredPoints * 2;
	
	rawDataBuffer = malloc(measuredPoints * sizeof(int16_t));
	zeros = malloc(measuredPoints * sizeof(double));
	dataValues = malloc(measuredPoints * sizeof(double));
	avgSpectrumDisplay = malloc(measuredPoints * sizeof(double)/2);
	timeValues = malloc(measuredPoints * sizeof(float));
	freqValues = malloc(measuredPoints * sizeof(float)/2);
	
	//avgSpectrum[0] = malloc(measuredPoints * sizeof(double)/2);
	for(int i =0;i<4;i++) {
		if(isEnabled(panelHandle, channelLeds[i]))
			avgSpectrum[i] = malloc(measuredPoints * sizeof(double)/2);
	}
	
	// Initialize picoscope
	picoscopeInit();
	ps6000GetTimebase2(scopeHandle, timebase, measuredPoints, &timeInterval_ns, 0, NULL, 0);
	SetCtrlVal(panelHandle, MAINPANEL_RATEBOX, 1/(timeInterval_ns*1e-9));
	
	// Initialize DAC stuff
	GetCtrlVal(panelHandle, MAINPANEL_BOARDNUM, &dacBoard);
	GetCtrlVal(panelHandle, MAINPANEL_VGNUM, &vgOut);
	GetCtrlVal(panelHandle, MAINPANEL_VDNUM, &vdOut);
	
	/* display the panel and run the user interface */
	errChk (DisplayPanel (panelHandle));
	errChk (RunUserInterface ());

Error:
	/* clean up */
	if (panelHandle > 0)
		DiscardPanel (panelHandle);
	return 0;
}


void picoscopeInit()
{
	int8_t *serial = (int8_t*) "AP231/007\0";

	PICO_STATUS status;
	
	status = ps6000OpenUnit(&scopeHandle, serial);

	switch (status) {
		case PICO_OK:
			break;
		default:
			break;
	}

	// Initialize some values
	updateTimeAxis();

	setupScopeChannels();
	
	// Set up data buffer
	ps6000MemorySegments(scopeHandle, 4, NULL);
	ps6000SetDataBuffer(scopeHandle, PS6000_CHANNEL_A, rawDataBuffer, measuredPoints, PS6000_RATIO_MODE_NONE);
	
	ps6000Stop(scopeHandle);
	
}

void setupScopeChannel(int scopeChannel, int enabledLed, int rangeRing, int couplingRing)
{
	// Get enabled status
	int enabled = 0;
	GetCtrlVal(panelHandle, enabledLed, &enabled);
	
	// Get Coupling
	int coupling = 0;
	GetCtrlVal(panelHandle, couplingRing, &coupling);
	switch (coupling) {
		case 1:
			coupling = PS6000_AC;
			break;
		case 2:
			coupling = PS6000_DC_1M;
			break;
	}
	
	// Get range
	double fullScale;
	int scaleSetting = 0;
	GetCtrlVal(panelHandle, rangeRing, &fullScale);
	switch ((int)(fullScale*1000)) {
		case 50:
			scaleSetting = PS6000_50MV;
			break;
		case 100:
			scaleSetting = PS6000_100MV;
			break;
		case 200:
			scaleSetting = PS6000_200MV;
			break;
		case 500:
			scaleSetting = PS6000_500MV;
			break;
		case 1000:
			scaleSetting = PS6000_1V;
			break;
		case 2000:
			scaleSetting = PS6000_2V;
			break;
		case 5000:
			scaleSetting = PS6000_5V;
			break;
		case 10000:
			scaleSetting = PS6000_10V;
			break;
		case 20000:
			scaleSetting = PS6000_20V;
			break;
	}
	
	// Set the channel
	ps6000SetChannel(scopeHandle, scopeChannel, (short) enabled, coupling, scaleSetting, 0, PS6000_BW_20MHZ);
}

void setupScopeChannels()
{

	// Set up data buffer
	ps6000SetDataBuffer(scopeHandle, PS6000_CHANNEL_A, rawDataBuffer, measuredPoints, PS6000_RATIO_MODE_NONE);
	
	
	setupScopeChannel(PS6000_CHANNEL_A, MAINPANEL_LEDA, MAINPANEL_RANGEA, MAINPANEL_COUPLINGA);
	setupScopeChannel(PS6000_CHANNEL_B, MAINPANEL_LEDA, MAINPANEL_RANGEA, MAINPANEL_COUPLINGA);
	setupScopeChannel(PS6000_CHANNEL_C, MAINPANEL_LEDA, MAINPANEL_RANGEA, MAINPANEL_COUPLINGA);
	setupScopeChannel(PS6000_CHANNEL_D, MAINPANEL_LEDA, MAINPANEL_RANGEA, MAINPANEL_COUPLINGA);
}

void PREF4 dataAvailableCallback(int16_t handle, PICO_STATUS status, void* pParameter)
{
	// Indicate that the measurement is done and therefore data is ready
	measurementInProgress = 0;
	dataReady = 1;
}

int CVICALLBACK avgBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			int runEnabled;
			short averages;
			GetCtrlAttribute(panelHandle, MAINPANEL_RUNBUTTON, ATTR_DIMMED, &runEnabled);
			if(!runEnabled){
				char tmpstr[128];
				GetCtrlVal(panelHandle, MAINPANEL_AVGBOX, &averages);
				sprintf(tmpstr, "%d/%d Averages", 0, averages);
				SetCtrlVal(panelHandle, MAINPANEL_AVGCOUNTDISP, tmpstr);
			}
		break;
	}
	return 0;
}

int CVICALLBACK dacButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			updateBiasCount();
		break;
	}
	return 0;
}

void updateBiasCount(){
	if(isEnabled(panelHandle, MAINPANEL_DACBUTTON)){
		int nBias;
		GetNumTableRows(panelHandle, MAINPANEL_TABLE, &nBias);
		char tmpstr[128];
		sprintf(tmpstr, "0/%d Bias Points", nBias);
		SetCtrlVal(panelHandle, MAINPANEL_BIASCOUNTDISP, tmpstr);	
	}else{
		SetCtrlVal(panelHandle, MAINPANEL_BIASCOUNTDISP, "Bias Disabled");  	
	}		
}	

int CVICALLBACK binsRing_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			//Reallocate Arrays
			GetCtrlVal(panelHandle, MAINPANEL_BINSRING, &measuredPoints);
			measuredPoints = measuredPoints * 2;
	
			free(rawDataBuffer);
			free(zeros);
			free(dataValues);
			free(avgSpectrumDisplay);
			free(timeValues);
			free(freqValues);
			
			rawDataBuffer = malloc(measuredPoints * sizeof(int16_t));
			zeros = malloc(measuredPoints * sizeof(double));
			dataValues = malloc(measuredPoints * sizeof(double));
			avgSpectrumDisplay = malloc(measuredPoints * sizeof(double)/2);
			timeValues = malloc(measuredPoints * sizeof(float));
			freqValues = malloc(measuredPoints * sizeof(float)/2);
			
			for(int i =0;i<4;i++) {
				free(avgSpectrum[i]);
				if(isEnabled(panelHandle, channelLeds[i]))
					avgSpectrum[i] = malloc(measuredPoints * sizeof(double)/2);
			}
			
			
			updateTimeAxis();
			
			// Update the data bufer
			ps6000SetDataBuffer(scopeHandle, PS6000_CHANNEL_A, rawDataBuffer, measuredPoints, PS6000_RATIO_MODE_NONE);
			break;
	}
	
	return 0;
}

void processData(int nMeasured, int averages, FILE **timeFPs)
{
	dataReady = 0;
	
	// Clear graph in preparation for new plots
	DeleteGraphPlot(panelHandle, MAINPANEL_FREQGRAPH, -1, VAL_DELAYED_DRAW); 
	
	// Loop over each scope input
	for(int i = 0;i < 4;i++) {
		// Skip if the input is disabled
		if(!isEnabled(panelHandle, channelLeds[i]))
			continue;
		
		// Get data from scope
		uint32_t nPoints = measuredPoints;
		ps6000GetValues(scopeHandle, 0, &nPoints, 1, PS6000_RATIO_MODE_NONE, i, NULL);
		
		// Convert values to double
		double fullScale;
		GetCtrlVal(panelHandle, channelRanges[i], &fullScale);
		double coefficient;
		GetCtrlVal(panelHandle, channelCoeffs[i], &coefficient);
		for (int j = 0;j < measuredPoints;j++) {
			dataValues[j] = (double) rawDataBuffer[j] / 32767 * fullScale / coefficient;
		}
	
		// Save timeValues if time domain saving is requested and this is the first sweep at this bias point
		if (isEnabled(panelHandle, channelMeasTime[i]) && nMeasured==0) {
			// Save time domain signal
			fprintf(timeFPs[i], "\n%e", dataValues[0]);

			for(int j=1; j<measuredPoints; j++) {
				fprintf(timeFPs[j], ",%e", dataValues[j]);	
			}
		}

		// Compute FFT if the FFT is requested
		if (isEnabled(panelHandle, channelMeasFreq[i])) {
			// Take FFT
			for(int j = 0;j < measuredPoints; j++) {
				zeros[j] = 0;
			}
			FFT(dataValues, zeros, measuredPoints);

			// From now on we only care about the first half of the points
			for(int j = 0;j < measuredPoints/2; j++) {
				// Get magnitude
				dataValues[j] = (sqrt(dataValues[j]*dataValues[j] + zeros[j]*zeros[j])) / measuredPoints;

				// Average spectra
				avgSpectrum[i][j] = (avgSpectrum[i][j] * ((double) nMeasured / (double) averages) + dataValues[j] * (1 / (double) averages)) * ((double) averages) / ((double) nMeasured + 1);
		
				// Display units
				avgSpectrumDisplay[j] = 20*log10(avgSpectrum[i][j]);
			}
		
			// Display the data
			PlotXY(panelHandle, MAINPANEL_FREQGRAPH, freqValues, avgSpectrumDisplay, measuredPoints/2, VAL_FLOAT, VAL_DOUBLE, VAL_THIN_LINE, VAL_NO_POINT, VAL_SOLID, 1, VAL_BLACK);
		}
	}
}

void handleMeasurement(char *path, char *name, char *ext)
{
	FILE *freqFPs[4] = {0, 0, 0, 0};
	FILE *timeFPs[4] = {0, 0, 0, 0};
	PICO_STATUS status;
	
	// Disable the run button
	SetCtrlAttribute(panelHandle, MAINPANEL_STOPBUTTON, ATTR_DIMMED, FALSE);
	SetCtrlAttribute(panelHandle, MAINPANEL_RUNBUTTON, ATTR_DIMMED, TRUE);
	SetCtrlAttribute(panelHandle, MAINPANEL_NEXTBUTTON, ATTR_DIMMED, FALSE);
	SetCtrlAttribute(panelHandle, MAINPANEL_BINSRING, ATTR_DIMMED, TRUE);
	SetCtrlAttribute(panelHandle, MAINPANEL_RATEBOX, ATTR_DIMMED, TRUE);
	
	userRequestedStop = 0;
	
	int dacEnabled = 0;
	int nBias = 1;
	GetCtrlVal(panelHandle, MAINPANEL_DACBUTTON, &dacEnabled);
	
	if (dacEnabled)
		GetNumTableRows(panelHandle, MAINPANEL_TABLE, &nBias);
	
	char *outputFileFreq = NULL;
	char *outputFileTime = NULL;
	int filenameLen = strlen(path) + strlen(name) + 5 + 2 + strlen(ext) + 1;
	
	// Store frequency and time axes in the files
	for(int i = 0;i<4;i++) {
		// Skip if this channel is neither measurement is requested
		if (!(isEnabled(panelHandle, channelMeasFreq[i]) || isEnabled(panelHandle, channelMeasTime[i])))
			continue;

		// Save frequency row
		if (isEnabled(panelHandle, channelMeasFreq[i])) {
			// Build filename and open file
			outputFileFreq = malloc(filenameLen * sizeof(char));
			strcpy(outputFileFreq, path);
			strcat(outputFileFreq, name);
			strcat(outputFileFreq, "_Freq_");
			strcat(outputFileFreq, channelLabel[i]);
			strcat(outputFileFreq, ext);
			freqFPs[i] = fopen(outputFileFreq, "w+");
			free(outputFileFreq);
			outputFileFreq = NULL;
	
			// Write data
			fprintf(freqFPs[i], "%e", freqValues[0]);
			for(int j=1; j<measuredPoints/2; j++) {
				fprintf(freqFPs[i], ",%e", freqValues[j]);	
			}
		}
	
		// Save time row
		if (isEnabled(panelHandle, channelMeasTime[i])) {
			// Build filename and open file
			outputFileTime = malloc(filenameLen * sizeof(char));
			strcpy(outputFileTime, path);
			strcat(outputFileTime, name);
			strcat(outputFileTime, "_Time_");
			strcat(outputFileTime, channelLabel[i]);
			strcat(outputFileTime, ext);
			timeFPs[i] = fopen(outputFileTime, "w+");
			free(outputFileTime);
			outputFileTime = NULL;
	
			// Write data
			fprintf(timeFPs[i], "%e", timeValues[0]);
			for(int j=1; j<measuredPoints; j++) {
				fprintf(timeFPs[i], ",%e", timeValues[j]);	
			}
		}
	}
	
	// Loop over bias conditions measuring at each
	for(int i = 0; i < nBias; i++) {
		short averages = 1;
		int nMeasured = 0;
		char tmpstr[128];

		// Get number of requested averages
		GetCtrlVal(panelHandle, MAINPANEL_AVGBOX, &averages);
	
		sprintf(tmpstr, "0/%d Completed", averages);
		SetCtrlVal(panelHandle, MAINPANEL_AVGCOUNTDISP, tmpstr);
		
		// Go to bias point `i` in table
		gotoBiasPoint(i);
		
		double delay;
		GetCtrlVal(panelHandle, MAINPANEL_DELAYBOX, &delay);
		Delay(delay);
		
		// Loop over a single bias condition and average
		for(nMeasured = 0; nMeasured < averages; nMeasured++) {
			if (userRequestedStop)
				break;
			if(userRequestedNext)
				break;
		
			measurementInProgress = 1;
			dataReady = 0;
			status = ps6000RunBlock(scopeHandle, measuredPoints/2, measuredPoints/2, timebase, 1, 0, 0, dataAvailableCallback, 0);

			switch (status) {
				case PICO_OK:
					break;
				default:
					break;
			}
	
			while(measurementInProgress) {
				// Handle events
				ProcessSystemEvents();
				if (userRequestedStop) {
					// Update progress counter
					sprintf(tmpstr, "%d/%d Completed", nMeasured, nMeasured);
					SetCtrlVal(panelHandle, MAINPANEL_AVGCOUNTDISP, tmpstr);
					break;
				}
				
				if(userRequestedNext) 
					break;
			}
	
			if(userRequestedStop)
				break;
			if(userRequestedNext)
				break;
		
			// Load and process data into appropriate arrays
			if (dataReady)
				processData(nMeasured, averages, timeFPs);
		
			// Update progress counter
			GetCtrlVal(panelHandle, MAINPANEL_AVGBOX, &averages);
			sprintf(tmpstr, "%d/%d Averages", nMeasured + 1, averages);
			SetCtrlVal(panelHandle, MAINPANEL_AVGCOUNTDISP, tmpstr);
			
		}
		
		// Save average spectrum
		fprintf(freqFPs[0], "\n%e", avgSpectrum[0]);
	
		for(int i=1; i<measuredPoints/2; i++) {
			fprintf(freqFPs[0], ",%e", avgSpectrum[i]);	
		}
		
		if(userRequestedNext){
			userRequestedNext = 0;
			// Tell the scope to stop
			ps6000Stop(scopeHandle);
			measurementInProgress = 0;
		}	
		
		if (userRequestedStop)
			break;
		
		// Check if the number of bias points changed 
		if (dacEnabled)
			GetNumTableRows(panelHandle, MAINPANEL_TABLE, &nBias);
	}
	
	// Tell the scope to stop
	ps6000Stop(scopeHandle);
	measurementInProgress = 0;
	
	// Close data files
	for(int i = 0;i<4;i++) {
		if (freqFPs[i])
			fclose(freqFPs[i]);
		if (timeFPs[i])
			fclose(timeFPs[i]);
	}
	
	// Re-enable the run button
	SetCtrlAttribute(panelHandle, MAINPANEL_STOPBUTTON, ATTR_DIMMED, TRUE);
	SetCtrlAttribute(panelHandle, MAINPANEL_RUNBUTTON, ATTR_DIMMED, FALSE);
	SetCtrlAttribute(panelHandle, MAINPANEL_NEXTBUTTON, ATTR_DIMMED, TRUE);
	SetCtrlAttribute(panelHandle, MAINPANEL_BINSRING, ATTR_DIMMED, FALSE);
	SetCtrlAttribute(panelHandle, MAINPANEL_RATEBOX, ATTR_DIMMED, FALSE);
}

// Split up a filename into the corresponding parts
void fileparts(char *fullfile, char **path, char **name, char **ext)
{
	//find the length of fullfile
	int fullfileLen = strlen(fullfile);
	int pathlen = 0;
	int namelen = 0;
	int extlen = 0;
	
	//loop to find the last "/" in fullfile to find path
	int i;
	for(i=fullfileLen-1; i>=0; i--){
		if(fullfile[i] == '\\'){
			fullfile[i] = 0;
			break;
		}	
	}
	
	//find path length
	pathlen = i;
	
	//loop to find last "." in fullfile to find ext (if exists)
	for(i=fullfileLen-1; i>=pathlen; i--){
		if(fullfile[i] == '.'){
			fullfile[i] = 0;
			break;
		}	
	}
	
	//find ext length
	extlen = fullfileLen - i - 1;
	
	//find name length
	namelen = fullfileLen - pathlen - extlen - 1;
	
	//create arrays
	*path = malloc((pathlen+2)*sizeof(char));
	strcpy(*path, fullfile);
	strcat(*path, "\\");
	
	*name = malloc((namelen+1)*sizeof(char));
	strcpy(*name, fullfile+pathlen+1);
	
	*ext = malloc((extlen+2)*sizeof(char));
	strcpy(*ext, ".");
	strcat(*ext, fullfile+pathlen+1+namelen);
}

void gotoBiasPoint(int index)
{
	double Vg, Vd;
	double VgCoeff, VdCoeff;
	if (isEnabled(panelHandle, MAINPANEL_DACBUTTON)) {
		
		GetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(1,index+1), &Vg);
		GetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(2,index+1), &Vd);
		GetCtrlVal(panelHandle, MAINPANEL_VGCOEFFBOX, &VgCoeff);
		GetCtrlVal(panelHandle, MAINPANEL_VDCOEFFBOX, &VdCoeff);
		
		SetActiveTableCell(panelHandle, MAINPANEL_TABLE, MakePoint(1,index+1));
		
		// Highlight background of bias in progress
		int nBias;
		GetNumTableRows(panelHandle, MAINPANEL_TABLE, &nBias);
		if(index==0) {
			SetTableCellRangeAttribute(panelHandle, MAINPANEL_TABLE, MakeRect(1,1,nBias,2), ATTR_TEXT_BGCOLOR, VAL_WHITE);   
		} else {
			SetTableCellRangeAttribute(panelHandle, MAINPANEL_TABLE, VAL_TABLE_ROW_RANGE(index), ATTR_TEXT_BGCOLOR, VAL_WHITE); 
		}
		SetTableCellRangeAttribute(panelHandle, MAINPANEL_TABLE, VAL_TABLE_ROW_RANGE(index+1), ATTR_TEXT_BGCOLOR, VAL_PANEL_GRAY);
		
		cbVOut(dacBoard, vgOut, BIP10VOLTS, Vg/VgCoeff*0.001, 0);
		cbVOut(dacBoard, vdOut, BIP10VOLTS, Vd/VdCoeff*0.001, 0);
	 }
}

int isEnabled(int panel, int control)
{
	int enabled;
	GetCtrlVal(panel, control, &enabled);
	return enabled;
}

void loadConditions(){
	char biasFilename[MAX_PATHNAME_LEN];
	int selectionStatus = FileSelectPopup("", "*.cfg", "*.cfg; *.csv; *.*", "Select Bias Conditions", VAL_LOAD_BUTTON, 0, 0, 1, 1, biasFilename); 

	if(selectionStatus == VAL_NO_FILE_SELECTED){
		return;		
	}
	
	char *record = NULL, *line = NULL, *InputFile = NULL;
	int filepointer = 0, lineLength = 0;
	
	DeleteTableRows(panelHandle, MAINPANEL_TABLE, 1, -1);
	
	int rowCount = 0;
	InputFile = fileread(biasFilename, "r");
	
	double *VgCol, *VdCol;
	int arrayLength = 100;
	VgCol =  malloc(arrayLength*sizeof(double));
	VdCol =  malloc(arrayLength*sizeof(double));  
	
	while((lineLength = getInputNew(InputFile, &filepointer, &line)) != 0){
		if (lineLength == EOF)
			break;
		
		if(rowCount+1 > arrayLength){
			arrayLength = arrayLength*2;
			VgCol = realloc(VgCol, arrayLength*sizeof(double));
			VdCol = realloc(VdCol, arrayLength*sizeof(double));
		}
		
		record = strtok(line, ",");
		VgCol[rowCount] = (double) atof(record);
		
		record = strtok(NULL, ",");
		VdCol[rowCount] = (double) atof(record); 
		
		rowCount++;
	}
	
	InsertTableRows(panelHandle, MAINPANEL_TABLE, -1, rowCount, VAL_USE_MASTER_CELL_TYPE);
	SetTableCellRangeVals(panelHandle, MAINPANEL_TABLE, MakeRect(1,1,rowCount, 1), VgCol, VAL_COLUMN_MAJOR);
	SetTableCellRangeVals(panelHandle, MAINPANEL_TABLE, MakeRect(1,2,rowCount, 1), VdCol, VAL_COLUMN_MAJOR);
	
	free(VgCol);
	free(VdCol);
	free(line);
	free(InputFile);
	InputFile = NULL;
}

void buildTable(){
	int NumVgSteps, NumVdSteps; 
	double VgStart, VgStop, VgStepSize;
	double VdStart, VdStop, VdStepSize;
	 
	//get start, stop, and step size values from boxes
	GetCtrlVal(tgHandle, TGPANEL_VGSTARTBOX, &VgStart);  
	GetCtrlVal(tgHandle, TGPANEL_VGSTOPBOX, &VgStop);
	GetCtrlVal(tgHandle, TGPANEL_NUMVGSTEPBOX, &NumVgSteps);
	GetCtrlVal(tgHandle, TGPANEL_VGSTEPSIZEBOX, &VgStepSize);
	
	GetCtrlVal(tgHandle, TGPANEL_VDSTARTBOX, &VdStart);  
	GetCtrlVal(tgHandle, TGPANEL_VDSTOPBOX, &VdStop);
	GetCtrlVal(tgHandle, TGPANEL_NUMVDSTEPBOX, &NumVdSteps);
	GetCtrlVal(tgHandle, TGPANEL_VDSTEPSIZEBOX, &VdStepSize);

	//build array of Vg and Vd
	double VgVals[NumVgSteps+1];
	double VdVals[NumVdSteps+1];
	
	VgVals[0] = VgStart;
	VdVals[0] = VdStart;
	
	for(int k=1; k<NumVgSteps; k++){
		VgVals[k] = VgVals[k-1] + VgStepSize;		
	}
	
	for(int k=1; k<NumVdSteps; k++){
		VdVals[k] = VdVals[k-1] + VdStepSize;		
	}
	
	//get table option
	int ringIndex;
	GetCtrlVal(tgHandle, TGPANEL_GENERATERING, &ringIndex);
	
	//clear table
	DeleteTableRows(panelHandle, MAINPANEL_TABLE, 1, -1);
	
	//build correct table
	switch(ringIndex){
		case 1:	//Matrix
			generateMatrix(VgVals, VdVals, NumVgSteps, NumVdSteps);
			break;
		case 2:	//Vg Sweep
			generateMatrix(VgVals, VdVals, NumVgSteps, NumVdSteps);
			break;
		case 3: //Vd Sweep
			generateMatrix(VgVals, VdVals, NumVgSteps, NumVdSteps);
			break;
		case 4:	//Arbitrary Line
			InsertTableRows(panelHandle, MAINPANEL_TABLE, -1, NumVgSteps, VAL_USE_MASTER_CELL_TYPE);
			SetTableCellRangeVals(panelHandle, MAINPANEL_TABLE, MakeRect(1,1,NumVgSteps, 1), VgVals, VAL_COLUMN_MAJOR);
			SetTableCellRangeVals(panelHandle, MAINPANEL_TABLE, MakeRect(1,2,NumVgSteps, 1), VdVals, VAL_COLUMN_MAJOR);
			break;
	}	
}	

void calculateVgStepSize() {
	int NumVgSteps; 
	double VgStart, VgStop, VgStepSize;

	//Get start, stop, and number of steps from textboxes
	GetCtrlVal(tgHandle, TGPANEL_VGSTARTBOX, &VgStart);  
	GetCtrlVal(tgHandle, TGPANEL_VGSTOPBOX, &VgStop);
	GetCtrlVal(tgHandle, TGPANEL_NUMVGSTEPBOX, &NumVgSteps);

	//Calclate and set step size
	if(NumVgSteps != 1) {
		VgStepSize = (VgStop - VgStart)/(NumVgSteps - 1);
		SetCtrlVal(tgHandle, TGPANEL_VGSTEPSIZEBOX, VgStepSize);
	}
	else {
		SetCtrlVal(tgHandle, TGPANEL_VGSTEPSIZEBOX, 0.00);
	}	
}


void calculateVdStepSize() {
	int NumVdSteps;
	double VdStart, VdStop, VdStepSize;

	//Get start, stop, and number of steps from textboxes 
	GetCtrlVal(tgHandle, TGPANEL_VDSTARTBOX, &VdStart);  
	GetCtrlVal(tgHandle, TGPANEL_VDSTOPBOX, &VdStop);
	GetCtrlVal(tgHandle, TGPANEL_NUMVDSTEPBOX, &NumVdSteps);

	//Calclate and set step size
	if(NumVdSteps != 1) {
		VdStepSize = (VdStop - VdStart)/(NumVdSteps - 1);
		SetCtrlVal(tgHandle, TGPANEL_VDSTEPSIZEBOX, VdStepSize);
	}
	else {
		SetCtrlVal(tgHandle, TGPANEL_VDSTEPSIZEBOX, 0.00);
	}	
}

void saveTable(char *savetableFilename) {
	double Vg, Vd; 
	int nBias;
	GetNumTableRows(panelHandle, MAINPANEL_TABLE, &nBias);
	
	FILE *tablefp = fopen(savetableFilename, "w+");   
												  //MakePoint(col,row)  
	GetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(1,1), &Vg);
	GetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(2,1), &Vd);
	
	fprintf(tablefp, "%e", Vg);
	fprintf(tablefp, ",%e", Vd);
	
	for(int i=1; i<nBias; i++) {
		GetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(1,i+1), &Vg);
		GetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(2,i+1), &Vd);
		
		fprintf(tablefp, "\n%e", Vg);
		fprintf(tablefp, ",%e", Vd);	
	}
	
	fclose(tablefp); 
}	

void delRows(){
	Rect cellrange;
	Point activecell;
	
	GetTableSelection(panelHandle, MAINPANEL_TABLE, &cellrange); 
	
	if(cellrange.height == 0) {
		GetActiveTableCell(panelHandle, MAINPANEL_TABLE, &activecell);
		DeleteTableRows(panelHandle, MAINPANEL_TABLE, activecell.y, 1);
	}else {
		DeleteTableRows(panelHandle, MAINPANEL_TABLE, cellrange.top, cellrange.height);
	}
}	

void updateTimeAxis()
{
	float timeInterval_ns = 0;
	ps6000GetTimebase2(scopeHandle, timebase, measuredPoints, &timeInterval_ns, 0, NULL, 0);
	int time = timeInterval_ns;
	for (int i = 0;i < measuredPoints/2;i++) {
		timeValues[i] = (time-timeInterval_ns)/1e9;
		freqValues[i] = i/(measuredPoints*timeInterval_ns/1e9);
		time += timeInterval_ns;
	}
	for (int i = measuredPoints/2;i < measuredPoints;i++) {
		timeValues[i] = time/1e9;
		time += timeInterval_ns;  
	}
}

int CVICALLBACK led_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			printf("click");
			break;
	}	
	return 0;
}

int CVICALLBACK loadButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			loadConditions();
			SetCtrlVal(panelHandle, MAINPANEL_DACBUTTON, 1);
			updateBiasCount();
			break;
	}	
	return 0;
}

int CVICALLBACK channel_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			int channel = 0;
			int channelIndex = 0;
			switch (control) {
				case MAINPANEL_RANGEA:
				case MAINPANEL_COUPLINGA:
				case MAINPANEL_FREQA:
				case MAINPANEL_TIMEA:
					channel = PS6000_CHANNEL_A;
					channelIndex = 0;
					break;
				case MAINPANEL_RANGEB:
				case MAINPANEL_COUPLINGB:
				case MAINPANEL_FREQB:
				case MAINPANEL_TIMEB:
					channel = PS6000_CHANNEL_B;
					channelIndex = 1;
					break;
				case MAINPANEL_RANGEC:
				case MAINPANEL_COUPLINGC:
				case MAINPANEL_FREQC:
				case MAINPANEL_TIMEC:
					channel = PS6000_CHANNEL_C;
					channelIndex = 2;
					break;
				case MAINPANEL_RANGED:
				case MAINPANEL_COUPLINGD:
				case MAINPANEL_FREQD:
				case MAINPANEL_TIMED:
					channel = PS6000_CHANNEL_D;
					channelIndex = 3;
					break;
			}
			
			if (isEnabled(panelHandle, channelMeasFreq[channelIndex]) || isEnabled(panelHandle, channelMeasTime[channelIndex])) {
				// Enable the current channel
				SetCtrlVal(panelHandle, channelLeds[channelIndex], 1);
				SetCtrlAttribute(panelHandle, channelRanges[channelIndex], ATTR_DIMMED, 0);
				SetCtrlAttribute(panelHandle, channelCouplings[channelIndex], ATTR_DIMMED, 0);
				SetCtrlAttribute(panelHandle, channelCoeffs[channelIndex], ATTR_DIMMED, 0);
			}
			else {
				// Disable the current channel
				SetCtrlVal(panelHandle, channelLeds[channelIndex], 0);
				SetCtrlAttribute(panelHandle, channelRanges[channelIndex], ATTR_DIMMED, 1);
				SetCtrlAttribute(panelHandle, channelCouplings[channelIndex], ATTR_DIMMED, 1);
				SetCtrlAttribute(panelHandle, channelCoeffs[channelIndex], ATTR_DIMMED, 1);
			}
			
			setupScopeChannel(channel, channelLeds[channelIndex], channelRanges[channelIndex], channelCouplings[channelIndex]);
			break;
	}	
	return 0;
}

int CVICALLBACK clearButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			DeleteTableRows(panelHandle, MAINPANEL_TABLE, 1, -1);
			InsertTableRows(panelHandle, MAINPANEL_TABLE, -1, 1, VAL_USE_MASTER_CELL_TYPE);	
			SetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(1,1), 0.0);
			SetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(2,1), 0.0);
			updateBiasCount(); 
			break;
	}	
	return 0;
}

int CVICALLBACK addrowButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			InsertTableRows(panelHandle, MAINPANEL_TABLE, -1, 1, VAL_USE_MASTER_CELL_TYPE);	
			updateBiasCount();
			break;
	}	
	return 0;
}

int CVICALLBACK boardNum_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			GetCtrlVal(panelHandle, MAINPANEL_BOARDNUM, &dacBoard);
			GetCtrlVal(panelHandle, MAINPANEL_VGNUM, &vgOut);
			GetCtrlVal(panelHandle, MAINPANEL_VDNUM, &vdOut);
			break;
	}	
	return 0;
}

int CVICALLBACK delrowButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			delRows();
			updateBiasCount();  
			break;
	}	
	return 0;
}


int CVICALLBACK nextButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			userRequestedNext = 1;
			break;
	}	
	return 0;
}

int CVICALLBACK rateBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	double sampleRate;
	float timeInterval_ns;
	switch(event){
		case EVENT_COMMIT:
			GetCtrlVal(panelHandle, MAINPANEL_RATEBOX, &sampleRate);
			timebase = RoundRealToNearestInteger(156250000/sampleRate + 4);
			ps6000GetTimebase2(scopeHandle, timebase,measuredPoints, &timeInterval_ns, 0, NULL, 0);
			sampleRate = 1/(timeInterval_ns*1e-9);
			SetCtrlVal(panelHandle, MAINPANEL_RATEBOX, sampleRate);
			updateTimeAxis();
			break;
	}	
	return 0;
}

int CVICALLBACK runButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			char saveFilename[MAX_PATHNAME_LEN];
			int status = FileSelectPopup("", "*.csv", "*.csv;*.*", "Select save file", VAL_SAVE_BUTTON, 0, 0, 1, 1, saveFilename);
			if (status) {
				SetCtrlVal(panelHandle, MAINPANEL_FILEPREFIX, saveFilename);
				
				// Get filename
				char *path, *name, *ext;
				fileparts(saveFilename, &path, &name, &ext);
				char *saveFilename = malloc((strlen(path) + strlen(name) + 4 + 1) * sizeof(char));
				
				strcpy(saveFilename, path);
				strcat(saveFilename, name);
				strcat(saveFilename, ".cfg");
				
				saveTable(saveFilename);
				
				handleMeasurement(path, name, ext);
				
				free(path);
				free(name);
				free(ext);
				free(saveFilename);
			}
			break;
	}
	
	return 0;
}

int CVICALLBACK savetableButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			char savetableFilename[512];
			int status = FileSelectPopup("", "*.cfg", "*.cfg;*.*", "Select save file", VAL_SAVE_BUTTON, 0, 0, 1, 1, savetableFilename);
			if (status)
				saveTable(savetableFilename);
			break;
	}
	
	return 0;
}

int CVICALLBACK stopButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			userRequestedStop = 1;
			break;
	}
	
	return 0;
}

int CVICALLBACK tgPanel_CB (int panel, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_CLOSE:
			DiscardPanel(tgHandle);
			tgHandle = 0;
			break;
		case EVENT_KEYPRESS:
			if (eventData1 == VAL_ESC_VKEY) {
				HidePanel(tgHandle);
			}
			break;
	}
	
	return 0;
}

int CVICALLBACK buildtableButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			buildTable();
			updateBiasCount();  
			break;
	}	
	return 0;
}

int CVICALLBACK editVdBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_VAL_CHANGED:
			int ringIndex;
			GetCtrlVal(tgHandle, TGPANEL_GENERATERING, &ringIndex);
			
			switch(ringIndex){															
				case 1:	//Matrix
					calculateVdStepSize(); 
					break;
				case 2:	//Vg Sweep
					double vdstart;
					GetCtrlVal(tgHandle, TGPANEL_VDSTARTBOX, &vdstart);
					SetCtrlVal(tgHandle, TGPANEL_VDSTOPBOX, vdstart);
					SetCtrlVal(tgHandle, TGPANEL_NUMVDSTEPBOX, 1);
					calculateVdStepSize(); 
					break;
				case 3: //Vd Sweep
					calculateVdStepSize(); 
					break;
				case 4:	//Arbitrary Line
					calculateVdStepSize(); 
					break;
			}	
			break;
	}	
	return 0;
}

int CVICALLBACK editVgBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_VAL_CHANGED:
			int ringIndex;
			GetCtrlVal(tgHandle, TGPANEL_GENERATERING, &ringIndex);
			
			switch(ringIndex){															
				case 1:	//Matrix
					calculateVgStepSize(); 
					break;
				case 2:	//Vg Sweep
					calculateVgStepSize(); 
					break;
				case 3: //Vd Sweep
					double vgstart;
					GetCtrlVal(tgHandle, TGPANEL_VGSTARTBOX, &vgstart);
					SetCtrlVal(tgHandle, TGPANEL_VGSTOPBOX, vgstart);
					SetCtrlVal(tgHandle, TGPANEL_NUMVGSTEPBOX, 1);
					calculateVgStepSize(); 
					break;
				case 4:	//Arbitrary Line
					int vgstep;
					GetCtrlVal(tgHandle, TGPANEL_NUMVGSTEPBOX, &vgstep);
					SetCtrlVal(tgHandle, TGPANEL_NUMVDSTEPBOX, vgstep);
					calculateVgStepSize();
					calculateVdStepSize();
					break;
			}	
			break;
	}	
	return 0;
}

int CVICALLBACK generateButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			if (!tgHandle)
				tgHandle = LoadPanel(panelHandle, "spectrumGrabber.uir", TGPANEL);
			DisplayPanel(tgHandle);
			break;
	}	
	return 0;
}

void generateMatrix(double *VgVals, double *VdVals, int NumVgSteps, int NumVdSteps)
{
	//loop through arrays and construct table
	int rowCount = 0;
	double *VgCol, *VdCol;
	VgCol =  malloc((NumVgSteps*NumVdSteps)*sizeof(double));
	VdCol =  malloc((NumVgSteps*NumVdSteps)*sizeof(double)); 

	InsertTableRows(panelHandle, MAINPANEL_TABLE, -1, NumVgSteps*NumVdSteps, VAL_USE_MASTER_CELL_TYPE);     
	
	for(int i=0; i<NumVgSteps; i++){
		for(int	j=0; j<NumVdSteps; j++){
			VgCol[rowCount] = VgVals[i];
			VdCol[rowCount] = VdVals[j];
			
			rowCount++;
		}	
	}
	
	SetTableCellRangeVals(panelHandle, MAINPANEL_TABLE, MakeRect(1,1,NumVgSteps*NumVdSteps, 1), VgCol, VAL_COLUMN_MAJOR);
	SetTableCellRangeVals(panelHandle, MAINPANEL_TABLE, MakeRect(1,2,NumVgSteps*NumVdSteps, 1), VdCol, VAL_COLUMN_MAJOR);
	
	free(VgCol);
	free(VdCol);
}	

int CVICALLBACK generateRing_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			int ringIndex;
			GetCtrlVal(tgHandle, TGPANEL_GENERATERING, &ringIndex);
			
			//Enable all options
			SetCtrlAttribute(tgHandle, TGPANEL_VGSTARTBOX, ATTR_DIMMED, FALSE); 
			SetCtrlAttribute(tgHandle, TGPANEL_VGSTOPBOX, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(tgHandle, TGPANEL_NUMVGSTEPBOX, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(tgHandle, TGPANEL_VGSTEPSIZEBOX, ATTR_DIMMED, FALSE);    
		
			SetCtrlAttribute(tgHandle, TGPANEL_VDSTARTBOX, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(tgHandle, TGPANEL_VDSTOPBOX, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(tgHandle, TGPANEL_NUMVDSTEPBOX, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(tgHandle, TGPANEL_VDSTEPSIZEBOX, ATTR_DIMMED, FALSE);
			
			switch(ringIndex){															
				case 1:	//Matrix
					break;
				case 2:	//Vg Sweep
					SetCtrlAttribute(tgHandle, TGPANEL_VDSTOPBOX, ATTR_DIMMED, TRUE);
					SetCtrlAttribute(tgHandle, TGPANEL_NUMVDSTEPBOX, ATTR_DIMMED, TRUE);
					SetCtrlAttribute(tgHandle, TGPANEL_VDSTEPSIZEBOX, ATTR_DIMMED, TRUE);
					
					double vdstart;
					GetCtrlVal(tgHandle, TGPANEL_VDSTARTBOX, &vdstart);
					SetCtrlVal(tgHandle, TGPANEL_VDSTOPBOX, vdstart);
					SetCtrlVal(tgHandle, TGPANEL_NUMVDSTEPBOX, 1);
					calculateVdStepSize(); 
					break;
				case 3: //Vd Sweep
					SetCtrlAttribute(tgHandle, TGPANEL_VGSTOPBOX, ATTR_DIMMED, TRUE);
					SetCtrlAttribute(tgHandle, TGPANEL_NUMVGSTEPBOX, ATTR_DIMMED, TRUE);
					SetCtrlAttribute(tgHandle, TGPANEL_VGSTEPSIZEBOX, ATTR_DIMMED, TRUE); 
					
					double vgstart;
					GetCtrlVal(tgHandle, TGPANEL_VGSTARTBOX, &vgstart);
					SetCtrlVal(tgHandle, TGPANEL_VGSTOPBOX, vgstart);
					SetCtrlVal(tgHandle, TGPANEL_NUMVGSTEPBOX, 1);
					calculateVgStepSize();
					break;
				case 4:	//Arbitrary Line
					SetCtrlAttribute(tgHandle, TGPANEL_NUMVDSTEPBOX, ATTR_DIMMED, TRUE);
					
					int vgstep;
					GetCtrlVal(tgHandle, TGPANEL_NUMVGSTEPBOX, &vgstep);
					SetCtrlVal(tgHandle, TGPANEL_NUMVDSTEPBOX, vgstep);
					calculateVgStepSize();
					calculateVdStepSize();
					break;
			}	
		break;
	}	
	return 0;
}

int CVICALLBACK mainpanel_CB (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	if (event == EVENT_CLOSE) {
		userRequestedStop = 1;
		ps6000CloseUnit(scopeHandle);
		QuitUserInterface (0);
	}
	return 0;
}

int getInputNew(char FileInput[], int *pointer, char **line) {
    
    // Fuction to get a line of data from a file
    
    int length,c,i,start;
    
    if (line[0] == '\0' && (*pointer) > 0)
        return EOF;
    
    
    if ((*line) != NULL) {
        free((*line));  // If line is in use, free the space
        *line = NULL;
    }
    
    start = *pointer;
    c = FileInput[*pointer];
    if (c=='\0') {
        ;
    }
    i=0;
    while (c != '\0' && c != '\n' && c!= '\r') {  //increment pointer to find end of line
        (*pointer)++;
        i++;
        c = FileInput[*pointer];
    }
    
    length = i;
    (*line) = (char *) malloc((length+1)*sizeof(char)); //allocate space for line
    for (i=0; i<length;i++) {
        (*line)[i] = FileInput[start+i];
    }
    (*line)[i] = '\0'; // append termination
    if (c != '\0')(*pointer)++; //increment file pointer to skip end of line
    while (FileInput[*pointer] == '\n' || FileInput[*pointer]== '\r') (*pointer)++; //increment again if there is extra linefeeds or CRs
    
    if (c == '\0' && i == 0)
        length = EOF;
    /*else
     length = i;*/
    
    return length;
}

char *fileread(char name[], char access[]) {
    
    // Function to read a file
#define bufferlength 256  // max buffer length
#define MAX_FILENAME 1024  // max buffer length
    int ioerr_loc, i, c = 0;
    char buffer[bufferlength] = "";
    char *pFileData;
    FILE *pFile;
    
    pFileData = (char*) malloc(bufferlength*sizeof(char));
    pFileData[0] = '\0';
    pFile = fopen(name, access);
    if (pFile == NULL) {
        ioerr_loc = errno;
        //printf("File error %i\n",ferror);
        perror("error");
        return NULL;
    }
    else
        ioerr_loc = 0;
    
    for (i=0;i<bufferlength-1 && (c=getc(pFile)) != EOF; i++) // load the first chuck of data, up to 256 bytes
        pFileData[i] = (char) c;
    //if (c !=EOF)
    pFileData[i] = '\0';  //close off the first string
    //else
    //   pFileData[i] = EOF;
    
    while (c !=EOF) {
        i=0;
        for (i=0;i<bufferlength-1 && (c=getc(pFile)) != EOF ;i++) // load the next chuck of data, up to 256 bytes
            buffer[i] = (char) c;
        //if (c !=EOF)
        buffer[i] = '\0';  //close off the string
        //else
        // buffer[i] = EOF;
        pFileData = (char *) realloc(pFileData, (strlen(pFileData)+bufferlength+1)*sizeof(char));
        strcat(pFileData, buffer);
    }
    
    fclose(pFile);
    
    return pFileData;
}
