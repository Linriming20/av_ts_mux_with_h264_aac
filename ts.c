
#include "h264_nalu.h"
#include "aac_adts.h"
#include "ts.h"
#include "crcLib.h"



static uint32_t endian_convert(uint32_t val)
{
	return (((val >>  0)  & 0xff) << 24) |\
		   (((val >>  8)  & 0xff) << 16) |\
		   (((val >> 16) & 0xff) << 8)  |\
		   (((val >> 24) & 0xff) << 0);
}


static int fixFirstPacketAdaptionFieldStruct(T_AdaptationField *ptAdaptationField, uint64_t pts)
{
	if(!ptAdaptationField)
	{
		printf("[%s:%d] Params invalid!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	ptAdaptationField->adaptation_field_length = 7; /* pcr_flag = 1: has pcr */
	ptAdaptationField->discontinuty_indicator = 0;
	ptAdaptationField->random_access_indicator = 0; /* is keyflame? always 0, you can set it such "iskeyflame?1:0" */
	ptAdaptationField->elementary_stream_priority_indicator = 0;
	/* 5 flags for optional fields */
	ptAdaptationField->pcr_flag = 1;
	ptAdaptationField->opcr_flag = 0;
	ptAdaptationField->splicing_point_flag = 0;
	ptAdaptationField->transport_private_data_flag = 0;
	ptAdaptationField->adaptation_field_extension_flag = 0;

	ptAdaptationField->pcr  = pts * 300;
	ptAdaptationField->opcr = 0;
	ptAdaptationField->splice_countdown = 0;
	ptAdaptationField->transport_private_data_len = 0;

	return 0;
}


static int fixCommonAdaptionFieldStruct(T_AdaptationField *ptAdaptationField)
{
	if(!ptAdaptationField)
	{
		printf("[%s:%d] Params invalid!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	ptAdaptationField->adaptation_field_length = 1; /* 5 flags = 0 */
	ptAdaptationField->discontinuty_indicator = 0;
	ptAdaptationField->random_access_indicator = 0; /* is keyflame? always 0, you can set it such "iskeyflame?1:0" */
	ptAdaptationField->elementary_stream_priority_indicator = 0;
	/* 5 flags for optional fields */
	ptAdaptationField->pcr_flag = 0;
	ptAdaptationField->opcr_flag = 0;
	ptAdaptationField->splicing_point_flag = 0;
	ptAdaptationField->transport_private_data_flag = 0;
	ptAdaptationField->adaptation_field_extension_flag = 0;

	ptAdaptationField->pcr  = 0;
	ptAdaptationField->opcr = 0;
	ptAdaptationField->splice_countdown = 0;
	ptAdaptationField->transport_private_data_len = 0;

	return 0;
}



static int fixAdtsFrameToPesStruct(uint8_t *adtsData, uint32_t adtsDataLen, uint64_t pts, T_PesEsStruct *ptAdtsPesStu)
{
	if(!ptAdtsPesStu || !adtsData || !adtsDataLen)
	{
		printf("[%s:%d] Params invalid!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	/* pes header */
	ptAdtsPesStu->packet_start_code_prefix = 0x000001;
	ptAdtsPesStu->stream_id = TS_STREAM_ID_AUDIO;
	ptAdtsPesStu->pes_packet_length = 0;
	ptAdtsPesStu->marker_bit = 0b10;
	ptAdtsPesStu->pes_scrambling_control = 0b00;
	ptAdtsPesStu->pes_priority = 0b0;
	ptAdtsPesStu->data_alignment_indicator = 0b0;
	ptAdtsPesStu->copyright = 0b0;
	ptAdtsPesStu->original_or_copy = 0b0;
	ptAdtsPesStu->pts_dts_flags = 0b10;  // '10': audio(aac) only PTS
	ptAdtsPesStu->escr_flag = 0b0;
	ptAdtsPesStu->es_rate_flag = 0b0;
	ptAdtsPesStu->dsm_trick_mode_flag = 0b0;
	ptAdtsPesStu->additional_copy_info_flag = 0b0;
	ptAdtsPesStu->pes_crc_flag = 0b0;
	ptAdtsPesStu->pes_extension_flag = 0b0;
	ptAdtsPesStu->pes_data_length = 5;   // pts_dts_flags = '10': audio(aac) only PTS

	ptAdtsPesStu->ts_pts_dts.reserved_1 = 0b1111;
	if(pts > 0x7FFFFFFF)
	{
		ptAdtsPesStu->ts_pts_dts.pts_32_30 = (pts >> 30) & 0x07;
		ptAdtsPesStu->ts_pts_dts.marker_bit1 = 0b1;
	}
	else
	{
		ptAdtsPesStu->ts_pts_dts.pts_32_30  = 0;
		ptAdtsPesStu->ts_pts_dts.marker_bit1 = 0b0;
	}

	if(pts > 0x7FFF)
	{
		ptAdtsPesStu->ts_pts_dts.pts_29_15 = (pts >> 15) & 0x007FFF;
		ptAdtsPesStu->ts_pts_dts.marker_bit2 = 0b1;
	}
	else
	{
		ptAdtsPesStu->ts_pts_dts.pts_29_15 = 0;
		ptAdtsPesStu->ts_pts_dts.marker_bit2 = 0b0;
	}

	ptAdtsPesStu->ts_pts_dts.pts_14_0 = pts & 0x007FFF;
	ptAdtsPesStu->ts_pts_dts.marker_bit3 = 0b1;

	/* es data */
	ptAdtsPesStu->av_data = adtsData;
	ptAdtsPesStu->av_data_len = adtsDataLen;

	return 0;
}



int fixH264FrameToPesStruct(uint8_t *h264Data, uint32_t h264DataLen, uint64_t pts, T_PesEsStruct* ptH264PesStu)
{
	if(!ptH264PesStu || !h264Data || !h264DataLen)
	{
		printf("[%s:%d] Params invalid!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	/* pes header */
	ptH264PesStu->packet_start_code_prefix = 0x000001;
	ptH264PesStu->stream_id = TS_STREAM_ID_VIDEO;
	ptH264PesStu->pes_packet_length = 0;
	ptH264PesStu->marker_bit = 0b10;
	ptH264PesStu->pes_scrambling_control = 0b00;
	ptH264PesStu->pes_priority = 0b0;
	ptH264PesStu->data_alignment_indicator = 0b0;
	ptH264PesStu->copyright = 0b0;
	ptH264PesStu->original_or_copy = 0b0;
	ptH264PesStu->pts_dts_flags = 0b10;   // '10':  only PTS
	ptH264PesStu->escr_flag = 0b0;
	ptH264PesStu->es_rate_flag = 0b0;
	ptH264PesStu->dsm_trick_mode_flag = 0b0;
	ptH264PesStu->additional_copy_info_flag = 0b0;
	ptH264PesStu->pes_crc_flag = 0b0;
	ptH264PesStu->pes_extension_flag = 0b0;
	ptH264PesStu->pes_data_length = 5;   // pts_dts_flags = '10'

	ptH264PesStu->ts_pts_dts.reserved_1 = 0x03;
	if(pts > 0x7FFFFFFF)
	{
		ptH264PesStu->ts_pts_dts.pts_32_30 = (pts >> 30) & 0x07;
		ptH264PesStu->ts_pts_dts.marker_bit1 = 0b1;
	}
	else
	{
		ptH264PesStu->ts_pts_dts.pts_32_30  = 0;
		ptH264PesStu->ts_pts_dts.marker_bit1 = 0b0;
	}

	if(pts > 0x7FFF)
	{
		ptH264PesStu->ts_pts_dts.pts_29_15 = (pts >> 15) & 0x007FFF ;
		ptH264PesStu->ts_pts_dts.marker_bit2 = 0b1;
	}
	else
	{
		ptH264PesStu->ts_pts_dts.pts_29_15 = 0;
		ptH264PesStu->ts_pts_dts.marker_bit2 = 0b0;
	}

	ptH264PesStu->ts_pts_dts.pts_14_0 = pts & 0x007FFF;
	ptH264PesStu->ts_pts_dts.marker_bit3 = 0b1;

	/* es data */
	ptH264PesStu->av_data = h264Data;
	ptH264PesStu->av_data_len = h264DataLen;

	return 0;
}



static int fixFirstPacketAdaptionField(T_AdaptationField *adaptationField, uint8_t adaptFieldLen, uint8_t *adaptBuf)
{
	uint8_t adaptiveFlags = 0x00;
	uint8_t adaptivePos = 1; // offset

	if(!adaptationField || !adaptBuf)
	{
		printf("[%s:%d] Params invalid!\n", __FUNCTION__, __LINE__);
		return -1;
	}


	if(adaptationField->discontinuty_indicator)
	{
		adaptiveFlags |= 0x80;
	}
	if(adaptationField->random_access_indicator)
	{
		adaptiveFlags |= 0x40;
	}
	if(adaptationField->elementary_stream_priority_indicator)
	{
		adaptiveFlags |= 0x20;
	}
	if(adaptationField->pcr_flag)
	{
		adaptiveFlags |= 0x10;

		uint64_t pcr_base = adaptationField->pcr / 300;
		uint64_t pcr_ext = adaptationField->pcr % 300;
		adaptBuf[adaptivePos + 0] = (pcr_base >> 25) & 0xff;
		adaptBuf[adaptivePos + 1] = (pcr_base >> 17) & 0xff;
		adaptBuf[adaptivePos + 2] = (pcr_base >> 9) & 0xff;
		adaptBuf[adaptivePos + 3] = (pcr_base >> 1) & 0xff;
		adaptBuf[adaptivePos + 4] = pcr_base << 7 | pcr_ext >> 8 | 0x7e;
		adaptBuf[adaptivePos + 5] = (pcr_ext) & 0xff;
		adaptivePos += 6;
	}
	if(adaptationField->opcr_flag)
	{
		adaptiveFlags |= 0x08;

		uint64_t opcr_base = adaptationField->opcr / 300;
		uint64_t opcr_ext = adaptationField->opcr % 300;
		adaptBuf[adaptivePos + 0] = (opcr_base >> 25) & 0xff;
		adaptBuf[adaptivePos + 1] = (opcr_base >> 17) & 0xff;
		adaptBuf[adaptivePos + 2] = (opcr_base >> 9) & 0xff;
		adaptBuf[adaptivePos + 3] = (opcr_base >> 1) & 0xff;
		adaptBuf[adaptivePos + 4] = ((opcr_base << 7) & 0x80) | ((opcr_ext >> 8) & 0x01);
		adaptBuf[adaptivePos + 5] = (opcr_ext) & 0xff;
		adaptivePos += 6;
	}
	if(adaptationField->splicing_point_flag)
	{
		adaptiveFlags |= 0x04;

		adaptBuf[adaptivePos] = adaptationField->splice_countdown;
		adaptivePos += 1;
	}
	if(adaptationField->transport_private_data_flag)
	{
		adaptiveFlags |= 0x02;

		if(adaptationField->transport_private_data_len + 1 > adaptFieldLen - adaptivePos)
			return -2;
		else
		{
			adaptBuf[adaptivePos] = adaptationField->transport_private_data_len;
			memcpy(&adaptBuf[adaptivePos + 1], adaptationField->transport_private_data, adaptBuf[adaptivePos]);
			adaptivePos += (1 + adaptBuf[adaptivePos]);
		}
	}
	if(adaptationField->adaptation_field_extension_flag)
	{
		adaptiveFlags |= 0x01;

		adaptBuf[adaptivePos] = 1;
		adaptBuf[adaptivePos + 1] = 0;
	}

	adaptBuf[0] = adaptiveFlags;

	return 0;
}

static int generateTsHeaderData(uint16_t pid, uint8_t payloadStartInd, uint8_t adaptFieldControl, uint8_t *outTsHeaderData)
{
	T_TsHeader tsHeader = {0};
	static uint8_t continuity_counter_pat = 0; /* 0~f */
	static uint8_t continuity_counter_pmt = 0; /* 0~f */
	static uint8_t continuity_counter_audio = 0; /* 0~f */
	static uint8_t continuity_counter_video = 0; /* 0~f */

	if(!outTsHeaderData)
	{
		printf("[%s:%d] Params invalid!\n", __FUNCTION__, __LINE__);
		return -1;
	}


	/* fix */
	tsHeader.sync_byte = TS_HEADER_SYNC_BYTE;
	tsHeader.transport_error_indicator = 0;
	tsHeader.payload_unit_start_indicator = payloadStartInd;
	tsHeader.transport_priority = 0;
	tsHeader.pid = pid;
	tsHeader.transport_scrambling_control = 0;
	tsHeader.adaptation_field_control = adaptFieldControl;

	switch(pid)
	{
		case TS_PID_PAT:  tsHeader.continuity_counter = continuity_counter_pat++ % 0x10;   break;
		case TS_PID_PMT:  tsHeader.continuity_counter = continuity_counter_pmt++ % 0x10;   break;
		case TS_PID_AAC:  tsHeader.continuity_counter = continuity_counter_audio++ % 0x10; break;
		case TS_PID_H264: tsHeader.continuity_counter = continuity_counter_video++ % 0x10; break;
		default: return -2;
	}

	/* save */
	outTsHeaderData[0] = tsHeader.sync_byte;
	outTsHeaderData[1] = tsHeader.transport_error_indicator << 7 |\
						 tsHeader.payload_unit_start_indicator << 6 |\
						 tsHeader.transport_priority << 5 |\
						 ((tsHeader.pid >> 8) & 0x1f);
	outTsHeaderData[2] = tsHeader.pid & 0x00ff;
	outTsHeaderData[3] = tsHeader.transport_scrambling_control << 6 |\
						 tsHeader.adaptation_field_control << 4 |\
						 tsHeader.continuity_counter;

	return 0;
}


static int packPat2TsAndWriteToFile(FILE *fpTsFile)
{
	T_Pat pat = {};
	uint8_t tsBuf[TS_PACKET_SIZE] = {0};
	uint32_t crc_32 = 0xFFFFFFFF;

	if(!fpTsFile)
	{
		printf("[%s:%d] Params invalid!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	/* 1. fix params */
	pat.table_id = 0x00;
	pat.section_syntax_indicator = 0b1;
	pat.zero = 0b0;
	pat.reserved_1 = 0b11;
	pat.section_length = 13; 			// 16 Bytes(pat total size) - 3 Bytes(sizeof table_id~section_length)
	pat.transport_stream_id = 0x0001;
	pat.reserved_2 = 0b11;
	pat.version_number = 0b00000;
	pat.current_next_indicator = 0b1; 	// available
	pat.section_number = 0x00;
	pat.last_section_number = 0x00;
	pat.program_number = 0x0001; 		// PMT
	pat.reserved_3 = 0b111;
	pat.pid = TS_PID_PMT; 				// program_number: 0x0001 -> pmt's pid
	//pat.crc_32 = crc_32; 				// we'll calculate when write to buf.

	/* 2. fix ts data */
	/* 2.1 fix ts data: ts header */
	generateTsHeaderData(TS_PID_PAT, 0b1, 0b01, tsBuf); // 0b1: start, 0b01: playload.

	/* 2.2 fix ts data: adaptation field length */
	tsBuf[4] = 0; /* no adaption field */

	/* 2.3 fix ts data: pat data */
	tsBuf[5] = pat.table_id;
	tsBuf[6] = pat.section_syntax_indicator << 7 | pat.zero  << 6 | pat.reserved_1 << 4 | ((pat.section_length >> 8) & 0x0F);
	tsBuf[7] = pat.section_length & 0x00FF;
	tsBuf[8] = pat.transport_stream_id >> 8;
	tsBuf[9] = pat.transport_stream_id & 0x00FF;
	tsBuf[10] = pat.reserved_2 << 6 | pat.version_number << 1 | pat.current_next_indicator;
	tsBuf[11] = pat.section_number;
	tsBuf[12] = pat.last_section_number;
	tsBuf[13] = pat.program_number>>8;
	tsBuf[14] = pat.program_number & 0x00FF;
	tsBuf[15]= pat.reserved_3 << 5 | ((pat.pid >> 8) & 0x0F);
	tsBuf[16]= pat.pid & 0x00FF;
	crc_32 = endian_convert(crc32_mpeg_2(&tsBuf[5], 16 - 5 + 1));
	memcpy(&tsBuf[17], (unsigned char *)&crc_32, 4);

	/* 2.4 fix ts data: clear with "0xff" */
	memset(&tsBuf[21], 0xff, TS_PACKET_SIZE - 21);

	/* 3. write to file */
	fwrite(tsBuf, 1, TS_PACKET_SIZE, fpTsFile);

	return 0;
}


static int packPmt2TsAndWriteToFile(FILE *fpTsFile)
{
	T_Pmt pmt;
	uint8_t tsBuf[TS_PACKET_SIZE] = {0};
	uint32_t crc_32 = 0xFFFFFFFF;

	if(!fpTsFile)
	{
		printf("[%s:%d] Params invalid!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	/* 1. fix params */
	pmt.table_id = 0x02;
	pmt.section_syntax_indicator = 0b1;
	pmt.zero = 0b0;
	pmt.reserved_1 = 0b11;
	pmt.section_length = 23; 			// 16 + 5 + 5 - 3
	pmt.program_number = 1; 			// only one program
	pmt.reserved_2 = 0b11;
	pmt.version_number = 0b0000;
	pmt.current_next_indicator = 0b1; 	// available
	pmt.section_number = 0x00;
	pmt.last_section_number = 0x00;
	pmt.reserved_3 = 0b111;
	pmt.pcr_pid = TS_PID_H264;
	pmt.reserved_4 = 0b1111;
	pmt.program_info_length = 0x000;
	pmt.stream_type_video = PMT_STREAM_TYPE_H264; 	// video type
	pmt.reserved_5_video = 0b111;
	pmt.elementary_pid_video = TS_PID_H264; 		// video pid
	pmt.reserved_6_video= 0b1111;
	pmt.es_info_length_video = 0x000;
	pmt.stream_type_audio = PMT_STREAM_TYPE_AAC; 	// audio type
	pmt.reserved_5_audio = 0b111;
	pmt.elementary_pid_audio = TS_PID_AAC; 			// audio pid
	pmt.reserved_6_audio = 0b1111;
	pmt.es_info_length_audio = 0x000;
	//pmt.crc_32 = crc_32; 							// we'll calculate when write to buf.

	/* 2. fix ts data */
	/* 2.1 fix ts data: ts header */
	generateTsHeaderData(TS_PID_PMT, 0b1, 0b01, tsBuf); // 0b1: start, 0b01: playload.

	/* 2.2 fix ts data: adaptation field length */
	tsBuf[4] = 0; /* no adaption field */

	/* 2.3 fix ts data: pmt data */
	tsBuf[5] = pmt.table_id;
	tsBuf[6] = pmt.section_syntax_indicator << 7 | pmt.zero  << 6 | pmt.reserved_1 << 4 | ((pmt.section_length >> 8) & 0x0F);
	tsBuf[7] = pmt.section_length & 0x00FF;
	tsBuf[8] = pmt.program_number >> 8;
	tsBuf[9] = pmt.program_number & 0x00FF;
	tsBuf[10] = pmt.reserved_2 << 6 | pmt.version_number << 1 | pmt.current_next_indicator;
	tsBuf[11] = pmt.section_number;
	tsBuf[12] = pmt.last_section_number;
	tsBuf[13] = pmt.reserved_3 << 5  | ((pmt.pcr_pid >> 8) & 0x1F);
	tsBuf[14] = pmt.pcr_pid & 0x0FF;
	tsBuf[15]= pmt.reserved_4 << 4 | ((pmt.program_info_length >> 8) & 0x0F);
	tsBuf[16]= pmt.program_info_length & 0xFF;
	tsBuf[17]= pmt.stream_type_video;
	tsBuf[18]= pmt.reserved_5_video << 5 | ((pmt.elementary_pid_video >> 8 ) & 0x1F);
	tsBuf[19]= pmt.elementary_pid_video & 0x00FF;
	tsBuf[20]= pmt.reserved_6_video<< 4 | ((pmt.es_info_length_video >> 8) & 0x0F);
	tsBuf[21]= pmt.es_info_length_video & 0x0FF;
	tsBuf[22]= pmt.stream_type_audio;
	tsBuf[23]= pmt.reserved_5_audio<< 5 | ((pmt.elementary_pid_audio >> 8 ) & 0x1F);
	tsBuf[24]= pmt.elementary_pid_audio & 0x00FF;
	tsBuf[25]= pmt.reserved_6_audio << 4 | ((pmt.es_info_length_audio >> 8) & 0x0F);
	tsBuf[26]= pmt.es_info_length_audio & 0x0FF;
	crc_32 = endian_convert(crc32_mpeg_2(&tsBuf[5], 26 - 5 + 1));
	memcpy(&tsBuf[27], (unsigned char  *)&crc_32, 4);

	/* 2.4 fix ts data: clear with "0xff" */
	memset(&tsBuf[31], 0xff, TS_PACKET_SIZE - 31);

	/* 3. write to file */
	fwrite(tsBuf, 1, TS_PACKET_SIZE, fpTsFile);

	return 0;
}


static int packPes2TsAndWriteToFile(T_PesEsStruct* ptPesStu,
											uint16_t pid,
											T_AdaptationField *adaptionField,
											uint64_t pts,
											FILE *fpTsFile)
{
	uint8_t tsBuf[TS_PACKET_SIZE] = {0};
	uint8_t tsBufPos = 0;
	uint8_t stuffingLen = 0;
	uint32_t firstPacketLoadLen = 0;

	if (!ptPesStu || !adaptionField || !fpTsFile)
	{
		printf("[%s:%d] Params invalid!\n", __FUNCTION__, __LINE__);
		return -1;
	}


	/* calcurate the first packet playload length */
	firstPacketLoadLen = TS_PACKET_SIZE - TS_PACKET_HEADER_SIZE
						- adaptionField->adaptation_field_length
						- 1  /* sizeof adaptation_field_length */
						- 9  /* pes header(not contain pts/dts) */
						- 5; /* pts */

	if(ptPesStu->av_data_len < firstPacketLoadLen)
	{
		/* 1. ts header */
		generateTsHeaderData(pid, 0b1, 0b11, tsBuf); // 0b1: start, 0b11: adaption_field + playload.
		tsBufPos += TS_PACKET_HEADER_SIZE;

		/* 2. adaptation field */
		/* 2.1 adaptation field: adaptation field length */
		tsBuf[tsBufPos] = TS_PACKET_SIZE - TS_PACKET_HEADER_SIZE
						 - ptPesStu->av_data_len
						 - 1  /* sizeof adaptation_field_length */
						 - 9  /* pes header(not contain pts/dts) */
						 - 5; /* pts */

		/* 2.2 adaptation field: discontinuty_indicator ~ adaptation_field_extension_flag. (0x00: no optional fields) */
		tsBuf[tsBufPos + 1] = 0x00;
		tsBufPos += 2;

		/* 2.3 adaptation field: stuffing bytes */
		stuffingLen = TS_PACKET_SIZE - TS_PACKET_HEADER_SIZE
						  - ptPesStu->av_data_len
						  - 2  /* adaptation_field_length ~ adaptation_field_extension_flag */
						  - 9  /* pes header(not contain pts/dts) */
						  - 5; /* pts */
		memset(tsBuf + tsBufPos, 0xff, stuffingLen);
		tsBufPos += stuffingLen;

		/* 3. payload */
		/* 3.1 payload: pes header. (not contain pts/dts) */
		tsBuf[tsBufPos + 0] = (ptPesStu->packet_start_code_prefix >> 16) & 0xff;
		tsBuf[tsBufPos + 1] = (ptPesStu->packet_start_code_prefix >> 8) & 0xff;
		tsBuf[tsBufPos + 2] = ptPesStu->packet_start_code_prefix & 0xff;
		tsBuf[tsBufPos + 3] = ptPesStu->stream_id;
		tsBuf[tsBufPos + 4] = (ptPesStu->pes_packet_length >> 8) & 0xff;
		tsBuf[tsBufPos + 5] = ptPesStu->pes_packet_length & 0xff;
		tsBuf[tsBufPos + 6] = ptPesStu->marker_bit << 6 |\
							  ptPesStu->pes_scrambling_control << 4 |\
							  ptPesStu->pes_priority << 3 |\
							  ptPesStu->data_alignment_indicator << 2 |\
							  ptPesStu->copyright << 1 |\
							  ptPesStu->original_or_copy;
		tsBuf[tsBufPos + 7] = ptPesStu->pts_dts_flags << 6 |\
							  ptPesStu->escr_flag << 5 |\
							  ptPesStu->es_rate_flag << 4 |\
							  ptPesStu->dsm_trick_mode_flag << 3 |\
							  ptPesStu->additional_copy_info_flag << 2 |\
							  ptPesStu->pes_crc_flag << 1 |\
							  ptPesStu->pes_extension_flag;
		tsBuf[tsBufPos + 8] = ptPesStu->pes_data_length;
		tsBufPos += 9;

		/* 3.2 payload: pts */
		tsBuf[tsBufPos + 0] = (((0x3 << 4) | ((pts>> 29) & 0x0E) | 0x01) & 0xff);
		tsBuf[tsBufPos + 1] = (((((pts >> 14) & 0xfffe) | 0x01) >> 8) & 0xff);
		tsBuf[tsBufPos + 2] = ((((pts >> 14) & 0xfffe) | 0x01) & 0xff);
		tsBuf[tsBufPos + 3] = (((((pts << 1) & 0xfffe) | 0x01) >> 8) & 0xff);
		tsBuf[tsBufPos + 4] = ((((pts << 1) & 0xfffe) | 0x01) & 0xff);
		tsBufPos += 5;

		/* 3.3 payload: es. (audio or video data) */
		memcpy(tsBuf + tsBufPos, ptPesStu->av_data, ptPesStu->av_data_len);

		/* 4. write to file */
		fwrite(tsBuf, 1, TS_PACKET_SIZE, fpTsFile);

		return 0;
	}
	else
	{
		/* 1. ts header */
		generateTsHeaderData(pid, 0b1, 0b11, tsBuf); // 0b1: start, 0b11: adaption_field + playload.
		tsBufPos += TS_PACKET_HEADER_SIZE;

		/* 2. adaptation field */
		/* 2.1 adaptation field: adaptation field length */
		tsBuf[tsBufPos] = adaptionField->adaptation_field_length;
		tsBufPos += 1;

		/* 2.2 adaptation field: discontinuty_indicator ~ adaptation_field_extension_flag.
		 *     whether the pcr_flag is equal to 1 in this demo,
		 *     and "adaptionField->adaptation_field_length = 1 or 7"
		 */
		fixFirstPacketAdaptionField(adaptionField,
								    (TS_PACKET_SIZE - TS_PACKET_HEADER_SIZE - 1 - 9 - 5),
									&tsBuf[tsBufPos]);
		tsBufPos += adaptionField->adaptation_field_length;

		/* 3. payload*/
		/* 3.1 payload: pes header. (not contain pts/dts) */
		tsBuf[tsBufPos + 0] = (ptPesStu->packet_start_code_prefix >> 16) & 0xFF;
		tsBuf[tsBufPos + 1] = (ptPesStu->packet_start_code_prefix >> 8) & 0xFF;
		tsBuf[tsBufPos + 2] = ptPesStu->packet_start_code_prefix & 0xFF;
		tsBuf[tsBufPos + 3] = ptPesStu->stream_id;
		tsBuf[tsBufPos + 4] = ((ptPesStu->pes_packet_length) >> 8) & 0xFF;
		tsBuf[tsBufPos + 5] = (ptPesStu->pes_packet_length) & 0xFF;
		tsBuf[tsBufPos + 6] = ptPesStu->marker_bit << 6 | ptPesStu->pes_scrambling_control << 4 | ptPesStu->pes_priority << 3 |
							  ptPesStu->data_alignment_indicator << 2 | ptPesStu->copyright << 1 |ptPesStu->original_or_copy;
		tsBuf[tsBufPos + 7] = ptPesStu->pts_dts_flags << 6 |ptPesStu->escr_flag << 5 | ptPesStu->es_rate_flag << 4 |
								ptPesStu->dsm_trick_mode_flag << 3 | ptPesStu->additional_copy_info_flag << 2 | ptPesStu->pes_crc_flag << 1 | ptPesStu->pes_extension_flag;
		tsBuf[tsBufPos + 8] = ptPesStu->pes_data_length;
		tsBufPos += 9;

		/* 3.2 payload: pts */
		tsBuf[tsBufPos + 0] = (((0x3 << 4) | ((pts>> 29) & 0x0E) | 0x01) & 0xff);
		tsBuf[tsBufPos + 1] = (((((pts >> 14) & 0xfffe) | 0x01) >> 8) & 0xff);
		tsBuf[tsBufPos + 2] = ((((pts >> 14) & 0xfffe) | 0x01) & 0xff);
		tsBuf[tsBufPos + 3] = (((((pts << 1) & 0xfffe) | 0x01) >> 8) & 0xff);
		tsBuf[tsBufPos + 4] = ((((pts << 1) & 0xfffe) | 0x01) & 0xff);
		tsBufPos += 5;

		/* 3.3 payload: es. (audio or video data) */
		ptPesStu->av_data_cur_ptr = ptPesStu->av_data;
		memcpy(tsBuf + tsBufPos, ptPesStu->av_data_cur_ptr, firstPacketLoadLen);

		/* 4. write to file */
		fwrite(tsBuf, 1, TS_PACKET_SIZE, fpTsFile);

		/* >> change the pointer to pack remain es(Elementary Stream) data! << */
		ptPesStu->av_data_cur_ptr += firstPacketLoadLen;
		ptPesStu->av_data_len     -= firstPacketLoadLen;

		/* >> pack remain es(Elementary Stream) data! << */
		while(ptPesStu->av_data_len)
		{
			tsBufPos = 0;
			memset(tsBuf, 0, TS_PACKET_SIZE);

			if(ptPesStu->av_data_len >= (TS_PACKET_SIZE - TS_PACKET_HEADER_SIZE)) /* 184 */
			{
				generateTsHeaderData(pid, 0b0, 0b01, tsBuf); // 0b0: It isn't the start, 0b01: only playload.
				tsBufPos += TS_PACKET_HEADER_SIZE;
				memcpy(tsBuf + tsBufPos, ptPesStu->av_data_cur_ptr, (TS_PACKET_SIZE - TS_PACKET_HEADER_SIZE));

				fwrite(tsBuf, 1, TS_PACKET_SIZE, fpTsFile);

				/* >> change the pointer to pack remain es(Elementary Stream) data! << */
				ptPesStu->av_data_cur_ptr += (TS_PACKET_SIZE - TS_PACKET_HEADER_SIZE);
				ptPesStu->av_data_len     -= (TS_PACKET_SIZE - TS_PACKET_HEADER_SIZE);
			}
			else if(ptPesStu->av_data_len == (TS_PACKET_SIZE - TS_PACKET_HEADER_SIZE - 1) ||\
					ptPesStu->av_data_len == (TS_PACKET_SIZE - TS_PACKET_HEADER_SIZE - 2)) /* 183 or 182 */
			{
				generateTsHeaderData(pid, 0b0, 0b11, tsBuf); // 0b0: It isn't the start, 0b11: adaption_field + playload.
				tsBufPos += TS_PACKET_HEADER_SIZE;

				tsBuf[tsBufPos + 0] = 0x01;
				tsBuf[tsBufPos + 1] = 0x00;
				tsBufPos += 2;

				memcpy(&tsBuf[tsBufPos], ptPesStu->av_data_cur_ptr, (TS_PACKET_SIZE - TS_PACKET_HEADER_SIZE - 2));

				fwrite(tsBuf, 1, TS_PACKET_SIZE, fpTsFile);

				/* >> change the pointer to pack remain es(Elementary Stream) data! << */
				ptPesStu->av_data_cur_ptr += (TS_PACKET_SIZE - TS_PACKET_HEADER_SIZE - 2);
				ptPesStu->av_data_len     -= (TS_PACKET_SIZE - TS_PACKET_HEADER_SIZE - 2);
			}
			else
			{
				generateTsHeaderData(pid, 0b0, 0b11, tsBuf); // 0b0: It isn't the start, 0b11: adaption_field + playload.
				tsBufPos += TS_PACKET_HEADER_SIZE;

				tsBuf[tsBufPos + 0] = TS_PACKET_SIZE - TS_PACKET_HEADER_SIZE - ptPesStu->av_data_len - 1;
				tsBuf[tsBufPos + 1] = 0x00;
				tsBufPos += 2;

				memset(&tsBuf[tsBufPos], 0xff, (TS_PACKET_SIZE - TS_PACKET_HEADER_SIZE - ptPesStu->av_data_len - 2));
				tsBufPos += (TS_PACKET_SIZE - TS_PACKET_HEADER_SIZE - ptPesStu->av_data_len - 2);

				memcpy(&tsBuf[tsBufPos], ptPesStu->av_data_cur_ptr, ptPesStu->av_data_len);

				fwrite(tsBuf, 1, TS_PACKET_SIZE, fpTsFile);

				/* >>  es(Elementary Stream) data empty! << */
				ptPesStu->av_data_len = 0;
			}
		}
	}

	return 0;
}




int ts_mux_h264_aac(char *h264FileName, uint32_t vFps, char *aacFileName, char *tsFileName)
{
	int ret = -1;
	uint64_t timeStamp_us_a = 0;
	uint64_t timeStamp_us_v = 0;
	uint32_t videoFps = vFps;
	uint32_t audioSampleRate = -1;
	FILE *fpH264 = NULL;
	FILE *fpAAC  = NULL;
	FILE *fpTs  = NULL;
	uint8_t *h264Buf = NULL;
	uint8_t *aacBuf = NULL;
	T_AdtsHeader adtsHeader = {};
	T_NaluInfo naluInfo = {};
	T_PesEsStruct pesStu = {};
	T_AdaptationField adaptationField = {};

	if (!h264FileName || !videoFps || !aacFileName || !tsFileName)
	{
		printf("[%s:%d] Params invalid!\n", __FUNCTION__, __LINE__);
		return -1;
	}

	/* open file */
	fpH264 = fopen(h264FileName, "rb");
	if (!fpH264)
	{
		DEBUG("[%s:%d] open %s error!\n", __FUNCTION__, __LINE__, h264FileName);
		return -2;
	}
	fpAAC  = fopen(aacFileName,  "rb");
	if (!fpAAC)
	{
		DEBUG("[%s:%d] open %s error!\n", __FUNCTION__, __LINE__, aacFileName);
		return -3;
	}
	fpTs  = fopen(tsFileName,  "wb");
	if (!fpTs)
	{
		DEBUG("[%s:%d] open %s error!\n", __FUNCTION__, __LINE__, tsFileName);
		return -4;
	}

	/* alloc tmp memory */
	h264Buf = (uint8_t *)malloc(MAX_NALU_SIZE);
	if (!h264Buf)
	{
		DEBUG("[%s:%d] malloc error!\n", __FUNCTION__, __LINE__);
		return -5;
	}
	aacBuf = (uint8_t *)malloc(MAX_ADTS_SIZE);
	if (!aacBuf)
	{
		DEBUG("[%s:%d] malloc error!\n", __FUNCTION__, __LINE__);
		return -6;
	}

	/* parse AAC-ADTS */
	ret = getAdtsFrame(fpAAC, aacBuf, &adtsHeader);
	if(!ret)
	{
		fseek(fpAAC, 0, SEEK_SET); // reset
		switch(adtsHeader.sampling_freq_index)
		{
			case SFI_96000: audioSampleRate = 96000; break;
			case SFI_88200: audioSampleRate = 88200; break;
			case SFI_64000: audioSampleRate = 64000; break;
			case SFI_48000: audioSampleRate = 48000; break;
			case SFI_44100: audioSampleRate = 44100; break;
			case SFI_32000: audioSampleRate = 32000; break;
			case SFI_24000: audioSampleRate = 24000; break;
			case SFI_22050: audioSampleRate = 22050; break;
			case SFI_16000: audioSampleRate = 16000; break;
			case SFI_12000: audioSampleRate = 12000; break;
			case SFI_11025: audioSampleRate = 11025; break;
			case SFI_8000:  audioSampleRate =  8000; break;
			case SFI_7350:  audioSampleRate =  7350; break;
			default:        audioSampleRate =     0; break;
		}
		DEBUG("AAC Info:\n"
			  "\t id: %d\n"
			  "\t profile: %d\n"
			  "\t freq index: %d\n"
			  "\t sample rate: %d)\n"
			  "\t channels: %d\n",
			  adtsHeader.id, adtsHeader.profile,
			  adtsHeader.sampling_freq_index, audioSampleRate,
			  adtsHeader.channel_configuration);
	}


	#if 1 /* The PAT and PMT can show anywhere. */
	ret = packPat2TsAndWriteToFile(fpTs);
	if(ret < 0)
		goto mux_end;

	ret = packPmt2TsAndWriteToFile(fpTs);
	if(ret < 0)
		goto mux_end;
	#endif

	while(1)
	{
		if(timeStamp_us_a > timeStamp_us_v)
		{
			ret = getOneH264Nalu(fpH264, h264Buf, &naluInfo);
			if (ret < 0)
			{
				if(ret == -2)
					DEBUG("video file end!\n");
				else
					printf("[%s:%d] getOneH264Nalu(...) error with %d!\n", __FUNCTION__, __LINE__, ret);

				goto mux_end;
			}
			DEBUG("\033[31m[%02d:%02d:%02d.%03d] get one h264 nalu(%d) with length: %d \033[0m\n",
									(uint32_t)(timeStamp_us_v*1000/TIME_SCALE / (60*60*1000) % 60), /* time: timeStamp_us_v * videoFps/TIME_SCALE * 1000/videoFps */
									(uint32_t)(timeStamp_us_v*1000/TIME_SCALE / (60*1000) % 60),
									(uint32_t)(timeStamp_us_v*1000/TIME_SCALE / (1000) % 60),
									(uint32_t)(timeStamp_us_v*1000/TIME_SCALE % (1000)),
									naluInfo.nalu_type,
									naluInfo.data_len);

			ret = fixH264FrameToPesStruct(h264Buf, naluInfo.data_len, timeStamp_us_v, &pesStu);
			if (ret < 0)
			{
				printf("[%s:%d] fixH264FrameToPesStruct(...) error with %d!\n", __FUNCTION__, __LINE__, ret);
				goto mux_end;
			}

			if(naluInfo.nalu_type == NALU_TYPE_IDR || naluInfo.nalu_type == NALU_TYPE_SLICE)
			{
				#if 1 /* The PAT and PMT can show anywhere. */
				ret = packPat2TsAndWriteToFile(fpTs);
				if(ret < 0)
					goto mux_end;

				ret = packPmt2TsAndWriteToFile(fpTs);
				if(ret < 0)
					goto mux_end;
				#endif

				fixFirstPacketAdaptionFieldStruct(&adaptationField, timeStamp_us_v);
				packPes2TsAndWriteToFile(&pesStu, TS_PID_H264, &adaptationField, timeStamp_us_v, fpTs);

				timeStamp_us_v += TIME_SCALE/videoFps;
			}
			else
			{
				fixCommonAdaptionFieldStruct(&adaptationField);
				packPes2TsAndWriteToFile(&pesStu, TS_PID_H264, &adaptationField, timeStamp_us_v, fpTs);
			}

		}
		else
		{
			ret = getAdtsFrame(fpAAC, aacBuf, &adtsHeader);
			if (ret)
			{
				if(ret == -2)
					DEBUG("audio file end!\n");
				else
					printf("[%s:%d] getAdtsFrame(...) error with %d!\n", __FUNCTION__, __LINE__, ret);
				goto mux_end;
			}
			DEBUG("\033[32m[%02d:%02d:%02d.%03d] get one adts frame with length: %d \033[0m\n",
									(uint32_t)(timeStamp_us_a/90 / (60*60*1000) % 60),
									(uint32_t)(timeStamp_us_a/90 / (60*1000) % 60),
									(uint32_t)(timeStamp_us_a/90 / (1000) % 60),
									(uint32_t)(timeStamp_us_a/90 % (1000)),
									adtsHeader.aac_frame_length);

			ret = fixAdtsFrameToPesStruct(aacBuf, adtsHeader.aac_frame_length, timeStamp_us_a, &pesStu);
			if (ret < 0)
			{
				printf("[%s:%d] fixAdtsFrameToMyPesStruct(...) error with %d!\n", __FUNCTION__, __LINE__, ret);
				goto mux_end;
			}

			fixCommonAdaptionFieldStruct(&adaptationField);
			packPes2TsAndWriteToFile(&pesStu, TS_PID_AAC, &adaptationField, timeStamp_us_a, fpTs);

			timeStamp_us_a += TIME_SCALE*1024/audioSampleRate;
		}
	}

mux_end:
	if(h264Buf) free(h264Buf);
	if(aacBuf) free(aacBuf);
	if(fpH264) fclose(fpH264);
	if(fpAAC)  fclose(fpAAC);
	if(fpTs)  {fflush(fpTs); fclose(fpTs);}

	return 0;
}


