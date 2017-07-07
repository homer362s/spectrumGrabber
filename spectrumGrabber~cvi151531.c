
#include <analysis.h>
#include <ansi_c.h>
#include <cvirte.h>		
#include <userint.h>
#include "spectrumGrabber.h"
#include "toolbox.h"
#include "ps6000Api.h"

//#define TESTPOINTS 20000

int measuredPoints = 0;

//uint32_t timebase = 1566;
uint32_t timebase = 7816;
int16_t *dataBuffer;
double *zeros;
double *dataValues;
float *timeValues;
float *freqValues;

//int16_t dataBuffer[measuredPoints];
//double zeros[measuredPoints];
//double dataValues[measuredPoints];
//float timeValues[measuredPoints];
//float freqValues[measuredPoints];

static int panelHandle = 0;
int16_t scopeHandle;

void picoscopeInit();

int main (int argc, char *argv[])
{
	int error = 0;
	
	/* initialize and load resources */
	nullChk (InitCVIRTE (0, argv, 0));
	errChk (panelHandle = LoadPanel (0, "spectrumGrabber.uir", MAINPANEL));
	
	// Initialize picoscope
	//picoscopeInit();
	
	// ***********************************
	
	int16_t scopeCount;
	int8_t serials[128];
	int16_t serialLth = 256;

	int16_t handle;
	int8_t *serial;

	PICO_STATUS status;

	//Create initial arrays
	GetCtrlVal(panelHandle, MAINPANEL_BINSRING, &measuredPoints);
	measuredPoints = measuredPoints * 2;
	
	dataBuffer = malloc(measuredPoints * sizeof(int16_t));
	zeros = malloc(measuredPoints * sizeof(double));
	dataValues = malloc(measuredPoints * sizeof(double));
	timeValues = malloc(measuredPoints * sizeof(float));
	freqValues = malloc(measuredPoints * sizeof(float));
	
	// Get list of connected picoscopes 
	status = ps6000EnumerateUnits(&scopeCount, serials, &serialLth);

	switch (status) {
		case PICO_OK:
			break;
		default:
			break;
	}

	// Open the first picoscope (probably only works if there is just a single scope)
	serial = serials;
	status = ps6000OpenUnit(&handle, serial);

	switch (status) {
		case PICO_OK:
			break;
		default:
			break;
	}

	// Initialize some values
	float timeInterval_ns = 0;
	ps6000GetTimebase2(handle, timebase, measuredPoints, &timeInterval_ns, 0, NULL, 0);
	int time = timeInterval_ns;
	for (int i = 0;i < measuredPoints;i++) {
		//timeValues[i] = 1/(float) (time/1e9);
		timeValues[i] = time/1e9;
		f
		reqValues[i] = i/(measuredPoints*timeInterval_ns/1e9);
		time += timeInterval_ns;
	}

	// Set up the measurement
	status = ps6000SetChannel(handle, PS6000_CHANNEL_A, TRUE, PS6000_DC_1M, PS6000_50MV, 0, PS6000_BW_20MHZ); 
	status = ps6000SetChannel(handle, PS6000_CHANNEL_B, FALSE, PS6000_DC_1M, PS6000_100MV, 0, PS6000_BW_20MHZ); 
	status = ps6000SetChannel(handle, PS6000_CHANNEL_C, FALSE, PS6000_DC_1M, PS6000_100MV, 0, PS6000_BW_20MHZ); 
	status = ps6000SetChannel(handle, PS6000_CHANNEL_D, FALSE, PS6000_DC_1M, PS6000_100MV, 0, PS6000_BW_20MHZ); 
	
	scopeHandle = handle;
	
	// ***********************************
	
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
	int16_t scopeCount;
	int8_t serials[128];
	int16_t serialLth = 256;

	int16_t handle;
	int8_t *serial;

	PICO_STATUS status;

	//Create initial arrays
	GetCtrlVal(panelHandle, MAINPANEL_BINSRING, &measuredPoints);
	measuredPoints = measuredPoints * 2;
	
	dataBuffer = malloc(measuredPoints * sizeof(int16_t));
	zeros = malloc(measuredPoints * sizeof(double));
	dataValues = malloc(measuredPoints * sizeof(double));
	timeValues = malloc(measuredPoints * sizeof(float));
	freqValues = malloc(measuredPoints * sizeof(float));
	
	// Get list of connected picoscopes 
	status = ps6000EnumerateUnits(&scopeCount, serials, &serialLth);

	switch (status) {
		case PICO_OK:
			break;
		default:
			break;
	}

	// Open the first picoscope (probably only works if there is just a single scope)
	serial = serials;
	status = ps6000OpenUnit(&handle, serial);

	switch (status) {
		case PICO_OK:
			break;
		default:
			break;
	}

	// Initialize some values
	float timeInterval_ns = 0;
	ps6000GetTimebase2(handle, timebase, measuredPoints, &timeInterval_ns, 0, NULL, 0);
	int time = timeInterval_ns;
	for (int i = 0;i < measuredPoints;i++) {
		//timeValues[i] = 1/(float) (time/1e9);
		timeValues[i] = time/1e9;
		freqValues[i] = i/(measuredPoints*timeInterval_ns/1e9);
		time += timeInterval_ns;
	}

	// Set up the measurement
	status = ps6000SetChannel(handle, PS6000_CHANNEL_A, TRUE, PS6000_DC_1M, PS6000_50MV, 0, PS6000_BW_20MHZ); 
	status = ps6000SetChannel(handle, PS6000_CHANNEL_B, FALSE, PS6000_DC_1M, PS6000_100MV, 0, PS6000_BW_20MHZ); 
	status = ps6000SetChannel(handle, PS6000_CHANNEL_C, FALSE, PS6000_DC_1M, PS6000_100MV, 0, PS6000_BW_20MHZ); 
	status = ps6000SetChannel(handle, PS6000_CHANNEL_D, FALSE, PS6000_DC_1M, PS6000_100MV, 0, PS6000_BW_20MHZ); 
	
	scopeHandle = handle;
}

void PREF4 dataAvailableCallback(int16_t handle, PICO_STATUS status, void* pParameter)
{
	uint32_t nSamples = measuredPoints;

	status = ps6000SetDataBuffer(handle, PS6000_CHANNEL_A, dataBuffer, measuredPoints, PS6000_RATIO_MODE_NONE);
	
	status = ps6000GetValues(handle, 0, &nSamples, 1, PS6000_RATIO_MODE_NONE, 0, NULL);
	
	// Convert values to float or something like that
	for (int i = 0;i < measuredPoints;i++) {
		dataValues[i] = (double) dataBuffer[i] / 32767 * 0.05;
	}
	
	// Take FFT
	for(int i = 0;i < measuredPoints; i++) {
		zeros[i] = 0;
	}
	FFT(dataValues, zeros, measuredPoints);
	
	// Get magnitude
	for(int i = 0;i < measuredPoints; i++) {
		dataValues[i] = 20*log10((sqrt(pow(dataValues[i],2) + pow(zeros[i],2))) / measuredPoints);
	}
	
	// Calculate time related stuff
	float timeInterval_ns = 0;
	ps6000GetTimebase2(handle, timebase, measuredPoints, &timeInterval_ns, 0, NULL, 0);
	int time = timeInterval_ns;
	for (int i = 0;i < measuredPoints;i++) {
		//timeValues[i] = 1/(float) (time/1e9);
		timeValues[i] = time/1e9;
		freqValues[i] = i/(measuredPoints*timeInterval_ns/1e9);
		time += timeInterval_ns;
	}
	
	DeleteGraphPlot(panelHandle, MAINPANEL_GRAPH, -1, VAL_DELAYED_DRAW);
	PlotXY(panelHandle, MAINPANEL_GRAPH, freqValues, dataValues, measuredPoints/2, VAL_FLOAT, VAL_DOUBLE, VAL_THIN_LINE, VAL_NO_POINT, VAL_SOLID, 1, VAL_BLACK); 
	//PlotY(panelHandle, MAINPANEL_GRAPH, freqValues, measuredPoints, VAL_FLOAT, VAL_THIN_LINE, VAL_NO_POINT, VAL_SOLID, 1, VAL_BLACK);
	Delay(0.01);
}

int CVICALLBACK binsRing_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			//Reallocate Arrays
			GetCtrlVal(panelHandle, MAINPANEL_BINSRING, &measuredPoints);
			measuredPoints = measuredPoints * 2;
	
			dataBuffer = realloc(dataBuffer, measuredPoints * sizeof(int16_t));
			zeros = realloc(zeros, measuredPoints * sizeof(double));
			dataValues = realloc(dataValues, measuredPoints * sizeof(double));
			timeValues = realloc(timeValues, measuredPoints * sizeof(float));
			freqValues = realloc(freqValues, measuredPoints * sizeof(float));
			break;
	}
	
	return 0;
}

int CVICALLBACK runButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			PICO_STATUS status;
			
			
			status = ps6000RunBlock(scopeHandle, measuredPoints/2, measuredPoints/2, timebase, 1, 0, 0, dataAvailableCallback, 0);
			
			switch (status) {
				case PICO_OK:
					break;
				default:
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
		// Free memory
		ps6000CloseUnit(scopeHandle);
		QuitUserInterface (0);
	}
	return 0;
}
