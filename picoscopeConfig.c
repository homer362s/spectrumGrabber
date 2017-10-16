#include <ansi_c.h>
#include "picoscopes.h"
#include "dacConfig.h"

char *boolToStr(int val)
{
	if(val)
		return "true";
	return "false";
}

void savePicoscopeConfig(struct psconfig *config, struct dacconfig *dacConfig, char *filename)
{
	// Make a new config file
	FILE *fp = fopen(filename, "w");
	
	// Write header
	fprintf(fp, "%s: %s\n", getScopeTypeStr(config), config->serial);
	
	// Write global values
	fprintf(fp, "timebase: %d\n", config->timebase);
	fprintf(fp, "samples: %d\n", config->nPoints);
	fprintf(fp, "downsample: %d\n", config->downsampleRatio);
	
	// Write DAC settings
	fprintf(fp, "begin dac\n");
	fprintf(fp, "\tboard: %d\n", dacConfig->board);
	fprintf(fp, "\tvg: %d\n", dacConfig->vg);
	fprintf(fp, "\tvd: %d\n", dacConfig->vd);
	fprintf(fp, "end\n");
	
	// Loop over channel settings
	for (int i = 0;i < config->nChannels;i++) {
		fprintf(fp, "begin channel %d %s\n", i+1, config->channels[i].name);
		fprintf(fp, "\trange: %d\n", config->channels[i].range);
		fprintf(fp, "\tcoupling: %d\n", config->channels[i].coupling);
		fprintf(fp, "\tcoefficient: %e\n", config->channels[i].coefficient);
		fprintf(fp, "\tfreq: %s\n", boolToStr(config->channels[i].enabled));
		fprintf(fp, "\ttime: %s\n", boolToStr(config->channels[i].enabled));
		fprintf(fp, "end\n");
	}
	
	
	// Close your files when done
	fclose(fp);
}
