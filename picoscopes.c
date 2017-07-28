#include "ps6000Api.h"
#include "ps3000aApi.h"
#include "picoscopes.h"
#include <stddef.h>

//int ps6000NChannels = 4;
//int ps6000Channels[] = {PS6000_CHANNEL_A, PS6000_CHANNEL_B, PS6000_CHANNEL_C, PS6000_CHANNEL_D};

PICO_STATUS psOpenUnit(struct psconfig *config)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS3000:
			break;
		case PS3000A:
			break;	
		case PS6000:
			status = ps6000OpenUnit(&(config->handle), config->serial);
			break;
	}
	return status;
}

PICO_STATUS psCloseUnit(struct psconfig *config)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS3000:
			break;
		case PS3000A:
			break;	
		case PS6000:
			status = ps6000CloseUnit((config->handle));
			break;
	}
	return status;
}

PICO_STATUS psGetTimebase2(struct psconfig *config, uint32_t timebase, uint32_t measuredPoints, float *timeInterval_ns)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS3000:
			break;
		case PS3000A:
			break;	
		case PS6000:
			status = ps6000GetTimebase2((config->handle), timebase, measuredPoints, timeInterval_ns, 0, NULL, 0);
			break;
	}
	return status;
}

PICO_STATUS psRunBlock(struct psconfig *config, uint32_t measuredPoints, uint32_t timebase, void *dataAvailableCallback)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS3000:
			break;
		case PS3000A:
			break;	
		case PS6000:
			status = ps6000RunBlock((config->handle), 0, measuredPoints, timebase, 1, 0, 0, dataAvailableCallback, NULL);
			break;
	}
	return status;
}

PICO_STATUS psStop(struct psconfig *config)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS3000:
			break;
		case PS3000A:
			break;	
		case PS6000:
			status = ps6000Stop((config->handle));
			break;
	}
	return status;
}


PICO_STATUS psMemorySegments(struct psconfig *config)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS3000:
			break;
		case PS3000A:
			break;	
		case PS6000:
			status = ps6000MemorySegments((config->handle), 1, NULL);
			break;
	}
	return status;
}

PICO_STATUS psSetChannel(struct psconfig *config, int channelIndex)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS3000:
			break;
		case PS3000A:
			break;	
		case PS6000:
			ps6000SetChannel((config->handle), (config->channels[channelIndex].channel), (config->channels[channelIndex].enabled), (config->channels[channelIndex].coupling), (config->channels[channelIndex].range), 0, PS6000_BW_20MHZ);
			break;
	}
	return status;
}

PICO_STATUS psSetDataBuffer(struct psconfig *config, int channelIndex, int measuredPoints, int16_t *rawDataBuffer)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS3000:
			break;
		case PS3000A:
			break;	
		case PS6000:
			ps6000SetDataBuffer((config->handle), (config->channels[channelIndex].channel), rawDataBuffer, measuredPoints, PS6000_RATIO_MODE_NONE);
			break;
	}
	return status;
}

PICO_STATUS psGetValues(struct psconfig *config, uint32_t *nPoints, int16_t *overflow)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS3000:
			break;
		case PS3000A:
			break;	
		case PS6000:
			ps6000GetValues((config->handle), 0, nPoints, 1, PS6000_RATIO_MODE_NONE, 0, overflow); 
			break;
	}
	return status;
}
