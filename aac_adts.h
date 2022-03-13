#ifndef __AAC_ADTS_H__
#define __AAC_ADTS_H__

#include <stdio.h>
#include <stdint.h>


#define AAC_ADTS_HEADER_SIZE 	(7)

#define MAX_ADTS_SIZE 			(1024) /* 1K Bytes */


typedef enum{
    MPEG_4 = 0x0,
    MPEG_2 = 0x1,
}aac_id_t;


typedef enum{
    SFI_96000 = 0x0,
    SFI_88200 = 0x1,
    SFI_64000 = 0x2,
    SFI_48000 = 0x3,
    SFI_44100 = 0x4,
    SFI_32000 = 0x5,
    SFI_24000 = 0x6,
    SFI_22050 = 0x7,
    SFI_16000 = 0x8,
    SFI_12000 = 0x9,
    SFI_11025 = 0xa,
    SFI_8000  = 0xb,
    SFI_7350  = 0xc,
    SFI_ERROR = 0xd,
}sampling_freq_index_t;


/* AAC(ADTS) Header element member.
 * [Note: It is not stored as defined type size!!!]
 */
typedef struct{
	/* fixed header */
    uint32_t syncword;              // 12bit  '1111 1111 1111' is stand by ADTS frame
    uint32_t id;                    // 1 bit  0 for MPEG-4, 1 for MPEG-2
    uint32_t layer;                 // 2 bit  always '00'
    uint32_t protection_absent;     // 1 bit  1 not crc, 0 have crc 1
    uint32_t profile;               // 2 bit  AAC profile, '01' for AAC-LC
    uint32_t sampling_freq_index;   // 4 bit  reference to 'sampling_freq_index_t'
    uint32_t private_bit;           // 1 bit  always '0'
    uint32_t channel_configuration; // 3 bit  channels count
    uint32_t original_copy;         // 1 bit  always '0'
    uint32_t home;                  // 1 bit

    /* varible header */
    uint32_t copyright_identification_bit;   // 1 bit  always '0'
    uint32_t copyright_identification_start; // 1 bit  always '0'
    uint32_t aac_frame_length;               // 13bit  length of [adts header] + [adts data]
    uint32_t adts_buffer_fullness;           // 11bit  0x7FF stand by varible bit rate
    uint32_t number_of_raw_data_blocks_in_frame;  // 2 bit  always '00', number of AAC Frames(RDBs) in ADTS frame minus 1
}T_AdtsHeader, *PT_AdtsHeader;



/************************************************************************
 * function describe: get one frame aac(adts, include adts header) from
 *                    aac file.
 * params:
 *   [fp]: aac file handler.(in)
 *   [pAdtsFrameData]: the function will fill the aac data in it, must be
 *                     alloced memory before call this function.(out)
 *   [ptAdtsHeaderInfo]: AAC-ADTS header information in this frame.(out)
 * return: 0-success  other-error
 ************************************************************************/
int getAdtsFrame(FILE *fp, uint8_t *pAdtsFrameData, T_AdtsHeader *ptAdtsHeaderInfo);


#endif /*  __AAC_ADTS_H__ */
