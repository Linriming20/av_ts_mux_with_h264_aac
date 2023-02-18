#include <stdio.h>


#include "ts.h"



int main(int argc, char *argv[])
{
	if(argc == 1)
	{
		printf("Usage: \n"
			   "   %s avfile/test1_856x480_24fps.h264 24 avfile/test1_44100_stereo.aac out1.ts\n"
			   "   %s avfile/test2_720x480_30fps.h264 60 avfile/test2_48000_stereo.aac out2.ts\n"
			   "   %s avfile/test3_1280x720_20fps.h264 20 avfile/test1_44100_stereo.aac out3.ts\n",
			   argv[0], argv[0], argv[0]);
		return -1;
	}

	if(0 == ts_mux_h264_aac(argv[1], atoi(argv[2]), argv[3], argv[4]))
		printf("\033[32mMux ts file success!\033[0m \n");
	else
		printf("\033[31mMux ts file failed!\033[0m \n");

	return 0;
}
