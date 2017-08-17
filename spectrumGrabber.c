
#include "cbw.h" 
#include <cviauto.h>
#include <analysis.h>
#include <ansi_c.h>
#include <cvirte.h>		
#include <userint.h>
#include "spectrumGrabber.h"
#include "toolbox.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "picoscopes.h"
#include "transposeText.h"

struct limits {double min; double max; int iMin; int iMax};

enum uiState {UI_NO_SCOPE, UI_NEW_SCOPE, UI_IDLE, UI_MEASURING};

//uint32_t timebase = 7816;
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
int channelLabels[] = {MAINPANEL_LABELA, MAINPANEL_LABELB, MAINPANEL_LABELC, MAINPANEL_LABELD};
int channelLeds[] = {MAINPANEL_LEDA, MAINPANEL_LEDB, MAINPANEL_LEDC, MAINPANEL_LEDD};
int overloadLeds[] = {MAINPANEL_OLLEDA, MAINPANEL_OLLEDB, MAINPANEL_OLLEDC, MAINPANEL_OLLEDD};
int channelRanges[] = {MAINPANEL_RANGEA, MAINPANEL_RANGEB, MAINPANEL_RANGEC, MAINPANEL_RANGED};
int channelCouplings[] = {MAINPANEL_COUPLINGA, MAINPANEL_COUPLINGB, MAINPANEL_COUPLINGC, MAINPANEL_COUPLINGD};
int channelCoeffs[] = {MAINPANEL_COEFFA, MAINPANEL_COEFFB, MAINPANEL_COEFFC, MAINPANEL_COEFFD};
int channelMeasFreq[] = {MAINPANEL_FREQA, MAINPANEL_FREQB, MAINPANEL_FREQC, MAINPANEL_FREQD};
int channelMeasTime[] = {MAINPANEL_TIMEA, MAINPANEL_TIMEB, MAINPANEL_TIMEC, MAINPANEL_TIMED};
int colors[] = {VAL_MAGENTA, VAL_CYAN, VAL_BLUE, VAL_DK_GREEN};
char channelLabel[][2] = {"A", "B", "C", "D"};

int dacBoard;
int vgOut;
int vdOut;
static int panelHandle = 0;
int tgHandle = 0;
int16_t scopeHandle;

struct psconfig psConfig;

void calculateNewTimebase();
int picoscopeInit();
int getInputNew(char FileInput[], int *pointer, char **line);
char *fileread(char name[], char access[]);
void updateTimeAxis();
void setupScopeChannels();
int isEnabled(int handle, int control);
void gotoBiasPoint(int index);
void gotoUiState(int panelHandle, struct psconfig psConfig, enum uiState state);
void generateMatrix(double *VgVals, double *VdVals, int NumVgSteps, int NumVdSteps); 
void updateBiasCount();  
void updateTimeDisplay();
int isChannelEnabled(int channelIndex);
void updateFreqSavingWindow(double minFreq, double maxFreq, double freqStep); 
void updateTimeSavingWindow(double minTime, double maxTime, double timeStep);

int main (int argc, char *argv[])
{
	int error = 0;
	
	/* initialize and load resources */
	nullChk (InitCVIRTE (0, argv, 0));
	errChk (panelHandle = LoadPanel (0, "spectrumGrabber.uir", MAINPANEL));
	
	// Fill in picoscopes menu
	int8_t *picoSerial;
	enum scopeType picoType;
	char *scopeTypeChar;
	char menuName[20];
	InsertListItem(panelHandle, MAINPANEL_PICOSCOPERING, 0, "", -1);
	for(int i=0; i<3; i++){
		picoSerial = picoscopes[i].serial;
		picoType = picoscopes[i].type;
		
		switch(picoType){
			case PSNONE:
				scopeTypeChar = "NONE";
				break;
			case PS4000:
				scopeTypeChar = "PS4000";
				break;
			case PS3000A:
				scopeTypeChar = "PS3000A";
				break;
			case PS6000:
				scopeTypeChar = "PS6000";
				break;
		}
	
		sprintf(menuName, "%s: %s", scopeTypeChar, picoSerial);
		InsertListItem(panelHandle, MAINPANEL_PICOSCOPERING, i+1, menuName, i);
	}	
	
	// Initialize picoscope
	picoscopeInit();
	
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


// Read in the UI values and set up psConfig array to reflect those values
// Additionally calculate any intermediate values that are required
int picoscopeInit()
{
	PICO_STATUS status;
	
	// Fill in psConfig based on menu selection
	int pico;
	GetCtrlVal(panelHandle, MAINPANEL_PICOSCOPERING, &pico);
	
	// If the chosen scope is none disable the run button and return
	if(pico<0) {
		psConfig.type = PSNONE;
		gotoUiState(panelHandle, psConfig, UI_NO_SCOPE);
		return 0;
	}
	
	// Set up psConfig struct
	psConfig.type = picoscopes[pico].type;
	psConfig.serial = picoscopes[pico].serial;
	psConfig.nChannels = picoscopes[pico].nChannels;
	psConfig.nCouplings = picoscopes[pico].nCouplings;
	psConfig.couplings = picoscopes[pico].couplings;
	psConfig.nRanges = picoscopes[pico].nRanges;
	psConfig.ranges = picoscopes[pico].ranges;
	
	// Enable or disable ui elements according to the selected scope
	gotoUiState(panelHandle, psConfig, UI_NEW_SCOPE);
	
	// Open the picoscope
	status = psOpenUnit(&psConfig);
	if (status != 0) {
		return status;
	}
	
	// Read the number of requested points from the UI
	GetCtrlVal(panelHandle, MAINPANEL_BINSRING, &(psConfig.nPoints));
	psConfig.nPoints = psConfig.nPoints * 2;
	
	// Set up sample rate according to the UI values
	calculateNewTimebase();
	
	// Get the downsample ratio
	GetCtrlVal(panelHandle, MAINPANEL_DOWNSAMPLEBOX, &(psConfig.downsampleRatio));
	
	updateTimeDisplay();

	switch (status) {
		case PICO_OK:
			break;
		default:
			break;
	}

	//Initialize the channels
	psConfig.channels[0].channel = PS_CHANNEL_A;
	psConfig.channels[1].channel = PS_CHANNEL_B;
	psConfig.channels[2].channel = PS_CHANNEL_C;
	psConfig.channels[3].channel = PS_CHANNEL_D;
	
	setupScopeChannels();
	psStop(&psConfig);
	
	// Set up data buffer
	psMemorySegments(&psConfig);
	
	return 0;
	
}

void updateTimeSavingWindow(double minTime, double maxTime, double timeStep)
{
	SetCtrlVal(panelHandle, MAINPANEL_MINTIME, minTime);
	SetCtrlVal(panelHandle, MAINPANEL_MAXTIME, maxTime);
	int steps = RoundRealToNearestInteger((maxTime-minTime)/timeStep) + 1;
	char str[128];
	sprintf(str, "%d", steps);
	SetCtrlVal(panelHandle, MAINPANEL_NUMTBOX, str);
}

void updateFreqSavingWindow(double minFreq, double maxFreq, double freqStep)
{
	SetCtrlVal(panelHandle, MAINPANEL_MINFREQ, minFreq);
	SetCtrlVal(panelHandle, MAINPANEL_MAXFREQ, maxFreq);
	int steps = RoundRealToNearestInteger((maxFreq-minFreq)/freqStep) + 1;
	char str[128];
	sprintf(str, "%d", steps);
	SetCtrlVal(panelHandle, MAINPANEL_NUMFBOX, str);
}

void setupScopeChannel(int channelIndex, int channelLabel, int enabledLed, int rangeRing, int couplingRing, int coeffBox)
{
	int enabled;
	// Get Coupling
	int couplingInt;
	enum psCoupling coupling;
	GetCtrlVal(panelHandle, couplingRing, &couplingInt);
	coupling = (enum psCoupling) couplingInt;
	
	// Get range
	int rangeInt;
	enum psRange range;
	GetCtrlVal(panelHandle, rangeRing, &rangeInt);
	range = (enum psRange) rangeInt;
	
	double coeff;
	GetCtrlVal(panelHandle, coeffBox, &coeff); 
	
	if (isChannelEnabled(channelIndex)){
		enabled = 1;	
	} else {
		enabled = 0;	
	}	
	
	// Update psConfig
	psConfig.channels[channelIndex].range = range;
	psConfig.channels[channelIndex].coupling = coupling;
	psConfig.channels[channelIndex].coefficient = coeff;
	psConfig.channels[channelIndex].enabled = (int16_t) enabled;
	GetCtrlVal(panelHandle, channelLabel, psConfig.channels[channelIndex].name);
	
	// Set the channel
	psSetChannel(&psConfig, channelIndex);
}

void setupScopeChannels()
{
	// Set up scope channels according to UI defaults
	for(int i =0;i<psConfig.nChannels;i++) {
		setupScopeChannel(i, channelLabels[i], channelLeds[i], channelRanges[i], channelCouplings[i], channelCoeffs[i]);
	}
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
			updateTimeDisplay();
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

int CVICALLBACK delayBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			updateTimeDisplay();
		break;
	}
	return 0;
}

void processData(int nMeasured, int averages, int iBias, char ***timeFileNames, struct limits timeLimits)
{
	dataReady = 0;
	short overflow = 0;
	
	int freqTabHandle, timeTabHandle;
	GetPanelHandleFromTabPage(panelHandle, MAINPANEL_TAB, 0, &freqTabHandle);
	GetPanelHandleFromTabPage(panelHandle, MAINPANEL_TAB, 1, &timeTabHandle);
	
	// Clear graph in preparation for new plots
	DeleteGraphPlot(freqTabHandle, FREQTAB_FREQGRAPH, -1, VAL_NO_DRAW);
	
	if(nMeasured==0)
		DeleteGraphPlot(timeTabHandle, TIMETAB_TIMEGRAPH, -1, VAL_DELAYED_DRAW);
	
	// Loop over each scope input
	for(int iChannel = 0;iChannel < 4;iChannel++) {
		// Skip if the input is disabled
		if(!isEnabled(panelHandle, channelMeasFreq[iChannel]) && !isEnabled(panelHandle, channelMeasTime[iChannel]))
			continue;
		
		// Get data from scope
		uint32_t nPoints = psConfig.nPoints;
		
		psSetDataBuffer(&psConfig, iChannel, psConfig.nPoints, rawDataBuffer);
		psGetValues(&psConfig, &nPoints, &overflow);
		psSetDataBuffer(&psConfig, iChannel, psConfig.nPoints, NULL); 
		
		// Check for overload and turn on LED if overload occurred
		for(int i=0; i<4; i++) {
			if(overflow & (1<<i))
				SetCtrlVal(panelHandle, overloadLeds[i], 1);
		}
		
		// Log the raw data
		//FILE *logfp = fopen("dataLog.log", "w");
		//for(int j = 0;j<nPoints;j++) {
		//	fprintf(logfp, "%d\n", rawDataBuffer[j]);
		//}
		//fclose(logfp);
		
		scaleReading(&psConfig, iChannel, rawDataBuffer, dataValues);
	
		// Save timeValues if time domain saving is requested and this is the first sweep at this bias point
		if (isEnabled(panelHandle, channelMeasTime[iChannel]) && nMeasured==0 && isEnabled(panelHandle, MAINPANEL_DISABLESAVEBUTTON)) {
			// Save time domain signal
			FILE *fp = fopen(timeFileNames[iChannel][iBias + 1], "w");
			for(int j=timeLimits.iMin; j<timeLimits.iMax+1; j++) {
				fprintf(fp, "%13.6e\n", dataValues[j]);	
			}
			fclose(fp);
		}

		// Plot the time domain
		if (isChannelEnabled(iChannel) && nMeasured==0) { 
			PlotXY(timeTabHandle, TIMETAB_TIMEGRAPH, timeValues, dataValues, psConfig.nPoints, VAL_FLOAT, VAL_DOUBLE, VAL_THIN_LINE, VAL_NO_POINT, VAL_SOLID, 1, colors[iChannel]);
		}	
		
		// Compute FFT if the FFT is requested
		if (isEnabled(panelHandle, channelMeasFreq[iChannel])) {
			// Take FFT
			for(int j = 0;j < psConfig.nPoints; j++) {
				zeros[j] = 0;
			}
			FFT(dataValues, zeros, psConfig.nPoints);

			// From now on we only care about the first half of the points
			for(int j = 0;j < psConfig.nPoints/2; j++) {
				// Get magnitude
				dataValues[j] = (sqrt(dataValues[j]*dataValues[j] + zeros[j]*zeros[j])) / psConfig.nPoints;

				// Average spectra
				avgSpectrum[iChannel][j] = (avgSpectrum[iChannel][j] * ((double) nMeasured / (double) averages) + dataValues[j] * (1 / (double) averages)) * ((double) averages) / ((double) nMeasured + 1);
		
				// Display units
				if (avgSpectrum[iChannel][j] <= 0) { 
					avgSpectrumDisplay[j] = -1000;
				} else {
					avgSpectrumDisplay[j] = 20*log10(avgSpectrum[iChannel][j]);
				}
			}
		
			// Display the data
			PlotXY(freqTabHandle, FREQTAB_FREQGRAPH, freqValues, avgSpectrumDisplay, psConfig.nPoints/2, VAL_FLOAT, VAL_DOUBLE, VAL_THIN_LINE, VAL_NO_POINT, VAL_SOLID, 1, colors[iChannel]);
		}
	}
	//SetCtrlAttribute(panelHandle, MAINPANEL_FREQGRAPH, ATTR_REFRESH_GRAPH, TRUE);
	RefreshGraph(freqTabHandle, FREQTAB_FREQGRAPH);
}

void updateTimeAxis(struct limits *freqLimits, struct limits *timeLimits)
{
	float timeInterval_ns = 0;
	psGetTimebase2(&psConfig, &timeInterval_ns);
	for (int i = 0;i < psConfig.nPoints/2;i++) {
		timeValues[i] = (timeInterval_ns*i)/1e9;
		freqValues[i] = i/(psConfig.nPoints*timeInterval_ns/1e9);
		if (timeValues[i] < timeLimits->min)
			timeLimits->iMin = i+1;
		if (freqValues[i] < freqLimits->min)
			freqLimits->iMin = i+1;
		if (timeValues[i] <= timeLimits->max)
			timeLimits->iMax = i;
		if (freqValues[i] <= freqLimits->max)
			freqLimits->iMax = i;
	}
	for (int i = psConfig.nPoints/2;i < psConfig.nPoints;i++) {
		timeValues[i] = (timeInterval_ns*i)/1e9;
		if (timeValues[i] < timeLimits->min)
			timeLimits->iMin = i+1;
		if (timeValues[i] <= timeLimits->max)
			timeLimits->iMax = i;
	}
	if (timeLimits->iMin < 0)
		timeLimits->iMin = 0;
	if (timeLimits->iMax > psConfig.nPoints - 1)
		timeLimits->iMax = psConfig.nPoints - 1;
	if (freqLimits->iMin < 0)
		freqLimits->iMin = 0;
	if (freqLimits->iMax > psConfig.nPoints/2 - 1)
		freqLimits->iMax = psConfig.nPoints/2 - 1;
}

void handleMeasurement(char *path, char *name, char *ext)
{
	FILE *freqFP = NULL;
	FILE *timeFP = NULL;
	PICO_STATUS status;
	
	// Disable the run button
	gotoUiState(panelHandle, psConfig, UI_MEASURING);
	userRequestedStop = 0;
	
	// Allocate required memory
	rawDataBuffer = malloc(psConfig.nPoints * sizeof(int16_t));
	zeros = malloc(psConfig.nPoints * sizeof(double));
	dataValues = malloc(psConfig.nPoints * sizeof(double));
	avgSpectrumDisplay = malloc(psConfig.nPoints * sizeof(double)/2);
	timeValues = malloc(psConfig.nPoints * sizeof(float));
	freqValues = malloc(psConfig.nPoints * sizeof(float)/2);
	
	for(int i =0;i<4;i++) {
		if(isEnabled(panelHandle, channelMeasFreq[i]))
			avgSpectrum[i] = malloc(psConfig.nPoints * sizeof(double)/2);
	}
	
	// Determine saving limits
	struct limits freqLimits = {.iMin = 0};
	struct limits timeLimits = {.iMin = 0};
	GetCtrlVal(panelHandle, MAINPANEL_MINFREQ, &freqLimits.min);
	GetCtrlVal(panelHandle, MAINPANEL_MAXFREQ, &freqLimits.max);
	GetCtrlVal(panelHandle, MAINPANEL_MINTIME, &timeLimits.min);
	GetCtrlVal(panelHandle, MAINPANEL_MAXTIME, &timeLimits.max);
	
	// Initialize time axis values
	updateTimeAxis(&freqLimits, &timeLimits);
	
	int dacEnabled = 0;
	int nBias = 1;
	GetCtrlVal(panelHandle, MAINPANEL_DACBUTTON, &dacEnabled);
	
	if (dacEnabled)
		GetNumTableRows(panelHandle, MAINPANEL_TABLE, &nBias);
	
	
	// filename = path + _ + name + _ + type + _ + label + . + ext + null
	int filenameLen = strlen(path) + 1 + strlen(name) + 1 + 4 + 1 + 31 + 1 + strlen(ext) + 1;
	
	char **freqFileNames[4] = {0,0,0,0}, *freqFileName[4] = {0,0,0,0};
	char **timeFileNames[4] = {0,0,0,0}, *timeFileName[4] = {0,0,0,0};
	for(int i = 0;i < 4;i++) {
		freqFileNames[i] = malloc((nBias + 1) * sizeof(char*));
		timeFileNames[i] = malloc((nBias + 1) * sizeof(char*));
	}
	
	// Make all data filenames
	char *filename;
	for (int i = 0;i < 4;i++) {
		freqFileName[i] = malloc((filenameLen) * sizeof(char));
		sprintf(freqFileName[i], "%s%s_Freq_%s%s", path, name, psConfig.channels[i].name, ext);
		timeFileName[i] = malloc((filenameLen) * sizeof(char));
		sprintf(timeFileName[i], "%s%s_Time_%s%s", path, name, psConfig.channels[i].name, ext);
		for (int j = 0;j<nBias+1;j++) {
			freqFileNames[i][j] = malloc((filenameLen + 1 + 8) * sizeof(char));
			sprintf(freqFileNames[i][j], "%s%s_Freq_%s_%d%s", path, name, psConfig.channels[i].name, j, ext);
			timeFileNames[i][j] = malloc((filenameLen + 1 + 8) * sizeof(char));
			sprintf(timeFileNames[i][j], "%s%s_Time_%s_%d%s", path, name, psConfig.channels[i].name, j, ext);
		}
	}
	filename = NULL;
	
	for (int i = 0;i < nBias;i++) {
	}
	
	// Store frequency and time axes in the files
	for(int iChannel = 0;iChannel<4;iChannel++) {
		// Skip if this channel is neither measurement is requested
		if (!(isChannelEnabled(iChannel)))
			continue;

		// Save frequency column
		if (isEnabled(panelHandle, channelMeasFreq[iChannel]) && isEnabled(panelHandle, MAINPANEL_DISABLESAVEBUTTON)) {
			freqFP = fopen(freqFileNames[iChannel][0], "w");
	
			// Write data
			for(int j=freqLimits.iMin; j<freqLimits.iMax+1; j++) {
				fprintf(freqFP, "%13.6e\n", freqValues[j]);	
			}
			fclose(freqFP);
		}
	
		// Save time row
		if (isEnabled(panelHandle, channelMeasTime[iChannel]) && isEnabled(panelHandle, MAINPANEL_DISABLESAVEBUTTON)) {
			timeFP = fopen(timeFileNames[iChannel][0], "w");
	
			// Write data
			for(int j=timeLimits.iMin; j<timeLimits.iMax+1; j++) {
				fprintf(timeFP, "%13.6e\n", timeValues[j]);	
			}
			fclose(timeFP);
		}
	}
			
	// Loop over bias conditions measuring at each
	int iBias = 0;
	for(int i = 0; i < nBias; i++) {
		iBias = i;
		short averages = 1;
		int nMeasured = 0;
		char tmpstr[128];

		// Get number of requested averages
		GetCtrlVal(panelHandle, MAINPANEL_AVGBOX, &averages);
	
		sprintf(tmpstr, "0/%d Averages", averages);
		SetCtrlVal(panelHandle, MAINPANEL_AVGCOUNTDISP, tmpstr);
		
		// Go to bias point `i` in table
		gotoBiasPoint(i);
		
		double delay;
		GetCtrlVal(panelHandle, MAINPANEL_DELAYBOX, &delay);
		Delay(delay);
		
		// Loop within a single bias condition and average
		for(nMeasured = 0; nMeasured < averages; nMeasured++) {
			measurementInProgress = 1;
			dataReady = 0;
			
			status = psRunBlock(&psConfig, dataAvailableCallback);

			switch (status) {
				case PICO_OK:
					break;
				default:
					break;
			}
	
			while(measurementInProgress) {
				// Handle events
				ProcessSystemEvents();
				// If the user skips this data point delete the filename for the time file
				if ((userRequestedStop || userRequestedNext) && nMeasured == 0) {
					for (int k = 0;k<4;k++) {
						timeFileNames[k][i+1][0] = 0;
					}
				}
				if (userRequestedStop) {
					// Update progress counter
					sprintf(tmpstr, "%d/%d Averages", nMeasured, nMeasured);
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
				processData(nMeasured, averages, i, timeFileNames, timeLimits);
		
			// Update progress counter
			GetCtrlVal(panelHandle, MAINPANEL_AVGBOX, &averages);
			sprintf(tmpstr, "%d/%d Averages", nMeasured + 1, averages);
			SetCtrlVal(panelHandle, MAINPANEL_AVGCOUNTDISP, tmpstr);
			
			if(isEnabled(panelHandle, MAINPANEL_DACBUTTON)) {
				sprintf(tmpstr, "%d/%d Bias Points", i + 1, nBias); 
				SetCtrlVal(panelHandle, MAINPANEL_BIASCOUNTDISP, tmpstr);
			}
		}
		
		// Save average spectra
		if ((userRequestedStop || userRequestedNext) && nMeasured == 0) {
			for (int k = 0;k<4;k++) {
				freqFileNames[k][i+1][0] = 0;
			}
		} else {
			for (int j = 0;j < 4;j++) {
				if(isEnabled(panelHandle, channelMeasFreq[j]) && isEnabled(panelHandle, MAINPANEL_DISABLESAVEBUTTON)) {
					// Save average spectrum
					FILE *fp = fopen(freqFileNames[j][i+1], "w");
					for(int k=freqLimits.iMin; k<freqLimits.iMax+1; k++) {
						fprintf(fp, "%13.6e\n", avgSpectrum[j][k]);	
					}
					fclose(fp);
				}
			}
		}
		
		if (userRequestedStop)
			break;
		if(userRequestedNext){
			userRequestedNext = 0;
			// Tell the scope to stop
			psStop(&psConfig);
			measurementInProgress = 0;
		}	
		
		// Check if the number of bias points changed 
		if (dacEnabled)
			GetNumTableRows(panelHandle, MAINPANEL_TABLE, &nBias);
	}
	
	// Free memory
	free(rawDataBuffer);		rawDataBuffer = NULL;
	free(zeros);				zeros = NULL;
	free(dataValues);			dataValues = NULL;
	free(avgSpectrumDisplay);   avgSpectrumDisplay = NULL;
	free(timeValues);			timeValues = NULL;
	free(freqValues);			freqValues = NULL;
	
	for(int i =0;i<4;i++) {
		if(avgSpectrum) {
			free(avgSpectrum[i]);
			avgSpectrum[i] = NULL;
		}
	}
	
	// Tell the scope to stop
	psStop(&psConfig);
	measurementInProgress = 0;
	
	// Combine data files
	if (isEnabled(panelHandle, MAINPANEL_DISABLESAVEBUTTON)) {
		for(int i = 0;i < 4; i++) {
			if(isEnabled(panelHandle, channelMeasFreq[i]))
				combineFiles(freqFileName[i], freqFileNames[i], ",", iBias + 1 + 1, 13, 1<<12);
	
			if(isEnabled(panelHandle, channelMeasTime[i]))
				combineFiles(timeFileName[i], timeFileNames[i], ",", iBias + 1 + 1, 13, 1<<12);
		}
	}
	
	// Delete files and free filename memory
	for(int i = 0;i < 4;i++) {
		free(freqFileName[i]);
		free(timeFileName[i]);
		for(int j = 0;j<nBias+1;j++) {
			remove(freqFileNames[i][j]);  
			free(freqFileNames[i][j]);
			remove(timeFileNames[i][j]);
			free(timeFileNames[i][j]);
		}
		free(freqFileNames[i]);
		free(timeFileNames[i]);
	}
	
	// Re-enable the run button
	gotoUiState(panelHandle, psConfig, UI_IDLE);
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

int isChannelEnabled(int i)
{
	return isEnabled(panelHandle, channelMeasFreq[i]) | isEnabled(panelHandle, channelMeasTime[i]);
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
	
	SetCtrlVal(panelHandle, MAINPANEL_DACBUTTON, 1);
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

int CVICALLBACK loadButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			loadConditions();
			updateBiasCount();
			updateTimeDisplay();
			break;
	}	
	return 0;
}

int CVICALLBACK channel_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			// Stop current capture
			//psStop(&psConfig);
			
			//enum psChannel channel = 0;
			int channelIndex = 0;
			switch (control) {
				case MAINPANEL_LABELA:
				case MAINPANEL_RANGEA:
				case MAINPANEL_COUPLINGA:
				case MAINPANEL_COEFFA:
				case MAINPANEL_FREQA:
				case MAINPANEL_TIMEA:
					channelIndex = 0;
					break;
				case MAINPANEL_LABELB:
				case MAINPANEL_RANGEB:
				case MAINPANEL_COUPLINGB:
				case MAINPANEL_COEFFB:
				case MAINPANEL_FREQB:
				case MAINPANEL_TIMEB:
					channelIndex = 1;
					break;
				case MAINPANEL_LABELC:
				case MAINPANEL_RANGEC:
				case MAINPANEL_COUPLINGC:
				case MAINPANEL_COEFFC:
				case MAINPANEL_FREQC:
				case MAINPANEL_TIMEC:
					channelIndex = 2;
					break;
				case MAINPANEL_LABELD:
				case MAINPANEL_RANGED:
				case MAINPANEL_COUPLINGD:
				case MAINPANEL_COEFFD:
				case MAINPANEL_FREQD:
				case MAINPANEL_TIMED:
					channelIndex = 3;
					break;
			}
			
			if (isChannelEnabled(channelIndex)) {
				// Enable the current channel
				SetCtrlVal(panelHandle, channelLeds[channelIndex], 1);
				SetCtrlAttribute(panelHandle, channelRanges[channelIndex], ATTR_DIMMED, FALSE);
				SetCtrlAttribute(panelHandle, channelCouplings[channelIndex], ATTR_DIMMED, FALSE);
				SetCtrlAttribute(panelHandle, channelCoeffs[channelIndex], ATTR_DIMMED, FALSE);
				SetCtrlAttribute(panelHandle, overloadLeds[channelIndex], ATTR_DIMMED, FALSE);
			}
			else {
				// Disable the current channel
				SetCtrlVal(panelHandle, channelLeds[channelIndex], 0);
				SetCtrlAttribute(panelHandle, channelRanges[channelIndex], ATTR_DIMMED, TRUE);
				SetCtrlAttribute(panelHandle, channelCouplings[channelIndex], ATTR_DIMMED, TRUE);
				SetCtrlAttribute(panelHandle, channelCoeffs[channelIndex], ATTR_DIMMED, TRUE);
				SetCtrlAttribute(panelHandle, overloadLeds[channelIndex], ATTR_DIMMED, TRUE);
			}
			
			setupScopeChannel(channelIndex, channelLabels[channelIndex], channelLeds[channelIndex], channelRanges[channelIndex], channelCouplings[channelIndex], channelCoeffs[channelIndex]);
			
			// Resume capture if in progress
			//if(measurementInProgress)
			//	psRunBlock(&psConfig, dataAvailableCallback); 
			
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
			updateTimeDisplay();
			break;
	}	
	return 0;
}

int CVICALLBACK closePanel_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			DiscardPanel(panel);
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
			updateTimeDisplay();
			break;
	}	
	return 0;
}

int CVICALLBACK binsRing_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			GetCtrlVal(panelHandle, MAINPANEL_BINSRING, &(psConfig.nPoints));
			psConfig.nPoints = psConfig.nPoints * 2;
			updateTimeDisplay();
			double sampleRate;
			GetCtrlVal(panelHandle, MAINPANEL_RATEBOX, &sampleRate);
			double maxFreq;
			GetCtrlVal(panelHandle, MAINPANEL_MAXFREQ, &maxFreq);
			updateFreqSavingWindow(0, maxFreq, sampleRate/(psConfig.nPoints));
			updateTimeSavingWindow(0, psConfig.nPoints / sampleRate, 1/sampleRate);
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
			updateTimeDisplay();
			break;
	}	
	return 0;
}

int CVICALLBACK disableSave_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			if (isEnabled(panelHandle, MAINPANEL_DISABLESAVEBUTTON)) {
				SetPanelAttribute(panelHandle, ATTR_BACKCOLOR, 0x00F0F0F0);
			} else {
				SetPanelAttribute(panelHandle, ATTR_BACKCOLOR, 0x00FF9999);
			}
			break;
	}	
	return 0;
}

int CVICALLBACK dsBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			GetCtrlVal(panel, control, &(psConfig.downsampleRatio));
			calculateNewTimebase();
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

int CVICALLBACK picoscopeRing_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			psCloseUnit(&psConfig);
			int status = picoscopeInit();
			if (status != 0) {
				SetCtrlIndex(panelHandle, MAINPANEL_PICOSCOPERING, 0);
				picoscopeInit();
			}
			break;
	}	
	return 0;
}

int CVICALLBACK rateBox_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch(event){
		case EVENT_COMMIT:
			calculateNewTimebase();
			break;
	}	
	return 0;
}

// Find nearest timebase and change displayed sample rate to corresponding value
void calculateNewTimebase()
{
	// Only run this if a scope is selected
	if (psConfig.type == PSNONE)
		return;
	
	double sampleRate;
	float timeInterval_ns;
	GetCtrlVal(panelHandle, MAINPANEL_RATEBOX, &sampleRate);
	GetCtrlVal(panelHandle, MAINPANEL_DOWNSAMPLEBOX, &(psConfig.downsampleRatio));
	psUpdateTimebase(&psConfig, sampleRate);
	psGetTimebase2(&psConfig, &timeInterval_ns);
	sampleRate = 1/(timeInterval_ns*1e-9);
	SetCtrlVal(panelHandle, MAINPANEL_RATEBOX, sampleRate);
	updateTimeDisplay();
	updateFreqSavingWindow(0, sampleRate/2, 1/(psConfig.nPoints*timeInterval_ns*1e-9));
	updateTimeSavingWindow(0, psConfig.nPoints * timeInterval_ns * 1e-9, timeInterval_ns*1e-9);
}

int CVICALLBACK runButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			char saveFilename[MAX_PATHNAME_LEN];
			char path[MAX_PATHNAME_LEN-146], name[129], tstmpName[149];
			int month, day, year, hour, min, sec;
			
			for(int i=0; i<4; i++) {
				SetCtrlVal(panelHandle, overloadLeds[i], 0);
			}
			
			// Get file location and name(suffix)
			GetCtrlVal(panelHandle, MAINPANEL_FILEPATH, path);
			GetCtrlVal(panelHandle, MAINPANEL_FILEPREFIX, name); 
			
			// Get timestamp
			GetSystemDate(&month, &day, &year);
			GetSystemTime(&hour, &min, &sec);
			
			// Modify name
			sprintf(tstmpName, "%04d-%02d-%02d_%02d.%02d.%02d_%s", year, month, day, hour, min, sec, name);
			
			if (isEnabled(panelHandle, MAINPANEL_DISABLESAVEBUTTON)) {
				if (strcmp(path, "") == 0) {
					sprintf(saveFilename, "%s.cfg", tstmpName);
					saveTable(saveFilename);
				} else {
					sprintf(saveFilename, "%s\\%s.cfg", path, tstmpName);
					saveTable(saveFilename);
				}
			}
			
			handleMeasurement(path, tstmpName, ".csv");
				
			break;
	}
	
	return 0;
}

int CVICALLBACK dirButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			char saveFilepath[MAX_PATHNAME_LEN];
			if(DirSelectPopup("", "Select File Location", 1, 1, saveFilepath))
			{
				strcat(saveFilepath, "\\");
				SetCtrlVal(panelHandle, MAINPANEL_FILEPATH, saveFilepath);
			}
			break;
	}
	return 0;
}

int CVICALLBACK saveLimits_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			float timeStep = 0;
			psGetTimebase2(&psConfig, &timeStep);
			timeStep = timeStep * 1e-9;
			float binSize = 1/(psConfig.nPoints*timeStep);
			float maxFreq = 1/(timeStep * 2);
			float maxTime = timeStep * psConfig.nPoints;
			
			double newVal = 0;
			double otherVal = 0;
			GetCtrlVal(panelHandle, control, &newVal);
			
			// Snap value to nearest actual sample point
			long steps = 1;
			switch (control) {
				case MAINPANEL_MAXFREQ:
						if (newVal > maxFreq)
							newVal = maxFreq;
				case MAINPANEL_MINFREQ:
					steps = RoundRealToNearestInteger(newVal / binSize);
					newVal = steps * binSize;
					break;
				case MAINPANEL_MAXTIME:
					if (newVal > maxTime)
						newVal = maxTime;
				case MAINPANEL_MINTIME:
					steps = RoundRealToNearestInteger(newVal / timeStep);
					newVal = steps * timeStep;
					break;
			}
			
			// Enforce min < max  
			switch (control) {
				case MAINPANEL_MINFREQ:
					GetCtrlVal(panel, MAINPANEL_MAXFREQ, &otherVal);
					if (newVal > otherVal)
						newVal = otherVal;
					break;
				case MAINPANEL_MAXFREQ:
					GetCtrlVal(panel, MAINPANEL_MINFREQ, &otherVal);
					if (newVal < otherVal)
						newVal = otherVal;
					break;
				case MAINPANEL_MINTIME:
					GetCtrlVal(panel, MAINPANEL_MAXTIME, &otherVal);
					if (newVal > otherVal)
						newVal = otherVal;
					break;
				case MAINPANEL_MAXTIME:
					GetCtrlVal(panel, MAINPANEL_MINTIME, &otherVal);
					if (newVal < otherVal)
						newVal = otherVal;
					break;
			}
			
			// Update the value
			switch (control) {
				case MAINPANEL_MINFREQ:
					updateFreqSavingWindow(newVal, otherVal, binSize);
					break;
				case MAINPANEL_MAXFREQ:
					updateFreqSavingWindow(otherVal, newVal, binSize);
					break;
				case MAINPANEL_MINTIME:
					updateTimeSavingWindow(newVal, otherVal, timeStep);
					break;
				case MAINPANEL_MAXTIME:
					updateTimeSavingWindow(otherVal, newVal, timeStep);
					break;
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

int CVICALLBACK cfPanel_CB (int panel, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_CLOSE:
			DiscardPanel(panel);
			break;
		case EVENT_KEYPRESS:
			if (eventData1 == VAL_ESC_VKEY) {
				DiscardPanel(panel);
			}
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

void formatFloat(float num, char *unit, char *outstr)
{
	char *unitstr;
	int precision = 0;
	
	if (num < 1e-12) {
		num = num / 1e-15;
		unitstr = "f";
	} else if (num < 1e-9) {
		num = num / 1e-12;
		unitstr = "p";
	} else if (num < 1e-6) {
		num = num / 1e-9;
		unitstr = "n";
	} else if (num < 1e-3) {
		num = num / 1e-6;
		unitstr = "u";
	} else if (num < 1) {
		num = num / 1e-3;
		unitstr = "m";
	} else if (num < 1e3) {
		num = num;
		unitstr = "";
	} else if (num < 1e6) {
		num = num / 1e3;
		unitstr = "k";
	} else {
		num = num / 1e6;
		unitstr = "M";
	}
	
	if (num < 10) {
		precision = 2;
	} else if (num < 100) {
		precision = 1;
	} else {
		precision = 0;
	}
	
	sprintf(outstr, "%%.%df %%s%s", precision, unit);
	sprintf(outstr, outstr, num, unitstr);
}

void updateTimeDisplay()
{
	char outstr[64];
	double sampleRate = 0;
	short averages = 0;
	double delay;
	int biasPoints;
	
	GetCtrlVal(panelHandle, MAINPANEL_RATEBOX, &sampleRate);
	GetCtrlVal(panelHandle, MAINPANEL_AVGBOX, &averages);
	GetCtrlVal(panelHandle, MAINPANEL_DELAYBOX, &delay);
	GetNumTableRows(panelHandle, MAINPANEL_TABLE, &biasPoints);
	
	// Calculate bin size
	float timeInterval_ns = 0;
	psGetTimebase2(&psConfig, &timeInterval_ns);
	float binWidth = 1/(psConfig.nPoints*timeInterval_ns/1e9);
	
	formatFloat(binWidth, "Hz", outstr);
	SetCtrlVal(panelHandle, MAINPANEL_BINSIZETEXT, outstr);
	
	// Calculate capture time for one spectrum
	formatFloat(psConfig.nPoints / sampleRate, "s", outstr);
	SetCtrlVal(panelHandle, MAINPANEL_CAPTIMETEXT, outstr);
	
	// Calculate time per bias point
	formatFloat(psConfig.nPoints / sampleRate * averages, "s", outstr);
	SetCtrlVal(panelHandle, MAINPANEL_BIASTIMETEXT, outstr);
	
	// Calculate total time
	formatFloat(psConfig.nPoints / sampleRate * averages * biasPoints + delay * biasPoints, "s", outstr);
	SetCtrlVal(panelHandle, MAINPANEL_TOTALTIMETEXT, outstr);
}

int CVICALLBACK updateTimeAxis_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)       
{
	switch(event){
		case EVENT_VAL_CHANGED:
			double timemin, timemax;
			int timeTabHandle;
	
			GetPanelHandleFromTabPage(panelHandle, MAINPANEL_TAB, 1, &timeTabHandle);

			GetCtrlVal(timeTabHandle, TIMETAB_MINTIMEBOX, &timemin);
			GetCtrlVal(timeTabHandle, TIMETAB_MAXTIMEBOX, &timemax);

			if(timemin < timemax) {
				SetAxisScalingMode(timeTabHandle, TIMETAB_TIMEGRAPH, VAL_BOTTOM_XAXIS, VAL_MANUAL, timemin, timemax);
			}else {
				SetAxisScalingMode(timeTabHandle, TIMETAB_TIMEGRAPH, VAL_BOTTOM_XAXIS, VAL_MANUAL, timemax, timemin);
				SetCtrlVal(timeTabHandle, TIMETAB_MINTIMEBOX, timemax);
				SetCtrlVal(timeTabHandle, TIMETAB_MAXTIMEBOX, timemin);
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
			updateTimeDisplay();
			break;
	}	
	return 0;
}

int CVICALLBACK closetgButton_CB (int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			HidePanel(tgHandle);
			break;
	}
	return 0;
}

int CVICALLBACK combineFiles_CB (int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			char combineFilePath[MAX_PATHNAME_LEN];
			if(!DirSelectPopup("", "Select File Location", 1, 1, combineFilePath)) {
				break;
			}
			
			// Read values form UI
			int nFiles, buffLen;
			char fileprefix[256], fileext[16];
			GetCtrlVal(panel, CFPANEL_LASTFILE, &nFiles);
			GetCtrlVal(panel, CFPANEL_BUFFERBOX, &buffLen);
			GetCtrlVal(panel, CFPANEL_FILEPREFIX, fileprefix);
			GetCtrlVal(panel, CFPANEL_FILEEXT, fileext);
			
			// Build filename array
			// filename = path + \ + prefix + . + ext + null
			int filenameLen = strlen(combineFilePath) + 1 + strlen(fileprefix) + 1 + strlen(fileext) + 1;
			char *fileName = 0;
			char **fileNames = 0;

			// Make all data filenames
			char *filename;
			fileName = malloc((filenameLen) * sizeof(char));
			sprintf(fileName, "%s\\%s.%s", combineFilePath, fileprefix, fileext);
			fileNames = malloc((nFiles + 1) * sizeof(char*));
			for (int i = 0;i<nFiles+1;i++) {
				fileNames[i] = malloc((filenameLen + 1 + 8) * sizeof(char));
				sprintf(fileNames[i], "%s\\%s_%d.%s", combineFilePath, fileprefix, i, fileext);
			}
			filename = NULL;
			
			// Combine files
			combineFiles(fileName, fileNames, ",", nFiles+1, 13, buffLen); 
			
			// Free filename array
			free(fileName);
			for (int i = 0;i<nFiles+1;i++) {
				free(fileNames[i]);
			}
			free(fileNames);
			break;
	}
	return 0;
}

int CVICALLBACK combineButton_CB (int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			DisplayPanel(LoadPanel(panelHandle, "spectrumGrabber.uir", CFPANEL));
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

void gotoUiState(int panelHandle, struct psconfig psConfig, enum uiState state)
{
	char menuItemName[32];
	switch (state)
	{
		case UI_NO_SCOPE:
			for (int i = 0;i < 4;i++) {
				// Turn off LEDs
				SetCtrlVal(panelHandle, channelLeds[i], 0);
				SetCtrlVal(panelHandle, overloadLeds[i], 0);
				
				// Uncheck measurement checkboxes
				SetCtrlVal(panelHandle, channelMeasFreq[i], 0);
				SetCtrlVal(panelHandle, channelMeasTime[i], 0);
				
				// Hide the channel elements 
				SetCtrlAttribute(panelHandle, channelLabels[i], ATTR_VISIBLE, FALSE);     
				SetCtrlAttribute(panelHandle, channelLeds[i], ATTR_VISIBLE, FALSE);
				SetCtrlAttribute(panelHandle, channelRanges[i], ATTR_VISIBLE, FALSE);
				SetCtrlAttribute(panelHandle, channelCouplings[i], ATTR_VISIBLE, FALSE);
				SetCtrlAttribute(panelHandle, channelCoeffs[i], ATTR_VISIBLE, FALSE);
				SetCtrlAttribute(panelHandle, channelMeasFreq[i], ATTR_VISIBLE, FALSE);
				SetCtrlAttribute(panelHandle, channelMeasTime[i], ATTR_VISIBLE, FALSE);
				SetCtrlAttribute(panelHandle, overloadLeds[i], ATTR_VISIBLE, FALSE);
				
				// Clear channel menus
				ClearListCtrl(panelHandle, channelRanges[i]);
				ClearListCtrl(panelHandle, channelCouplings[i]);
				
				// Disable save limits
				SetCtrlAttribute(panelHandle, MAINPANEL_MINFREQ, ATTR_DIMMED, TRUE);
				SetCtrlAttribute(panelHandle, MAINPANEL_MAXFREQ, ATTR_DIMMED, TRUE);
				SetCtrlAttribute(panelHandle, MAINPANEL_MINTIME, ATTR_DIMMED, TRUE);
				SetCtrlAttribute(panelHandle, MAINPANEL_MAXTIME, ATTR_DIMMED, TRUE);
				
				// Disable run buttons
				SetCtrlAttribute(panelHandle, MAINPANEL_RUNBUTTON, ATTR_DIMMED, TRUE);
				SetCtrlAttribute(panelHandle, MAINPANEL_NEXTBUTTON, ATTR_DIMMED, TRUE);
				SetCtrlAttribute(panelHandle, MAINPANEL_STOPBUTTON, ATTR_DIMMED, TRUE);
			}
			break;
		case UI_NEW_SCOPE:
			for (int i = 0; i < psConfig.nChannels;i++) {
				// Turn off LEDs
				SetCtrlVal(panelHandle, channelLeds[i], 0);
				SetCtrlVal(panelHandle, overloadLeds[i], 0);
				
				// Uncheck measurement checkboxes
				SetCtrlVal(panelHandle, channelMeasFreq[i], 0);
				SetCtrlVal(panelHandle, channelMeasTime[i], 0);
				
				// Enable channels
				SetCtrlAttribute(panelHandle, channelLabels[i], ATTR_VISIBLE, TRUE);
				SetCtrlAttribute(panelHandle, channelLeds[i], ATTR_VISIBLE, TRUE);
				SetCtrlAttribute(panelHandle, channelRanges[i], ATTR_VISIBLE, TRUE);
				SetCtrlAttribute(panelHandle, channelCouplings[i], ATTR_VISIBLE, TRUE);
				SetCtrlAttribute(panelHandle, channelCoeffs[i], ATTR_VISIBLE, TRUE);
				SetCtrlAttribute(panelHandle, channelMeasFreq[i], ATTR_VISIBLE, TRUE);
				SetCtrlAttribute(panelHandle, channelMeasTime[i], ATTR_VISIBLE, TRUE);
				SetCtrlAttribute(panelHandle, overloadLeds[i], ATTR_VISIBLE, TRUE);
				
				// Undim checkboxes and channel LED
				SetCtrlAttribute(panelHandle, channelLabels[i], ATTR_DIMMED, FALSE);
				SetCtrlAttribute(panelHandle, channelLeds[i], ATTR_DIMMED, FALSE);
				SetCtrlAttribute(panelHandle, channelMeasFreq[i], ATTR_DIMMED, FALSE);
				SetCtrlAttribute(panelHandle, channelMeasTime[i], ATTR_DIMMED, FALSE);
				
				// Dim remaining channel elements
				SetCtrlAttribute(panelHandle, channelRanges[i], ATTR_DIMMED, TRUE);
				SetCtrlAttribute(panelHandle, channelCouplings[i], ATTR_DIMMED, TRUE);
				SetCtrlAttribute(panelHandle, channelCoeffs[i], ATTR_DIMMED, TRUE);
				SetCtrlAttribute(panelHandle, overloadLeds[i], ATTR_DIMMED, TRUE);
				
				// Clear all channel menus  
				ClearListCtrl(panelHandle, channelRanges[i]);
				ClearListCtrl(panelHandle, channelCouplings[i]);
				
				// Fill in ranges
				for (int j = 0;j < psConfig.nRanges;j++) {
					getRangeLabel(psConfig.ranges[j], menuItemName);
					InsertListItem(panelHandle, channelRanges[i], j, menuItemName, psConfig.ranges[j]);
				}
				
				// Fill in couplings
				for (int j = 0;j < psConfig.nCouplings;j++) {
					getCouplingLabel(psConfig.couplings[j], menuItemName);
					InsertListItem(panelHandle, channelCouplings[i], j, menuItemName, psConfig.couplings[j]);
				}
			}
			
			// Disable other channels
			for (int i = psConfig.nChannels;i < 4;i++) {
				SetCtrlAttribute(panelHandle, channelLabels[i], ATTR_VISIBLE, FALSE);
				SetCtrlAttribute(panelHandle, channelLeds[i], ATTR_VISIBLE, FALSE);
				SetCtrlAttribute(panelHandle, channelRanges[i], ATTR_VISIBLE, FALSE);
				SetCtrlAttribute(panelHandle, channelCouplings[i], ATTR_VISIBLE, FALSE);
				SetCtrlAttribute(panelHandle, channelCoeffs[i], ATTR_VISIBLE, FALSE);
				SetCtrlAttribute(panelHandle, channelMeasFreq[i], ATTR_VISIBLE, FALSE);
				SetCtrlAttribute(panelHandle, channelMeasTime[i], ATTR_VISIBLE, FALSE);
				SetCtrlAttribute(panelHandle, overloadLeds[i], ATTR_VISIBLE, FALSE);
			}

			// Go to idle state
			gotoUiState(panelHandle, psConfig, UI_IDLE);
			break;
		case UI_IDLE:
			// Enable run buttons
			SetCtrlAttribute(panelHandle, MAINPANEL_RUNBUTTON, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(panelHandle, MAINPANEL_NEXTBUTTON, ATTR_DIMMED, TRUE);
			SetCtrlAttribute(panelHandle, MAINPANEL_STOPBUTTON, ATTR_DIMMED, TRUE);
			
			// Enable save limits
			SetCtrlAttribute(panelHandle, MAINPANEL_MINFREQ, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(panelHandle, MAINPANEL_MAXFREQ, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(panelHandle, MAINPANEL_MINTIME, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(panelHandle, MAINPANEL_MAXTIME, ATTR_DIMMED, FALSE);
			
			// Enable run settings
			SetCtrlAttribute(panelHandle, MAINPANEL_BINSRING, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(panelHandle, MAINPANEL_RATEBOX, ATTR_DIMMED, FALSE);
			
			// Enable channel settings
			for (int i = 0;i < psConfig.nChannels;i++) {
				SetCtrlAttribute(panelHandle, channelLabels[i], ATTR_DIMMED, FALSE);
				SetCtrlAttribute(panelHandle, channelRanges[i], ATTR_DIMMED, !isChannelEnabled(i));
				SetCtrlAttribute(panelHandle, channelCouplings[i], ATTR_DIMMED, !isChannelEnabled(i));
				SetCtrlAttribute(panelHandle, channelCoeffs[i], ATTR_DIMMED, !isChannelEnabled(i));
				SetCtrlAttribute(panelHandle, channelMeasFreq[i], ATTR_DIMMED, FALSE);
				SetCtrlAttribute(panelHandle, channelMeasTime[i], ATTR_DIMMED, FALSE);
			}
			
			// Enable bias points table
			SetCtrlAttribute(panelHandle, MAINPANEL_GENERATEBUTTON, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(panelHandle, MAINPANEL_LOADBUTTON, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(panelHandle, MAINPANEL_CLEARBUTTON, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(panelHandle, MAINPANEL_ADDROWBUTTON, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(panelHandle, MAINPANEL_DELROWBUTTON, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(panelHandle, MAINPANEL_SAVETABLEBUTTON, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(panelHandle, MAINPANEL_DACBUTTON, ATTR_DIMMED, FALSE);
			break;
		case UI_MEASURING:
			// Disable run buttons
			SetCtrlAttribute(panelHandle, MAINPANEL_RUNBUTTON, ATTR_DIMMED, TRUE);
			SetCtrlAttribute(panelHandle, MAINPANEL_NEXTBUTTON, ATTR_DIMMED, FALSE);
			SetCtrlAttribute(panelHandle, MAINPANEL_STOPBUTTON, ATTR_DIMMED, FALSE);
			
			// Disable save limits
			SetCtrlAttribute(panelHandle, MAINPANEL_MINFREQ, ATTR_DIMMED, TRUE);
			SetCtrlAttribute(panelHandle, MAINPANEL_MAXFREQ, ATTR_DIMMED, TRUE);
			SetCtrlAttribute(panelHandle, MAINPANEL_MINTIME, ATTR_DIMMED, TRUE);
			SetCtrlAttribute(panelHandle, MAINPANEL_MAXTIME, ATTR_DIMMED, TRUE);
			
			// Disable run settings
			SetCtrlAttribute(panelHandle, MAINPANEL_BINSRING, ATTR_DIMMED, TRUE);
			SetCtrlAttribute(panelHandle, MAINPANEL_RATEBOX, ATTR_DIMMED, TRUE);
			
			// Disable channel settings
			for (int i = 0;i < psConfig.nChannels;i++) {
				SetCtrlAttribute(panelHandle, channelLabels[i], ATTR_DIMMED, TRUE);
				SetCtrlAttribute(panelHandle, channelRanges[i], ATTR_DIMMED, TRUE);
				SetCtrlAttribute(panelHandle, channelCouplings[i], ATTR_DIMMED, TRUE);
				SetCtrlAttribute(panelHandle, channelCoeffs[i], ATTR_DIMMED, TRUE);
				SetCtrlAttribute(panelHandle, channelMeasFreq[i], ATTR_DIMMED, TRUE);
				SetCtrlAttribute(panelHandle, channelMeasTime[i], ATTR_DIMMED, TRUE);
			}
			
			// Disable bias points table
			SetCtrlAttribute(panelHandle, MAINPANEL_GENERATEBUTTON, ATTR_DIMMED, TRUE);
			SetCtrlAttribute(panelHandle, MAINPANEL_LOADBUTTON, ATTR_DIMMED, TRUE);
			SetCtrlAttribute(panelHandle, MAINPANEL_CLEARBUTTON, ATTR_DIMMED, TRUE);
			SetCtrlAttribute(panelHandle, MAINPANEL_ADDROWBUTTON, ATTR_DIMMED, TRUE);
			SetCtrlAttribute(panelHandle, MAINPANEL_DELROWBUTTON, ATTR_DIMMED, TRUE);
			SetCtrlAttribute(panelHandle, MAINPANEL_SAVETABLEBUTTON, ATTR_DIMMED, TRUE);
			SetCtrlAttribute(panelHandle, MAINPANEL_DACBUTTON, ATTR_DIMMED, TRUE);
			break;
	}
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
		psCloseUnit(&psConfig);
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
