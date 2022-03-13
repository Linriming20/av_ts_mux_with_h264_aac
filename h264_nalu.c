#include "h264_nalu.h"

int getOneH264Nalu(FILE *fp, uint8_t *pNaluData, T_NaluInfo *ptNaluInfo)
{
	uint32_t readBytes = 0;
	uint32_t pos = 0;

	if(!fp || !pNaluData || !ptNaluInfo)
		return -1;

	if((readBytes = fread(pNaluData, 1, 4, fp)) <= 0)
		return -2;

	// judge the type of NALU start code
	if(pNaluData[0] == 0 && pNaluData[1] == 0 && pNaluData[2] == 0 && pNaluData[3] == 1)
	{
		pos = 4; // start by pNaluData[4]
		ptNaluInfo->startcode_len = 4;
	}
	else if(pNaluData[0] == 0 && pNaluData[1] == 0 && pNaluData[2] == 1)
	{
		pos = 3;
		ptNaluInfo->startcode_len = 3;
		fseek(fp, -1, SEEK_CUR); // if start code type is 3, need to adapt point
	}
	else
		return -3;

	// find next NALU
	while(1)
	{
		int val = 0;
		if((val = fgetc(fp)) != EOF)
		{
			pNaluData[pos] = (unsigned char)val;
		}
		else
		{
			// file end，pos should not add 1 in last loop
			pos -= 1;
			break;
		}

		/* judge the start code type of "00 00 00 01" or "00 00 01",
		 * and must judge the "00 00 00 01", as it include the position of "00 00 01"
		 */
		if(pNaluData[pos-3] == 0 && pNaluData[pos-2] == 0 && pNaluData[pos-1] == 0 && pNaluData[pos] == 1)
		{
			fseek(fp, -4, SEEK_CUR);
			pos -= 4;
			break;
		}
		else if(pNaluData[pos-2] == 0 && pNaluData[pos-1] == 0 && pNaluData[pos] == 1)
		{
			fseek(fp, -3, SEEK_CUR);
			pos -= 3;
			break;
		}
		pos++;
	}

	// NALU length = array index + 1
	ptNaluInfo->data_len = pos+1;
	ptNaluInfo->nalu_type = pNaluData[ptNaluInfo->startcode_len] & 0x1F;

	return 0;
}

