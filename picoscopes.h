
#ifndef __picoscopes_H__
#define __picoscopes_H__

#include "ps6000API.h"
#include "ps3000aAPI.h"

enum scopeType {PS3000, PS3000A, PS6000, PSNONE};

enum psChannel {PS_CHANNEL_A, PS_CHANNEL_B, PS_CHANNEL_C, PS_CHANNEL_D};
enum psCoupling {PS_AC, PS_DC};
enum psRange {PS_50MV, PS_100MV, PS_200MV, PS_500MV, PS_1V, PS_2V, PS_5V, PS_10V, PS_20V};

struct scopeDef {
	int8_t *serial;
	enum scopeType type;
};

struct pschannel {
	enum psChannel channel; 		// example: PS_CHANNEL_A
	enum rangeSetting range;		// example: PS_DC
	double rangeVal;				
	enum psCoupling coupling;		// example: PS_10V
	double coefficient;
	int16_t enabled;
};

struct psconfig {
	int16_t handle;
	enum scopeType type;
	int8_t *serial;
	uint32_t nPoints;
	int timebase;
	struct pschannel channels[4];
};

static struct scopeDef picoscopes[] = {{.serial = (int8_t*) "AP231/007", .type = PS6000},
									   {.serial = (int8_t*) "ES286/061", .type = PS3000A},
									   {.serial = (int8_t*) "IJY32/014", .type = PS3000}};
// Function prototypes
PICO_STATUS psOpenUnit(struct psconfig *config);
PICO_STATUS psCloseUnit(struct psconfig *config);
void psUpdateTimebase(struct psconfig *config, float sampleRate);
PICO_STATUS psGetTimebase2(struct psconfig *config, float *timeInterval_ns);
PICO_STATUS psRunBlock(struct psconfig *config, void *dataAvailableCallback);
PICO_STATUS psStop(struct psconfig *config);
PICO_STATUS psMemorySegments(struct psconfig *config);
PICO_STATUS psSetChannel(struct psconfig *config, int channelIndex);
PICO_STATUS psSetDataBuffer(struct psconfig *config, int channelIndex, int measuredPoints, int16_t *rawDataBuffer);
PICO_STATUS psGetValues(struct psconfig *config, uint32_t *nPoints, int16_t *overflow);

#endif  /* ndef __picoscopes_H__ */
