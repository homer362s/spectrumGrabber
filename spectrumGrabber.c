
#include "cbw.h" 
#include <analysis.h>
#include <ansi_c.h>
#include <cvirte.h>		
#include <userint.h>
#include "spectrumGrabber.h"
#include "toolbox.h"
#include "ps6000Api.h"


//#define measuredPoints 20000

int measuredPoints = 0;

//uint32_t timebase = 1566;
uint32_t timebase = 7816;
int16_t *rawDataBuffer;   // Stores data from the scope
double *zeros;			  // Stores zeros for FFT
double *dataValues;		  // Stores specctrum data for a single measurement
double *avgSpectrum;	  // Stores the average specturm
float *timeValues;
float *freqValues;

char measuring = 0;

static int panelHandle = 0;
int16_t scopeHandle;

void picoscopeInit();

int main (int argc, char *argv[])
{
	int error = 0;
	
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
	float timeInterval_ns = 0;
	ps6000GetTimebase2(scopeHandle, timebase, measuredPoints, &timeInterval_ns, 0, NULL, 0);
	int time = timeInterval_ns;
	for (int i = 0;i < measuredPoints;i++) {
		//timeValues[i] = 1/(float) (time/1e9);
		timeValues[i] = time/1e9;
		freqValues[i] = i/(measuredPoints*timeInterval_ns/1e9);
		time += timeInterval_ns;
	}

	// Set up the measurement
	status = ps6000SetChannel(scopeHandle, PS6000_CHANNEL_A, TRUE, PS6000_DC_1M, PS6000_50MV, 0, PS6000_BW_20MHZ); 
	status = ps6000SetChannel(scopeHandle, PS6000_CHANNEL_B, FALSE, PS6000_DC_1M, PS6000_100MV, 0, PS6000_BW_20MHZ); 
	status = ps6000SetChannel(scopeHandle, PS6000_CHANNEL_C, FALSE, PS6000_DC_1M, PS6000_100MV, 0, PS6000_BW_20MHZ); 
	status = ps6000SetChannel(scopeHandle, PS6000_CHANNEL_D, FALSE, PS6000_DC_1M, PS6000_100MV, 0, PS6000_BW_20MHZ); 
	
	// Set up data buffer
	status = ps6000SetDataBuffer(scopeHandle, PS6000_CHANNEL_A, rawDataBuffer, measuredPoints, PS6000_RATIO_MODE_NONE);
	
}

void PREF4 dataAvailableCallback(int16_t handle, PICO_STATUS status, void* pParameter)
{
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
			
			// Update the data bufer
			ps6000SetDataBuffer(scopeHandle, PS6000_CHANNEL_A, rawDataBuffer, measuredPoints, PS6000_RATIO_MODE_NONE);
			break;
	}
	
	return 0;
}

void handleMeasurement()
{
	PICO_STATUS status;
	short averages = 1;
	int nMeasured = 0;
	char tmpstr[128];
	
	// Get number of requested averages
	GetCtrlVal(panelHandle, MAINPANEL_AVGBOX, &averages);
	
	sprintf(tmpstr, "0/%d Completed", averages);
	SetCtrlVal(panelHandle, MAINPANEL_MEASCOUNTDISP, tmpstr);
	
	for(nMeasured = 0; nMeasured < averages; nMeasured++) {
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
		}
	
		// Convert values to double
		for (int i = 0;i < measuredPoints;i++) {
			dataValues[i] = (double) rawDataBuffer[i] / 32767 * 0.05;
		}
	
		// Take FFT
		for(int i = 0;i < measuredPoints; i++) {
			zeros[i] = 0;
		}
		FFT(dataValues, zeros, measuredPoints);
	
		// From now on we only care about the first half of the points
		for(int i = 0;i < measuredPoints/2; i++) {
			// Get magnitude
			dataValues[i] = 20*log10((sqrt(pow(dataValues[i],2) + pow(zeros[i],2))) / measuredPoints);
		
			// Average spectra
			avgSpectrum[i] = (avgSpectrum[i] * ((double) nMeasured / (double) averages) + dataValues[i] * (1 / (double) averages)) * ((double) averages) / ((double) nMeasured + 1);
		}
	
		// Calculate time related stuff
		float timeInterval_ns = 0;
		ps6000GetTimebase2(scopeHandle, timebase, measuredPoints, &timeInterval_ns, 0, NULL, 0);
		int time = timeInterval_ns;
		for (int i = 0;i < measuredPoints/2;i++) {
			timeValues[i] = time/1e9;
			freqValues[i] = i/(measuredPoints*timeInterval_ns/1e9);
			time += timeInterval_ns;
		}
	
		DeleteGraphPlot(panelHandle, MAINPANEL_GRAPH, -1, VAL_DELAYED_DRAW);
		PlotXY(panelHandle, MAINPANEL_GRAPH, freqValues, avgSpectrum, measuredPoints/2, VAL_FLOAT, VAL_DOUBLE, VAL_THIN_LINE, VAL_NO_POINT, VAL_SOLID, 1, VAL_BLACK); 
		
		// Update progress counter
		sprintf(tmpstr, "%d/%d Completed", nMeasured + 1, averages);
		SetCtrlVal(panelHandle, MAINPANEL_MEASCOUNTDISP, tmpstr);
	}
}

int CVICALLBACK runButton_CB(int panel, int control, int event, void *callbackData, int eventData1, int eventData2)
{
	switch (event) {
		case EVENT_COMMIT:
			handleMeasurement();
			break;
	}
	
	return 0;
}

int CVICALLBACK mainpanel_CB (int panel, int event, void *callbackData,
		int eventData1, int eventData2)
{
	if (event == EVENT_CLOSE) {
		ps6000CloseUnit(scopeHandle);
		QuitUserInterface (0);
	}
	return 0;
}
