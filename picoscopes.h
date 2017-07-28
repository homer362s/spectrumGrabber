
#ifndef __picoscopes_H__
#define __picoscopes_H__

enum scopeType {PS3000, PS3000A, PS6000};

struct scopeDef {
	int8_t *serial;
	enum scopeType type;
};

struct pschannel {
	int channel; // PS6000_CHANNEL_A
	int range;
	double rangeVal;
	int coupling;
	double coefficient;
	short enabled;
};

struct psconfig {
	int16_t handle;
	enum scopeType type;
	int8_t *serial;
	int nPoints;
	float sampleRate;
	struct pschannel channels[4];
};

static struct scopeDef picoscopes[] = {{.serial = (int8_t*) "AP231/007", .type = PS6000},
									   {.serial = (int8_t*) "ES286/061", .type = PS3000A},
									   {.serial = (int8_t*) "IJY32/014", .type = PS3000}};
// Function prototypes
PICO_STATUS psOpenUnit(struct psconfig *config);
PICO_STATUS psCloseUnit(struct psconfig *config);
PICO_STATUS psGetTimebase2(struct psconfig *config, uint32_t timebase, uint32_t measuredPoints, float *timeInterval_ns);
PICO_STATUS psRunBlock(struct psconfig *config, uint32_t measuredPoints, uint32_t timebase, void *dataAvailableCallback);
PICO_STATUS psStop(struct psconfig *config);
PICO_STATUS psMemorySegments(struct psconfig *config);
PICO_STATUS psSetChannel(struct psconfig *config, int channelIndex);
PICO_STATUS psSetDataBuffer(struct psconfig *config, int channelIndex, int measuredPoints, int16_t *rawDataBuffer);
PICO_STATUS psGetValues(struct psconfig *config, uint32_t *nPoints, int16_t *overflow);

#endif  /* ndef __picoscopes_H__ */
