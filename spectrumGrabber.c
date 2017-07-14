
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
double *avgSpectrum;	  // Stores the average specturm
float *timeValues;
float *freqValues;

char measuring = 0;
char userRequestedStop = 0;
char userRequestedNext = 0;

int dacBoard;
int vgOut;
int vdOut;
static int panelHandle = 0;
int16_t scopeHandle;

FILE *fp;
FILE *tablefp;

void picoscopeInit();
int getInputNew(char FileInput[], int *pointer, char **line);
char *fileread(char name[], char access[]);
void updateTimeAxis();

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
	avgSpectrum = malloc(measuredPoints * sizeof(double));
	timeValues = malloc(measuredPoints * sizeof(float));
	freqValues = malloc(measuredPoints * sizeof(float));
	
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
	//int8_t serials[128];
	//int16_t scopeCount;
	//int16_t serialLth = 256;

	int8_t *serial = (int8_t*) "AP231/007\0";

	PICO_STATUS status;
	
	// Get list of connected picoscopes 
	// This causes a general protection fault to occur when the function returns
	//status = ps6000EnumerateUnits(&scopeCount, serials, &serialLth);

	//switch (status) {
	//	case PICO_OK:
	//		break;
	//	default:
	//		break;
	//}

	// Open the first picoscope (probably only works if there is just a single scope)
	//serial = serials;
	status = ps6000OpenUnit(&scopeHandle, serial);

	switch (status) {
		case PICO_OK:
			break;
		default:
			break;
	}

	// Initialize some values
	updateTimeAxis();

	// Set up the measurement
	status = ps6000SetChannel(scopeHandle, PS6000_CHANNEL_A, TRUE, PS6000_AC, PS6000_50MV, 0, PS6000_BW_20MHZ); 
	status = ps6000SetChannel(scopeHandle, PS6000_CHANNEL_B, FALSE, PS6000_DC_1M, PS6000_100MV, 0, PS6000_BW_20MHZ); 
	status = ps6000SetChannel(scopeHandle, PS6000_CHANNEL_C, FALSE, PS6000_DC_1M, PS6000_100MV, 0, PS6000_BW_20MHZ); 
	status = ps6000SetChannel(scopeHandle, PS6000_CHANNEL_D, FALSE, PS6000_DC_1M, PS6000_100MV, 0, PS6000_BW_20MHZ); 
	
	// Set up data buffer
	status = ps6000SetDataBuffer(scopeHandle, PS6000_CHANNEL_A, rawDataBuffer, measuredPoints, PS6000_RATIO_MODE_NONE);
	
	ps6000Stop(scopeHandle);
	
}

void PREF4 dataAvailableCallback(int16_t handle, PICO_STATUS status, void* pParameter)
{
	if (measuring == 0) {
		printf("seasuring == 0 in dataAvailableCallback()!");
	}
	uint32_t nSamples = measuredPoints;
	status = ps6000GetValues(handle, 0, &nSamples, 1, PS6000_RATIO_MODE_NONE, 0, NULL);
	
	// Done with the current measurement
	measuring = 0;
}

int CVICALLBACK binsRing_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			//Reallocate Arrays
			GetCtrlVal(panelHandle, MAINPANEL_BINSRING, &measuredPoints);
			measuredPoints = measuredPoints * 2;
	
			rawDataBuffer = realloc(rawDataBuffer, measuredPoints * sizeof(int16_t));
			zeros = realloc(zeros, measuredPoints * sizeof(double));
			dataValues = realloc(dataValues, measuredPoints * sizeof(double));
			avgSpectrum = realloc(avgSpectrum, measuredPoints * sizeof(double));
			timeValues = realloc(timeValues, measuredPoints * sizeof(float));
			freqValues = realloc(freqValues, measuredPoints * sizeof(float));
			
			updateTimeAxis();
			
			// Update the data bufer
			ps6000SetDataBuffer(scopeHandle, PS6000_CHANNEL_A, rawDataBuffer, measuredPoints, PS6000_RATIO_MODE_NONE);
			break;
	}
	
	return 0;
}

void handleMeasurement()
{
	PICO_STATUS status;
	
	// Disable the run button
	SetCtrlAttribute(panelHandle, MAINPANEL_STOPBUTTON, ATTR_DIMMED, FALSE);
	SetCtrlAttribute(panelHandle, MAINPANEL_RUNBUTTON, ATTR_DIMMED, TRUE);
	SetCtrlAttribute(panelHandle, MAINPANEL_NEXTBUTTON, ATTR_DIMMED, FALSE);
	SetCtrlAttribute(panelHandle, MAINPANEL_BINSRING, ATTR_DIMMED, TRUE);
	SetCtrlAttribute(panelHandle, MAINPANEL_RATEBOX, ATTR_DIMMED, TRUE);
	
	
	userRequestedStop = 0;
	
	int nBias;
	GetNumTableRows(panelHandle, MAINPANEL_TABLE, &nBias);
	
	// Save frequency row
	char outputFile[512];
	GetCtrlVal(panelHandle, MAINPANEL_FILEPREFIX, outputFile);
	fp = fopen(outputFile, "w+");     
	
	fprintf(fp, "%f", freqValues[0]);
	for(int i=1; i<measuredPoints/2; i++) {
		fprintf(fp, ",%f", freqValues[i]);	
	}
	
	
	// Loop over bias conditions measuring at each
	for(int i = 0; i < nBias; i++) {
		short averages = 1;
		int nMeasured = 0;
		char tmpstr[128];

		// Get number of requested averages
		GetCtrlVal(panelHandle, MAINPANEL_AVGBOX, &averages);
	
		sprintf(tmpstr, "0/%d Completed", averages);
		SetCtrlVal(panelHandle, MAINPANEL_MEASCOUNTDISP, tmpstr);
		
		// Go to requested bias
		double Vg, Vd;
		double VgCoeff, VdCoeff;
		GetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(1,i+1), &Vg);
		GetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(2,i+1), &Vd);
		GetCtrlVal(panelHandle, MAINPANEL_VGCOEFFBOX, &VgCoeff);
		GetCtrlVal(panelHandle, MAINPANEL_VDCOEFFBOX, &VdCoeff);
		
		SetActiveTableCell(panelHandle, MAINPANEL_TABLE, MakePoint(1,i+1));
		
		// Highlight background of bias in progress
		if(i==0) {
			SetTableCellRangeAttribute(panelHandle, MAINPANEL_TABLE, MakeRect(1,1,nBias,2), ATTR_TEXT_BGCOLOR, VAL_WHITE);   
		 }else{
			SetTableCellRangeAttribute(panelHandle, MAINPANEL_TABLE, VAL_TABLE_ROW_RANGE(i), ATTR_TEXT_BGCOLOR, VAL_WHITE); 
		 }
		 SetTableCellRangeAttribute(panelHandle, MAINPANEL_TABLE, VAL_TABLE_ROW_RANGE(i+1), ATTR_TEXT_BGCOLOR, VAL_PANEL_GRAY);
		
		cbVOut(dacBoard, vgOut, BIP10VOLTS, Vg/VgCoeff*0.001, 0);
		cbVOut(dacBoard, vdOut, BIP10VOLTS, Vd/VdCoeff*0.001, 0);
		
		// Loop over a single bias condition and average
		for(nMeasured = 0; nMeasured < averages; nMeasured++) {
			if (userRequestedStop)
				break;
			if(userRequestedNext)
				break;
		
			measuring = 1;
			status = ps6000RunBlock(scopeHandle, measuredPoints/2, measuredPoints/2, timebase, 1, 0, 0, dataAvailableCallback, 0);

			switch (status) {
				case PICO_OK:
					break;
				default:
					break;
			}
	
			while(measuring) {
				// Handle events
				ProcessSystemEvents();
				if (userRequestedStop) {
					// Update progress counter
					sprintf(tmpstr, "%d/%d Completed", nMeasured, nMeasured);
					SetCtrlVal(panelHandle, MAINPANEL_MEASCOUNTDISP, tmpstr);
					break;
				}
				
				if(userRequestedNext) 
					break;
			}
	
			if (userRequestedStop)
				break;
			if(userRequestedNext)
				break;
		
			// Convert values to double
			double fullScale;
			GetCtrlVal(panelHandle, MAINPANEL_RANGERING, &fullScale);
			for (int i = 0;i < measuredPoints;i++) {
				dataValues[i] = (double) rawDataBuffer[i] / 32767 * fullScale;
			}
	
			// Take FFT
			for(int i = 0;i < measuredPoints; i++) {
				zeros[i] = 0;
			}
			FFT(dataValues, zeros, measuredPoints);
	
			// From now on we only care about the first half of the points
			for(int i = 0;i < measuredPoints/2; i++) {
				// Get magnitude
				dataValues[i] = 20*log10((sqrt(dataValues[i]*dataValues[i] + zeros[i]*zeros[i])) / measuredPoints);
		
				// Average spectra
				avgSpectrum[i] = (avgSpectrum[i] * ((double) nMeasured / (double) averages) + dataValues[i] * (1 / (double) averages)) * ((double) averages) / ((double) nMeasured + 1);
			}
	
			DeleteGraphPlot(panelHandle, MAINPANEL_GRAPH, -1, VAL_DELAYED_DRAW);
			PlotXY(panelHandle, MAINPANEL_GRAPH, freqValues, avgSpectrum, measuredPoints/2, VAL_FLOAT, VAL_DOUBLE, VAL_THIN_LINE, VAL_NO_POINT, VAL_SOLID, 1, VAL_BLACK); 
		
			// Update progress counter
			GetCtrlVal(panelHandle, MAINPANEL_AVGBOX, &averages);
			sprintf(tmpstr, "%d/%d Completed", nMeasured + 1, averages);
			SetCtrlVal(panelHandle, MAINPANEL_MEASCOUNTDISP, tmpstr);
			
		}
		
		// Save average spectrum
		fprintf(fp, "\n%f", avgSpectrum[0]);
	
		for(int i=1; i<measuredPoints/2; i++) {
			fprintf(fp, ",%f", avgSpectrum[i]);	
		}
		
		if(userRequestedNext){
			userRequestedNext = 0;
			// Tell the scope to stop
			ps6000Stop(scopeHandle);
			measuring = 0;
		}	
		
		if (userRequestedStop)
			break;
		
		// Check if the number of bias points changed
		GetNumTableRows(panelHandle, MAINPANEL_TABLE, &nBias);
	}
	
	// Tell the scope to stop
	ps6000Stop(scopeHandle);
	measuring = 0;
	
	fclose(fp);
	
	// Re-enable the run button
	SetCtrlAttribute(panelHandle, MAINPANEL_STOPBUTTON, ATTR_DIMMED, TRUE);
	SetCtrlAttribute(panelHandle, MAINPANEL_RUNBUTTON, ATTR_DIMMED, FALSE);
	SetCtrlAttribute(panelHandle, MAINPANEL_NEXTBUTTON, ATTR_DIMMED, TRUE);
	SetCtrlAttribute(panelHandle, MAINPANEL_BINSRING, ATTR_DIMMED, FALSE);
	SetCtrlAttribute(panelHandle, MAINPANEL_RATEBOX, ATTR_DIMMED, FALSE);
}


void loadConditions(){
	char biasFilename[MAX_PATHNAME_LEN];
	int selectionStatus = FileSelectPopup("", "*.csv", "*.csv; *.cfg; *.*", "Select Bias Conditions", VAL_LOAD_BUTTON, 0, 0, 1, 1, biasFilename); 

	if(selectionStatus == VAL_NO_FILE_SELECTED){
		return;		
	}
	
	char *record = NULL, *line = NULL, *InputFile = NULL;
	int filepointer = 0, lineLength = 0;
	
	DeleteTableRows(panelHandle, MAINPANEL_TABLE, 1, -1);
	
	int rowCount = 1;
	InputFile = fileread(biasFilename, "r");
	while((lineLength = getInputNew(InputFile, &filepointer, &line)) != 0){
		if (lineLength == EOF)
			break;
		
		InsertTableRows(panelHandle, MAINPANEL_TABLE, -1, 1, VAL_USE_MASTER_CELL_TYPE);	
		
		record = strtok(line, ",");
		SetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(1,rowCount), atof(record));
		
		record = strtok(NULL, ",");
		SetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(2,rowCount), atof(record));
		
		rowCount++;
	}
	
	free(line);
	free(InputFile);
	InputFile = NULL;
}

void buildTable(){
	int NumVgSteps, NumVdSteps; 
	double VgStart, VgStop, VgStepSize;
	double VdStart, VdStop, VdStepSize;
	 
	//get start, stop, and step size values from boxes
	GetCtrlVal(panelHandle, MAINPANEL_VGSTARTBOX, &VgStart);  
	GetCtrlVal(panelHandle, MAINPANEL_VGSTOPBOX, &VgStop);
	GetCtrlVal(panelHandle, MAINPANEL_NUMVGSTEPBOX, &NumVgSteps);
	GetCtrlVal(panelHandle, MAINPANEL_VGSTEPSIZEBOX, &VgStepSize);
	
	GetCtrlVal(panelHandle, MAINPANEL_VDSTARTBOX, &VdStart);  
	GetCtrlVal(panelHandle, MAINPANEL_VDSTOPBOX, &VdStop);
	GetCtrlVal(panelHandle, MAINPANEL_NUMVDSTEPBOX, &NumVdSteps);
	GetCtrlVal(panelHandle, MAINPANEL_VDSTEPSIZEBOX, &VdStepSize);

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
	
	//loop through arrays and construct table
	DeleteTableRows(panelHandle, MAINPANEL_TABLE, 1, -1);
	
	int rowCount = 0;
	
	for(int i=0; i<NumVgSteps; i++){
		for(int	j=0; j<NumVdSteps; j++){
			InsertTableRows(panelHandle, MAINPANEL_TABLE, -1, 1, VAL_USE_MASTER_CELL_TYPE);
			rowCount++;
			
			SetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(1,rowCount), VgVals[i]);
			SetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(2,rowCount), VdVals[j]);		
		}	
	}	
}	

void calculateVgStepSize() {
	int NumVgSteps; 
	double VgStart, VgStop, VgStepSize;

	//Get start, stop, and number of steps from textboxes
	GetCtrlVal(panelHandle, MAINPANEL_VGSTARTBOX, &VgStart);  
	GetCtrlVal(panelHandle, MAINPANEL_VGSTOPBOX, &VgStop);
	GetCtrlVal(panelHandle, MAINPANEL_NUMVGSTEPBOX, &NumVgSteps);

	//Calclate and set step size
	if(NumVgSteps != 1) {
		VgStepSize = (VgStop - VgStart)/(NumVgSteps - 1);
		SetCtrlVal(panelHandle, MAINPANEL_VGSTEPSIZEBOX, VgStepSize);
	}
	else {
		SetCtrlVal(panelHandle, MAINPANEL_VGSTEPSIZEBOX, 0.00);
	}	
}


void calculateVdStepSize() {
	int NumVdSteps;
	double VdStart, VdStop, VdStepSize;

	//Get start, stop, and number of steps from textboxes 
	GetCtrlVal(panelHandle, MAINPANEL_VDSTARTBOX, &VdStart);  
	GetCtrlVal(panelHandle, MAINPANEL_VDSTOPBOX, &VdStop);
	GetCtrlVal(panelHandle, MAINPANEL_NUMVDSTEPBOX, &NumVdSteps);

	//Calclate and set step size
	if(NumVdSteps != 1) {
		VdStepSize = (VdStop - VdStart)/(NumVdSteps - 1);
		SetCtrlVal(panelHandle, MAINPANEL_VDSTEPSIZEBOX, VdStepSize);
	}
	else {
		SetCtrlVal(panelHandle, MAINPANEL_VDSTEPSIZEBOX, 0.00);
	}	
}

void saveTable(char *savetableFilename) {
	double Vg, Vd; 
	int nBias;
	GetNumTableRows(panelHandle, MAINPANEL_TABLE, &nBias);
	
	tablefp = fopen(savetableFilename, "w+");   
												  //MakePoint(col,row)  
	GetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(1,1), &Vg);
	GetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(2,1), &Vd);
	
	fprintf(tablefp, "%f", Vg);
	fprintf(tablefp, ",%f", Vd);
	
	for(int i=1; i<nBias; i++) {
		GetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(1,i+1), &Vg);
		GetTableCellVal(panelHandle, MAINPANEL_TABLE, MakePoint(2,i+1), &Vd);
		
		fprintf(tablefp, "\n%f", Vg);
		fprintf(tablefp, ",%f", Vd);	
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
		timeValues[i] = time/1e9;
		freqValues[i] = i/(measuredPoints*timeInterval_ns/1e9);
		time += timeInterval_ns;
	}
}

int CVICALLBACK loadButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			loadConditions();
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
			break;
	}	
	return 0;
}

int CVICALLBACK addrowButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			InsertTableRows(panelHandle, MAINPANEL_TABLE, -1, 1, VAL_USE_MASTER_CELL_TYPE);	
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

int CVICALLBACK rangeRing_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	double fullScale;
	int scaleSetting;
	switch(event){
		case EVENT_COMMIT:
			GetCtrlVal(panelHandle, MAINPANEL_RANGERING, &fullScale);
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
			// Set up the measurement
			ps6000SetChannel(scopeHandle, PS6000_CHANNEL_A, TRUE, PS6000_AC, scaleSetting, 0, PS6000_BW_20MHZ); 
	
			// Set up data buffer
			ps6000SetDataBuffer(scopeHandle, PS6000_CHANNEL_A, rawDataBuffer, measuredPoints, PS6000_RATIO_MODE_NONE);
			break;
	}	
	return 0;
}

int CVICALLBACK runButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			char saveFilename[MAX_PATHNAME_LEN];
			char configFilename[MAX_PATHNAME_LEN+4];
			int status = FileSelectPopup("", "*.csv", "*.csv;*.*", "Select save file", VAL_SAVE_BUTTON, 0, 0, 1, 1, saveFilename);
			if (status) {
				SetCtrlVal(panelHandle, MAINPANEL_FILEPREFIX, saveFilename);
				strcpy(configFilename, saveFilename);
				strcat(configFilename, ".cfg");
				saveTable(configFilename);
				handleMeasurement();
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
			int status = FileSelectPopup("", "*.csv", "*.csv;*.*", "Select save file", VAL_SAVE_BUTTON, 0, 0, 1, 1, savetableFilename);
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

int CVICALLBACK buildtableButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			buildTable();
			break;
	}	
	return 0;
}

int CVICALLBACK editVgBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_VAL_CHANGED:
			calculateVgStepSize();
			break;
	}	
	return 0;
}

int CVICALLBACK editVdBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_VAL_CHANGED:
			calculateVdStepSize();
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
    int ioerr_loc, i, c;
    char buffer[bufferlength];
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
