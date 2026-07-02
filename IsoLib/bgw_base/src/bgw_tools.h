/***********************************************************************************
 This software module was originally developed by

 Fraunhofer HHI

 in the course of development of the ISO/IEC 23003-8 / ITU-T T.261 for reference purposes and its
 performance may not have been optimized. This software module is an implementation
 of one or more tools as specified by the ISO/IEC 23003-8 / ITU-T T.261 standard. ISO/IEC gives
 you a royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
 and make derivative works of this software module or modifications  thereof for use
 in implementations or products claiming conformance to the  ISO/IEC 23003-8 / ITU-T T.261 standard
 and which satisfy any specified conformance criteria. Those intending to use this
 software module in products are advised that its use may infringe existing patents.
 ISO/IEC have no liability for use of this software module or modifications thereof.
 Copyright is not released for products that do not conform to the ISO/IEC 23003-8 / ITU-T T.261
 standard.
 
 Fraunhofer HHI retains full right to modify and use the code for its
 own purpose, assign or donate the code to a third party and to inhibit third parties
 from using the code for products that do not conform to MPEG-related ITU Recommenda-
 tions and/or ISO/IEC International Standards.

 This copyright notice must be included in all copies or derivative works.

 Copyright (c) ISO/IEC 2026.
 
 ***********************************************************************************/

#ifndef INCLUDED_FAVS_TOOLS_H
#define INCLUDED_FAVS_TOOLS_H

/* BGW 4CC Codes c*/
enum
{
  BGW_4CC_BGW1 = MP4_FOUR_CHAR_CODE('b', 'g', 'w', '1'),
  BGW_4CC_BGWB = MP4_FOUR_CHAR_CODE('b', 'g', 'w', 'b'),
  BGW_4CC_BGW2 = MP4_FOUR_CHAR_CODE('b', 'g', 'w', '2'),
  BGW_4CC_BGWC = MP4_FOUR_CHAR_CODE('b', 'g', 'w', 'C')
};

/* BGW packet types */
enum
{
  /* Forbidden stream packet type for start code emulation prevention */
  BGW_FORBIDDEN_SPT = 0,
  /* Waveform parameter set */
  BGW_WPS_SPT = 1,
  /* Channel group parameter set */
  BGW_CGPS_SPT = 2,
  /* Timestamp */
  BGW_TIMESTAMP_SPT = 3,
  /* Independent frame */
  BGW_IF_SPT = 4,
  /* Dependent frame */
  BGW_DF_SPT = 5,
  /* Segment payload */
  BGW_SEGMENT_SPT = 6,
  /* Annotation channel */
  BGW_AC_SPT = 7,
  /* Feature set */
  BGW_FEATURE_SPT = 8,
  /* Synchronization */
  BGW_SYNC_SPT = 9,
  /* Local CRC 16 */
  BGW_LOCAL_CRC16_SPT = 10,
  /* Local CRC 32 */
  BGW_LOCAL_CRC32_SPT = 11,
  /* User identifier */
  BGW_UUID_U_SPT = 12,
  /* Stream identifier */
  BGW_UUID_S_SPT = 13,
  /* Global CRC 16 */
  BGW_GLOBAL_CRC16_SPT = 14,
  /* Global CRC 32 */
  BGW_GLOBAL_CRC32_SPT = 15,
  /* Authentication start */
  BGW_AUTH_START_SPT = 16,
  /* Authentication signature */
  BGW_AUTH_SIG_SPT = 17,
  /* Auxiliary metadata */
  BGW_AM_SPT = 18,
  /* Configuration set */
  BGW_CONFIG_SET_SPT = 19
};

/* Waveform types */
enum
{
  BGW_WAVEFORM_TYPE_WT_GENERIC = 0,
  BGW_WAVEFORM_TYPE_WT_BS2088 = 1
};

/* Default sample duration */
// @todo Define a proper default sample duration
u32 DEFAULT_SAMPLE_DURATION;

/**
 * @brief Initializes a bit buffer to retrieve a sequence of bits.
 * @param bb BitBuffer struct.
 * @param p Pointer to the data.
 * @param length Length of the data in bytes.
 * @return MP4NoErr on success, a negative integer on error.
 * @todo Merge with BitBuffer_Init present in other files.
 */
MP4Err bit_buffer_init(BitBuffer *bb, u8 *p, u32 length);

/**
 * @brief Calculates the Ceil(log2(x)) of a unsigned integer.
 * @param x Number.
 * @return MP4NoErr on success, a negative integer on error.
 */
u32 ceil_log2(u32 x);

/**
 * @brief Gets the following sequence of bits from a BitBuffer.
 * @param bb BitBuffer struct.
 * @param nBits Number of bits.
 * @param errout Output error
 * @return Bits read as a u32.
 * @todo Merge with GetBits present in other files.
 */
u32 get_bits(BitBuffer *bb, u32 nBits, MP4Err *errout);

/**
 * @brief Gets the following sequence of bytes from a BitBuffer.
 * @param bb BitBuffer struct.
 * @param nBytes Number of bytes.
 * @param p Retrieved data.
 * @return MP4NoErr on success, a negative integer on error.
 * @todo Merge with GetBytes present in other files.
 */
MP4Err get_bytes(BitBuffer *bb, u32 nBytes, u8 *p);

/**
 * @brief Reads an unsigned exp-golomb-coded value from a BitBuffer.
 * @param bb BitBuffer struct.
 * @param errout Output error.
 * @return Value read as a u32.
 * @todo Merge with read_golomb_uev present in other files.
 */
u32 read_golomb_uev(BitBuffer *bb, MP4Err *errout);

/**
 * @brief Reads an signed exp-golomb-coded value from a BitBuffer.
 * @param bb BitBuffer struct.
 * @param errout Output error.
 * @return Value read as a s32.
 */
s32 read_golomb_sev(BitBuffer *bb, MP4Err *errout);

/**
 * @brief Reads an escaped value from a BitBuffer.
 * @param bb BitBuffer struct.
 * @param value Output value.
 * @param k First number of bits to be read.
 * @param m Second number of bits to be read.
 * @param n Third number of bits to be read.
 * @param errout Output error.
 * @return Value read as a u32.
 */
u32 read_escaped_value(BitBuffer *bb, u32 *value, int k, int m, int n, MP4Err *errout);

/** 
 * @brief Reads a null-terminated UTF-8 string (st(v)) from the bitstream. The bitstream must be byte-aligned before calling.
 * @param bb BitBuffer struct.
 * @param errout Output error.
 * @return A heap-allocated string (caller must free), or NULL on error. 
 */
char* read_string_stv(BitBuffer *bb, MP4Err *errout);

/**
 * @brief Parses the CLI input parameters.
 * @param argc Number of parameters.
 * @param argv Parameters array.
 * @param parameters Output struct to store the parameters.
 * @return 1 on success, 0 on error.
 */
int parse_input_params(int argc, char* argv[], bgw_params *parameters);

#endif
