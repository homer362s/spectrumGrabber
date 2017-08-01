#include <utility.h>
#include "ps6000Api.h"
#include "ps3000aApi.h"
#include "picoscopes.h"
#include <stddef.h>

// Convert the general channel into an API specific channel
int convertChannel(enum scopeType type, enum psChannel channel) 
{
	switch (type) {
		case PS3000:
			break;
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
		case PS3000:
			break;
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
		case PS3000:
			break;
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
		case PS3000:
			break;
		case PS3000A:
			ps3000aMaximumValue(config->handle, &maxValue);
			break;
		case PS6000:
			maxValue = PS6000_MAX_VALUE;
			break;
	}
	
	for (int i = 0;i < config->nPoints;i++) {
		scaledData[i] = (double) rawData[i] / maxValue * config->channels[channel].rangeVal / config->channels[channel].coefficient;
	}
}

PICO_STATUS psOpenUnit(struct psconfig *config)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS3000:
			break;
		case PS3000A:
			ps3000aCloseUnit(config->handle);
			status = ps3000aOpenUnit(&(config->handle), config->serial);
			break;	
		case PS6000:
			ps6000CloseUnit(config->handle);
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
			status = ps3000aCloseUnit(config->handle);
			break;	
		case PS6000:
			status = ps6000CloseUnit(config->handle);
			break;
	}
	return status;
}

// Update config.timebase according the the sample rate and type of picoscope
void psUpdateTimebase(struct psconfig *config, float sampleRate)
{
	switch (config->type) {
		case PS3000:
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
		case PS3000:
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
		case PS3000:
			break;
		case PS3000A:
			status = ps3000aRunBlock(config->handle, 0, config->nPoints, config->timebase, 1, 0, 0, dataAvailableCallback, NULL); // status = 282   
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
		case PS3000:
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


PICO_STATUS psMemorySegments(struct psconfig *config)
{
	PICO_STATUS status = -1;
	
	switch(config->type) {
		case PS3000:
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
		case PS3000:
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
		case PS3000:
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
		case PS3000:
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
