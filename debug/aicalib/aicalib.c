#include <stdio.h>
#include "rkxml.h"

int main(int argc, char *argv[])
{
	int i;
	struct aical cal[AI_NUM];

	for (i = 0; i < AI_NUM; i++) {
		cal[i].loffset = 0x03;
		cal[i].hoffset = 0x5870;
	}

	rkXmlGenAiCalibDoc(cal, AI_NUM);

	return 0;
}
