
#ifndef __picoscopes_H__
#define __picoscopes_H__

#include "ps6000API.h"
#include "ps3000aAPI.h"

#define MAX_RANGES 13
#define MAX_COUPLINGS 2

enum scopeType {PS3000A, PS4000, PS6000, PSNONE};

enum psChannel {PS_CHANNEL_A, PS_CHANNEL_B, PS_CHANNEL_C, PS_CHANNEL_D};
enum psCoupling {PS_AC, PS_DC};
enum psRange {PS_10MV, PS_20MV, PS_50MV, PS_100MV, PS_200MV, PS_500MV, PS_1V, PS_2V, PS_5V, PS_10V, PS_20V, PS_50V, PS_100V};

struct scopeDef {
	const int8_t *serial;
	const enum scopeType type;
	const int nChannels;
	const int nRanges;
	const enum psRange ranges[MAX_RANGES];
	const int nCouplings;
	const enum psCouplings couplings[MAX_COUPLINGS];
	int downsampleSupport;
};

struct pschannel {
	enum psChannel channel; 		// example: PS_CHANNEL_A
	enum rangeSetting range;		// example: PS_DC
	enum psCoupling coupling;		// example: PS_10V
	double coefficient;
	int16_t enabled;
	char name[32];					// User specified name of the channel
};

struct psconfig {
	int16_t handle;					// Handle that identifies connected picoscope
	int nChannels;					// Number of channels installed on this picoscope
	int nRanges;					// Number of range settings on this picoscope
	enum psRange *ranges;			// List of valid ranges for this picoscope
	int nCouplings;					// Number of coupling settings on this picoscope
	enum psCouplings *couplings;	// List of valid coupling settings on this picoscope
	enum scopeType type;			// API version of the picoscope
	int8_t *serial;					// Serial string
	uint32_t nPoints;				// Number of points set to be measured
	int timebase;					// Timebase set to be used
	int32_t downsampleRatio;		// How many points sould be averaged into one
	int downsampleSupport;			// TRUE if downsampling is supported, FALSE otherwise
	struct pschannel channels[4];   // Settings for individual channels
};

static const struct scopeDef picoscopes[] = {{.serial = (int8_t*) "AP231/007", .type = PS6000, .nChannels=4, .nRanges = 9, .ranges={PS_50MV, PS_100MV, PS_200MV, PS_500MV, PS_1V, PS_2V, PS_5V, PS_10V, PS_20V}, .nCouplings = 2, .couplings = {PS_DC, PS_AC}, .downsampleSupport = 1},
									         {.serial = (int8_t*) "ES286/061", .type = PS3000A, .nChannels=4, .nRanges = 9, .ranges={PS_50MV, PS_100MV, PS_200MV, PS_500MV, PS_1V, PS_2V, PS_5V, PS_10V, PS_20V}, .nCouplings = 2, .couplings = {PS_DC, PS_AC}, .downsampleSupport = 1},
									         {.serial = (int8_t*) "AU354/016", .type = PS4000, .nChannels=2, .nRanges = 9, .ranges={PS_50MV, PS_100MV, PS_200MV, PS_500MV, PS_1V, PS_2V, PS_5V, PS_10V, PS_20V}, .nCouplings = 2, .couplings = {PS_DC, PS_AC}, .downsampleSupport = 0}};
// Function prototypes
void getRangeLabel(enum psRange range, char *label);
void getCouplingLabel(enum psCoupling coupling, char *label);
double getRangeValue(enum psRange range);
void scaleReading(struct psconfig *config, int channel, int16_t *rawData, double *scaledData);
PICO_STATUS psOpenUnit(struct psconfig *config);
PICO_STATUS psCloseUnit(struct psconfig *config);
void psUpdateTimebase(struct psconfig *config, float sampleRate);
PICO_STATUS psGetTimebase2(struct psconfig *config, float *timeInterval_ns);
PICO_STATUS psRunBlock(struct psconfig *config, void *dataAvailableCallback);
PICO_STATUS psStop(struct psconfig *config);
PICO_STATUS psMemorySegments(struct psconfig *config);
PICO_STATUS psSetChannel(struct psconfig *config, int channelIndex);
PICO_STATUS psSetDataBuffer(struct psconfig *config, int channelIndex, int16_t *rawDataBuffer);
PICO_STATUS psGetValues(struct psconfig *config, uint32_t *nPoints, int16_t *overflow);

#endif  /* ndef __picoscopes_H__ */
