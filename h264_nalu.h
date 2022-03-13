#ifndef __H264_H__
#define __H264_H__

#include <stdio.h>
#include <stdint.h>


#define TIME_SCALE 		(90000)

#define MAX_NALU_SIZE 	(1*1024*1024) /* 1M Bytes */


typedef enum{
	NALU_TYPE_SLICE    = 0x1,
	NALU_TYPE_DPA      = 0x2,
	NALU_TYPE_DPB      = 0x3,
	NALU_TYPE_DPC      = 0x4,
	NALU_TYPE_IDR      = 0x5,
	NALU_TYPE_SEI      = 0x6,
	NALU_TYPE_SPS      = 0x7,
	NALU_TYPE_PPS      = 0x8,
	NALU_TYPE_AUD      = 0x9,
	NALU_TYPE_EOSEQ    = 0xa,
	NALU_TYPE_EOSTREAM = 0xb,
	NALU_TYPE_FILL     = 0xc,
}h264_nalu_type_e;



typedef struct{
	uint32_t data_len; 	/* the length of this nalu.(include start code) */
	uint32_t startcode_len;
	h264_nalu_type_e nalu_type;
}T_NaluInfo, *PT_NaluInfo;



/************************************************************************
 * function describe: get one nalu(include start code) from h264 file.
 * params:
 *   [fp]: H.264 file handler.(in)
 *   [pNaluData]: the function will fill the nalu data in it, must be
 *                alloced memory before call this function.(out)
 *   [ptNaluInfo]: the information in this nalu.(out)
 * return: 0:success other:error
 ************************************************************************/
int getOneH264Nalu(FILE *fp, uint8_t *pNaluData, T_NaluInfo *ptNaluInfo);


#endif /* __H264_H__ */
