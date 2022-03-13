#ifndef __TS_H__
#define __TS_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


//#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
	#define DEBUG(fmt, args...)     printf(fmt, ##args)
#else
	#define DEBUG(fmt, args...)
#endif



#define TS_HEADER_SYNC_BYTE 	0x47


#define TS_PACKET_HEADER_SIZE 	4
#define TS_PACKET_SIZE 			188


/* PID: 13 bit */
#define TS_PID_PAT 				0x0000 	/* fixed */
#define TS_PID_PMT 				0x0FFF 	/* fixed */
#define TS_PID_H264 			0x0007 	/* custom */
#define TS_PID_AAC 				0x0008 	/* custom */


#define TS_STREAM_ID_H264 		0xE0 	/* always */
#define TS_STREAM_ID_AAC 		0xC0 	/* always */


#define PMT_STREAM_TYPE_H264 	0x1B 	/* fixed */
#define PMT_STREAM_TYPE_AAC 	0x0F 	/* fixed */
#define PMT_STREAM_TYPE_MP3 	0x03 	/* fixed */



/* ts layer header element member.
 * [Note: It is not stored as defined type size!!!]
 */
typedef struct tsHeader
{
	uint8_t  sync_byte;                    // 8b, must be 0x47
	uint8_t  transport_error_indicator;    // 1b, transport error flag, always '0'. '1': after the adapt field of the ts head is a useless byte, this byte is in the adaptive area length.
	uint8_t  payload_unit_start_indicator; // 1b, the load unit starts the identifier, and a complete packet begins with a tag of '1'.
	uint8_t  transport_priority;           // 1b, always '0'.
	uint16_t pid;                          // 13b, packet id.
	uint8_t  transport_scrambling_control; // 2b, always '00'. transfer disturbance control.
	uint8_t  adaptation_field_control;     // 2b, whether has adaption field. '00': reserved, '01': playload, '10': adaption_field, '11': playload+adaption_field.
	uint8_t  continuity_counter;           // 4b, increasing counter, 0~f, the starting value is not necessarily 0, but it must be continuous.
}T_TsHeader, *PT_TsHeader;



/* adaptationField element member.
 * [Note: It is not stored as defined type size!!!]
 */
typedef struct adaptationField
{
	uint8_t adaptation_field_length;              // 1B
	uint8_t discontinuty_indicator;               // 1b, the continuous state of the current transfer.
	uint8_t random_access_indicator;              // 1b, the next pes group with the same pid should contain the PTS fields and a raw flow access point.
	uint8_t elementary_stream_priority_indicator; // 1b, priority
	/* 5 flags for optional fields */
	uint8_t pcr_flag;                             // 1b
	uint8_t opcr_flag;                            // 1b
	uint8_t splicing_point_flag;                  // 1b
	uint8_t transport_private_data_flag;          // 1b
	uint8_t adaptation_field_extension_flag;      // 1b

	/* optional fields */
	uint64_t pcr;                         // 42b, Program Clock Reference
	uint64_t opcr;                        // 42b
	uint8_t splice_countdown;             // 1B
	uint8_t transport_private_data_len;   // 1B
	uint8_t transport_private_data[256];
}T_AdaptationField, *PT_AdaptationField;



/* pts and dts element member.
 * [Note: It is not stored as defined type size!!!]
 */
typedef struct tsPtsDts
{
	uint8_t  reserved_1;  // 4b
	uint8_t  pts_32_30;   // 3b
	uint8_t  marker_bit1; // 1b
	uint32_t pts_29_15;   // 15b
	uint8_t  marker_bit2; // 1b
	uint32_t pts_14_0;    // 15b
	uint8_t  marker_bit3; // 1b

	uint8_t  reserved_2;  // 4b
	uint8_t  dts_32_30;   // 3b
	uint8_t  marker_bit4; // 1b
	uint32_t dts_29_15;   // 15b
	uint8_t  marker_bit5; // 1b
	uint32_t dts_14_0;    // 15b
	uint8_t  marker_bit6; // 3b
}TsPtsDts;


/* PAT element member.
 * [Note: It is not stored as defined type size!!!]
 */
typedef struct pat
{
	uint8_t table_id;                 // 8b, PAT is fixed 0x00.
	uint8_t section_syntax_indicator; // 1b, fixed '1'.
	uint8_t zero;                     // 1b, fixed '0'.
	uint8_t reserved_1;               // 2b, fixed '11'.
	uint16_t section_length;          // 12b, length of data behind, comtain "crc32 code".
	uint16_t transport_stream_id;     // 16b, fixed 0x0001.
	uint8_t reserved_2;               // 2b, fixed '11'.
	uint8_t version_number;           // 5b, fixed '00000'.
	uint8_t current_next_indicator;   // 1b, fixed '1', the "pat" is available, '0' is wait for next "pat".
	uint8_t section_number;           // 8b, fixed 0x00.
	uint8_t last_section_number;      // 8b, fixed 0x00.
	/* loop start */
	uint32_t program_number;          // 16b, 0x0000: NIT, 0x0001: PMT.
	uint8_t reserved_3;               // 3b, fixed '111'.
	uint32_t pid;                     // 13b, program_number pid
	/* loop end */
	uint32_t crc_32;                  // 32b, the crc32 check code for the previous data
}T_Pat, *PT_Pat;



/* PMT element member.
 * [Note: It is not stored as defined type size!!!]
 */
typedef struct pmt
{
	uint8_t  table_id;                 // 8b, PMT is fixed 0x02.
	uint8_t  section_syntax_indicator; // 1b, fixed '1'.
	uint8_t  zero;                     // 1b, fixed '0'.
	uint8_t  reserved_1;               // 2b, fixed '11'.
	uint32_t section_length;           // 12b, length of data behind, comtain "crc32 code".
	uint32_t program_number;           // 16b, pid of applicable program.
	uint8_t  reserved_2;               // 2b, fixed '11'.
	uint8_t  version_number;           // 5b, fixed '00000', if pat variable will be '00001'.
	uint8_t  current_next_indicator;   // 1b, fixed '1'.
	uint8_t  section_number;           // 8b, fixed 0x00.
	uint8_t  last_section_number;      // 8b, fixed 0x00.
	uint8_t  reserved_3;               // 3b, fixed '111'.
	uint32_t pcr_pid;                  // 13b, The pcr is grouped by the ts, which is designated as the video pid.
	uint8_t  reserved_4;               // 4b, fixed '1111'.
	uint32_t program_info_length;      // 12b, program describes, 0x000 for no.
	/* loop start */
	/* program 1 */
	uint8_t  stream_type_video;        // 8b, stream type. 0x1b: h264  0x0f: aac  0x03: mp3
	uint8_t  reserved_5_video;         // 3b, fixed '111'
	uint32_t elementary_pid_video;     // 13b, pid of "stream type"
	uint8_t  reserved_6_video;         // 4b, fixed '1111'
	uint32_t es_info_length_video;     // 12b, program describes, 0x000 for no.

	/* program 2 */
	uint8_t  stream_type_audio;
	uint8_t  reserved_5_audio;
	uint32_t elementary_pid_audio;
	uint8_t  reserved_6_audio;
	uint32_t es_info_length_audio;
	/* loop end */

	uint32_t crc_32;                   // 32b, the crc32 check code for the previous data
}T_Pmt, *PT_Pmt;




/* custom "PES" structure, it includes "pes header" and "es data".
 * so the member's size is different in ts file.
 */
typedef struct
{
	/* "pes header"(not contain pts/dts) : 9 Bytes */
	uint32_t packet_start_code_prefix;  // 3B, start code, must be 0x000001.
	uint8_t  stream_id;                 // 1B, audio(0xc0~0xdf), always 0xc0; video(0xe0~0xef), always 0xe0.
	uint32_t pes_packet_length;         // 2B, the length of the data behind, 0 indicates unlimited length.
										// 1B: always 0x80 in this Byte.
	uint8_t  marker_bit;                //   2b, must be '10'.
	uint8_t  pes_scrambling_control;    //   2b, pes packet scrambling control.
	uint8_t  pes_priority;              //   1b, pes packet priority.
	uint8_t  data_alignment_indicator;  //   1b, '1': the follows video or audio syncword is shown in the back of the pes package.
	uint8_t  copyright;                 //   1b, copyright protection.
	uint8_t  original_or_copy;          //   1b, playload source.
										// 1B: 0x80: only pts; 0xc0: pts+dts
	uint8_t  pts_dts_flags;             //   2b, '10': PTS; '11': PTS+DTS, '00': none, '01': reserved.
	uint8_t  escr_flag;                 //   1b, '1': escr_base+escr_expand; '0': none.
	uint8_t  es_rate_flag;              //   1b, es rate.
	uint8_t  dsm_trick_mode_flag;       //   1b, whether the 8 bitt connection field exists.
	uint8_t  additional_copy_info_flag; //   1b, additional copy information.
	uint8_t  pes_crc_flag;              //   1b, whether there is a call.
	uint8_t  pes_extension_flag;        //   1b, extension.
	uint8_t  pes_data_length;           // 1B, 5 or 10, the length of the data(pts, dts) behind.

	TsPtsDts ts_pts_dts;                // pts+dts structure.

	/* "es data": 1024*1024+4 Bytes max */
	//uint8_t  av_data[MAX_ONE_FRAME_SIZE]; // one frame audio or video data.
	uint8_t* av_data; 					// one frame audio or video data.
	uint8_t* av_data_cur_ptr; 			// position for current "av_data"
	uint32_t av_data_len;				// length of data.
}T_PesEsStruct, *PT_PesEsStruct;




/************************************************************************
 * function describe: Mux h264 and aac to TS file.
 * params:
 *   [h264FileName]: h264 file.(in)
 *   [vFps]: video fps.(in)
 *   [aacFileName]: aac file file.(in)
 *   [tsFileName]: ts file.(in)
 * return: 0-success  other-error
 ************************************************************************/
int ts_mux_h264_aac(char *h264FileName, uint32_t vFps, char *aacFileName, char *tsFileName);


#endif

