
#include <analysis.h>
#include <ansi_c.h>
#include <cvirte.h>		
#include <userint.h>
#include "spectrumGrabber.h"
#include "toolbox.h"
#include "ps6000Api.h"

#define TESTPOINTS 20000

int measuredPoints = 0;

uint32_t timebase = 1566;
//int16_t *dataBuffer;
//double *zeros;
//double *dataValues;
//float *timeValues;
//float *freqValues;

int16_t dataBuffer[TESTPOINTS];
double zeros[TESTPOINTS];
double dataValues[TESTPOINTS];
float timeValues[TESTPOINTS];
float freqValues[TESTPOINTS];

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
	picoscopeInit();
	
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

	// 
	
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
	ps6000GetTimebase2(handle, timebase, TESTPOINTS, &timeInterval_ns, 0, NULL, 0);
	int time = timeInterval_ns;
	for (int i = 0;i < TESTPOINTS;i++) {
		//timeValues[i] = 1/(float) (time/1e9);
		timeValues[i] = time/1e9;
		freqValues[i] = i/(TESTPOINTS*timeInterval_ns/1e9);
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
	uint32_t nSamples = TESTPOINTS;

	status = ps6000SetDataBuffer(handle, PS6000_CHANNEL_A, dataBuffer, TESTPOINTS, PS6000_RATIO_MODE_NONE);
	
	status = ps6000GetValues(handle, 0, &nSamples, 1, PS6000_RATIO_MODE_NONE, 0, NULL);
	
	// Convert values to float or something like that
	for (int i = 0;i < TESTPOINTS;i++) {
		dataValues[i] = (double) dataBuffer[i] / 32767 * 0.05;
	}
	
	// Take FFT
	for(int i = 0;i < TESTPOINTS; i++) {
		zeros[i] = 0;
	}
	FFT(dataValues, zeros, TESTPOINTS);
	
	// Get magnitude
	for(int i = 0;i < TESTPOINTS; i++) {
		dataValues[i] = 20*log10((sqrt(pow(dataValues[i],2) + pow(zeros[i],2))) / TESTPOINTS);
	}
	
	DeleteGraphPlot(panelHandle, MAINPANEL_GRAPH, -1, VAL_DELAYED_DRAW);
	PlotXY(panelHandle, MAINPANEL_GRAPH, freqValues, dataValues, TESTPOINTS/2, VAL_FLOAT, VAL_DOUBLE, VAL_THIN_LINE, VAL_NO_POINT, VAL_SOLID, 1, VAL_BLACK); 
	//PlotY(panelHandle, MAINPANEL_GRAPH, freqValues, TESTPOINTS, VAL_FLOAT, VAL_THIN_LINE, VAL_NO_POINT, VAL_SOLID, 1, VAL_BLACK);
	Delay(0.01);
}

int CVICALLBACK binsRing_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			break;
	}
	
	return 0;
}

int CVICALLBACK runButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			PICO_STATUS status;
			
			status = ps6000RunBlock(scopeHandle, TESTPOINTS/2, TESTPOINTS/2, timebase, 1, 0, 0, dataAvailableCallback, 0);
			
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
	if (event == EVENT_CLOSE)
		QuitUserInterface (0);
	return 0;
}
