#include <ansi_c.h>
#include <utility.h>
#include "ps6000Api.h"
#include "ps3000aApi.h"
#include "ps4000Api.h"
#include "picoscopes.h"
#include <stddef.h>
#include <math.h>

#define FALSE 0
#define TRUE 1

void getRangeLabel(enum psRange range, char *label)
{
	char *tmp;
	switch (range) {
		case PS_10MV:
			tmp = "10 mV";
			break;
		case PS_20MV:
			tmp = "20 mV";
			break;
		case PS_50MV:
			tmp = "50 mV";
			break;
		case PS_100MV:
			tmp = "100 mV";
			break;
		case PS_200MV:
			tmp = "200 mV";
			break;
		case PS_500MV:
			tmp = "500 mV";
			break;
		case PS_1V:
			tmp = "1 V";
			break;
		case PS_2V:
			tmp = "2 V";
			break;
		case PS_5V:
			tmp = "5 V";
			break;
		case PS_10V:
			tmp = "10 V";
			break;
		case PS_20V:
			tmp = "20 V";
			break;
		case PS_50V:
			tmp = "50 V";
			break;
		case PS_100V:
			tmp = "100 V";
			break;
	}
	
	strcpy(label, tmp);
}

void getCouplingLabel(enum psCoupling coupling, char *label)
{
	char *tmp;
	switch (coupling) {
		case PS_DC:
			tmp = "DC";
			break;
		case PS_AC:
			tmp = "AC";
			break;
	}
	strcpy(label, tmp);
}

double getRangeValue(enum psRange range)
{
	switch (range) {
		case PS_10MV:
			return 0.01;
		case PS_20MV:
			return 0.02;
		case PS_50MV:
			return 0.05;
		case PS_100MV:
			return 0.1;
		case PS_200MV:
			return 0.2;
		case PS_500MV:
			return 0.5;
		case PS_1V:
			return 1;
		case PS_2V:
			return 2;
		case PS_5V:
			return 5;
		case PS_10V:
			return 10;
		case PS_20V:
			return 20;
		case PS_50V:
			return 50;
		case PS_100V:
			return 100;
	}
}

// Convert the general channel into an API specific channel
int convertChannel(enum scopeType type, enum psChannel channel) 
{
	switch (type) {
		case PS4000:
			switch (channel) {
				case PS_CHANNEL_A:
					return PS4000_CHANNEL_A;
				case PS_CHANNEL_B:
					return PS4000_CHANNEL_B;
				case PS_CHANNEL_C:
					return PS4000_CHANNEL_C;
				case PS_CHANNEL_D:
					return PS4000_CHANNEL_D;
			}
		case PS3000A:
			switch (channel) {
				case PS_CHANNEL_A:
					return PS3000A_CHANNEL_A;
				case PS_CHANNEL_B:
					return PS3000A_CHANNEL_B;
				case PS_CHANNEL_C:
					return PS3000A_CHANNEL_C;
				case PS_CHANNEL_D:
					return PS3000A_CHANNEL_D;
			}
		case PS6000:
			switch (channel) {
				case PS_CHANNEL_A:
					return PS6000_CHANNEL_A;
				case PS_CHANNEL_B:
					return PS6000_CHANNEL_B;
				case PS_CHANNEL_C:
					return PS6000_CHANNEL_C;
				case PS_CHANNEL_D:
					return PS6000_CHANNEL_D;
			}
	}
	return -1;
}

// Convert the general coupling into an API specific coupling 
int convertCoupling(enum scopeType type, enum psCoupling coupling)
{
	switch (type) {
		case PS4000:
			switch (coupling) {
				case PS_AC:
					return FALSE;
				case PS_DC:
					return TRUE;
			}
		case PS3000A:
			switch (coupling) {
				case PS_AC:
					return PS3000A_AC;
				case PS_DC:
					return PS3000A_DC;
			}
		case PS6000:
			switch (coupling) {
				case PS_AC:
					return PS6000_AC;
				case PS_DC:
					return PS6000_DC_1M;
			}
	}
	return -1;
}

// Convert the general range into an API specific range 
int convertRange(enum scopeType type, enum psRange range)
{
	switch (type) {
		case PS4000:
			switch (range) {
				case PS_10MV:
					return PS4000_10MV;
				case PS_20MV:
					return PS4000_20MV;
				case PS_50MV:
					return PS4000_50MV;
				case PS_100MV:
					return PS4000_100MV;
				case PS_200MV:
					return PS4000_200MV;
				case PS_500MV:
					return PS4000_500MV;
				case PS_1V:
					return PS4000_1V;
				case PS_2V:
					return PS4000_2V;
				case PS_5V:
					return PS4000_5V;
				case PS_10V:
					return PS4000_10V;
				case PS_20V:
					return PS4000_20V;
				case PS_50V:
					return PS4000_50V;
				case PS_100V:
					return PS4000_100V;
			}
		case PS3000A:
			switch (range) {
				case PS_50MV:
					return PS3000A_50MV;
				case PS_100MV:
					return PS3000A_100MV;
				case PS_200MV:
					return PS3000A_200MV;
				case PS_500MV:
					return PS3000A_500MV;
				case PS_1V:
					return PS3000A_1V;
				case PS_2V:
					return PS3000A_2V;
				case PS_5V:
					return PS3000A_5V;
				case PS_10V:
					return PS3000A_10V;
				case PS_20V:
					return PS3000A_20V;
			}
		case PS6000:
			switch (range) {
				case PS_50MV:
					return PS6000_50MV;
				case PS_100MV:
					return PS6000_100MV;
				case PS_200MV:
					return PS6000_200MV;
				case PS_500MV:
					return PS6000_500MV;
				case PS_1V:
					return PS6000_1V;
				case PS_2V:
					return PS6000_2V;
				case PS_5V:
					return PS6000_5V;
				case PS_10V:
					return PS6000_10V;
				case PS_20V:
					return PS6000_20V;
			}
	}
	return -1;
}

void scaleReading(struct psconfig *config, int channel, int16_t *rawData, double *scaledData)
{
	int16_t maxValue = 1;
	switch (config->type) {
		case PS4000:
			maxValue = PS4000_MAX_VALUE;
			break;
		case PS3000A:
			ps3000aMaximumValue(config->handle, &maxValue);
			break;
		case PS6000:
			maxValue = PS6000_MAX_VALUE;
			break;
	}
	
	for (int i = 0;i < config->nPoints;i++) {
		scaledData[i] = (double) rawData[i] / maxValue * getRangeValue(config->channels[channel].range) / config->channels[channel].coefficient;
	}
}

PICO_STATUS psOpenUnit(struct psconfig *config)
{
	PICO_STATUS status = -1;
	
	psCloseUnit(config);
	switch(config->type) {
		case PS4000:
			//ps4000CloseUnit(config->handle);
			status = ps4000OpenUnit(&(config->handle));
			break;
		case PS3000A:
			//ps3000aCloseUnit(config->handle);
			status = ps3000aOpenUnit(&(config->handle), config->serial);
			break;	
		case PS6000:
			//ps6000CloseUnit(config->handle);
			status = ps6000OpenUnit(&(config->handle), config->serial);
			break;
	}
	return status;
}

PICO_STATUS psCloseUnit(struct psconfig *config)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS4000:
			status = ps4000CloseUnit(config->handle);
			break;
		case PS3000A:
			status = ps3000aCloseUnit(config->handle);
			break;	
		case PS6000:
			status = ps6000CloseUnit(config->handle);
			break;
	}
	return status;
}

// TODO: The PS3000A and PS6000 timebase functions are incomplete, finish these
// Update config.timebase according the the sample rate and type of picoscope
void psUpdateTimebase(struct psconfig *config, float sampleRate)
{
	switch (config->type) {
		case PS4000:
			if (sampleRate < 48e-9) {
				config->timebase = RoundRealToNearestInteger(sqrt(250000000/sampleRate));
				if (config->timebase < 0)
					config->timebase = 0;
				if (config->timebase > 3)
					config->timebase = 3;
			} else {
				config->timebase = RoundRealToNearestInteger(31250000/sampleRate + 2);
				if (config->timebase < 4)
					config->timebase = 4;
				if (config->timebase > (1<<30) - 1)
					config->timebase = (1<<30) - 1;
			}
			break;
		case PS3000A:
			config->timebase = RoundRealToNearestInteger(125000000/sampleRate + 2);
			break;
		case PS6000:
			config->timebase = RoundRealToNearestInteger(156250000/sampleRate + 4);
			break;
	}
}

PICO_STATUS psGetTimebase2(struct psconfig *config, float *timeInterval_ns)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS4000:
			status = ps4000GetTimebase2(config->handle, config->timebase, config->nPoints, timeInterval_ns, 0, NULL, 0);
			break;
		case PS3000A:
			status = ps3000aGetTimebase2(config->handle, config->timebase, config->nPoints, timeInterval_ns, 0, NULL, 0);
			break;	
		case PS6000:
			status = ps6000GetTimebase2(config->handle, config->timebase, config->nPoints, timeInterval_ns, 0, NULL, 0);
			break;
	}
	return status;
}

PICO_STATUS psRunBlock(struct psconfig *config, void *dataAvailableCallback)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS4000:
			status = ps4000RunBlock(config->handle, 0, config->nPoints, config->timebase, 1, NULL, 0, dataAvailableCallback, NULL);
			break;
		case PS3000A:
			status = ps3000aRunBlock(config->handle, 0, config->nPoints, config->timebase, 1, 0, 0, dataAvailableCallback, NULL);
			break;	
		case PS6000:
			status = ps6000RunBlock(config->handle, 0, config->nPoints, config->timebase, 1, 0, 0, dataAvailableCallback, NULL);
			break;
	}
	return status;
}

PICO_STATUS psStop(struct psconfig *config)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS4000:
			status = ps4000Stop(config->handle);
			break;
		case PS3000A:
			status = ps3000aStop(config->handle);
			break;	
		case PS6000:
			status = ps6000Stop(config->handle);
			break;
	}
	return status;
}


// TODO: Make this return the number of samples so we can properly update the UI
PICO_STATUS psMemorySegments(struct psconfig *config)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS4000:
			status = ps4000MemorySegments(config->handle, 1, NULL);
			break;
		case PS3000A:
			status = ps3000aMemorySegments(config->handle, 1, NULL);
			break;	
		case PS6000:
			status = ps6000MemorySegments(config->handle, 1, NULL);
			break;
	}
	return status;
}

PICO_STATUS psSetChannel(struct psconfig *config, int channelIndex)
{
	PICO_STATUS status = -1;
	
	int channel = convertChannel(config->type, config->channels[channelIndex].channel);
	int coupling = convertCoupling(config->type, config->channels[channelIndex].coupling);
	int range = convertRange(config->type, config->channels[channelIndex].range);
	
	switch(config->type) {
		case PS4000:
			status = ps4000SetChannel(config->handle, channel, config->channels[channelIndex].enabled, (int16_t) coupling, range);
			break;
		case PS3000A:
			status = ps3000aSetChannel(config->handle, channel, config->channels[channelIndex].enabled, coupling, range, 0); 
			break;	
		case PS6000:
			status = ps6000SetChannel(config->handle, channel, config->channels[channelIndex].enabled, coupling, range, 0, PS6000_BW_20MHZ);
			break;
	}
	return status;
}

PICO_STATUS psSetDataBuffer(struct psconfig *config, int channelIndex, int measuredPoints, int16_t *buffer)
{
	PICO_STATUS status = -1;
	
	int channel = convertChannel(config->type, config->channels[channelIndex].channel);
	
	switch(config->type) {
		case PS4000:
			status = ps4000SetDataBuffer(config->handle, channel, buffer, measuredPoints);
			break;
		case PS3000A:
			status = ps3000aSetDataBuffer(config->handle, channel, buffer, measuredPoints, 0, PS3000A_RATIO_MODE_NONE);
			break;	
		case PS6000:
			status = ps6000SetDataBuffer(config->handle, channel, buffer, measuredPoints, PS6000_RATIO_MODE_NONE);
			break;
	}
	return status;
}

PICO_STATUS psGetValues(struct psconfig *config, uint32_t *nPoints, int16_t *overflow)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS4000:
			status = ps4000GetValues(config->handle, 0, nPoints, 1, RATIO_MODE_NONE, 0, overflow);
			break;
		case PS3000A:
			status = ps3000aGetValues(config->handle, 0, nPoints, 1, PS3000A_RATIO_MODE_NONE, 0, overflow);
			break;	
		case PS6000:
			status = ps6000GetValues(config->handle, 0, nPoints, 1, PS6000_RATIO_MODE_NONE, 0, overflow); 
			break;
	}
	return status;
}
