#include "aac_adts.h"


int getAdtsFrame(FILE *fp, uint8_t *pAdtsFrameData, T_AdtsHeader *ptAdtsHeaderInfo)
{
	uint32_t readBytes = 0;

	if(!fp || !pAdtsFrameData || !ptAdtsHeaderInfo)
		return -1;

	// ADTS header size is AAC_ADTS_HEADER_SIZE(=7) bytes
	readBytes = fread(pAdtsFrameData, 1, AAC_ADTS_HEADER_SIZE, fp);
	if(readBytes <= 0)
		return -2;


	ptAdtsHeaderInfo->syncword              = (pAdtsFrameData[0] << 4 ) | (pAdtsFrameData[1]  >> 4);
	ptAdtsHeaderInfo->id                    = (pAdtsFrameData[1] & 0x08) >> 3;
	ptAdtsHeaderInfo->layer                 = (pAdtsFrameData[1] & 0x06) >> 1;
	ptAdtsHeaderInfo->protection_absent     =  pAdtsFrameData[1] & 0x01;
	ptAdtsHeaderInfo->profile               = (pAdtsFrameData[2] & 0xc0) >> 6;
	ptAdtsHeaderInfo->sampling_freq_index   = (pAdtsFrameData[2] & 0x3c) >> 2;
	ptAdtsHeaderInfo->private_bit           = (pAdtsFrameData[2] & 0x02) >> 1;
	ptAdtsHeaderInfo->channel_configuration = (((pAdtsFrameData[2] & 0x01) << 2) | ((pAdtsFrameData[3] & 0xc0) >> 6));
	ptAdtsHeaderInfo->original_copy         = (pAdtsFrameData[3] & 0x20) >> 5;
	ptAdtsHeaderInfo->home                  = (pAdtsFrameData[3] & 0x10) >> 4;
	ptAdtsHeaderInfo->copyright_identification_bit   = (pAdtsFrameData[3] & 0x08) >> 3;
	ptAdtsHeaderInfo->copyright_identification_start = (pAdtsFrameData[3] & 0x04) >> 2;
	ptAdtsHeaderInfo->aac_frame_length = ((pAdtsFrameData[3] & 0x03) << 11) |
										 ((pAdtsFrameData[4] & 0xFF) << 3) |
										 ((pAdtsFrameData[5] & 0xE0) >> 5);
	ptAdtsHeaderInfo->adts_buffer_fullness = ((pAdtsFrameData[5] & 0x1f) << 6 | (pAdtsFrameData[6] & 0xfc) >> 2);
	ptAdtsHeaderInfo->number_of_raw_data_blocks_in_frame = (pAdtsFrameData[6] & 0x03);

	if (ptAdtsHeaderInfo->syncword != 0xFFF)
		return -3;

	/* read the remaining frame of ADTS data outside the AAC_ADTS_HEADER_SIZE(=7) bytes header,
	 * and it should be written after offsetting the header by AAC_ADTS_HEADER_SIZE(=7) bytes
	 */
	readBytes = fread(pAdtsFrameData + AAC_ADTS_HEADER_SIZE, 1, ptAdtsHeaderInfo->aac_frame_length - AAC_ADTS_HEADER_SIZE, fp);
	if(readBytes <= 0)
		return -4;

	return 0;
}

