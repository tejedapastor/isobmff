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


#ifndef __BGW_H__
#define __BGW_H__

#include "ISOMovies.h"
#include "bgw_structures.h"
#include "bgw_tools.h"
#include <stdio.h>

/**
* @brief Creates and populates a bgw_stream struct from a file containing a T.261 bitstream.
* @param input Path to input file.
* @param stream Struct that will store the main information of the T.261 bitstream.
* @return MP4NoErr on success, a negative integer on error.
*/
MP4Err bgw_parse_stream(FILE* input, bgw_stream **stream);

/**
* @brief Carries out specific actions for each stream packet type.
* @param packetType Type of the stream packet.
* @param packetLength Length in bytes of the payload of the stream packet.
* @param packetPayload Payload of the stream packet.
* @param packetLabel Substream indication of the stream packet.
* @param stream Struct containing the main information of the T.261 stream.
* @return MP4NoErr on success, a negative integer on error.
*/
MP4Err bgw_handle_packet_data(u32 packetType, u32 packetLength, u8* packetPayload, u64 packetLabel, bgw_stream** stream);

/**
* @brief Initializes a new frame sequence for an independent frame.
* @param independentFrame Struct containing the information of the independent frame.
* @param stream Struct containing the main information of the T.261 stream.
* @return MP4NoErr on success, a negative integer on error.
*/
MP4Err bgw_create_new_frame_sequence(bgw_independent_frame* independentFrame, bgw_stream** stream);

/**
* @brief Inserts the information of a dependent frame into an existing frame sequence.
* @param dependentFrame Struct containing the information of the dependent frame.
* @param stream Struct containing the main information of the T.261 stream.
* @return MP4NoErr on success, a negative integer on error.
*/
MP4Err bgw_insert_df_in_frame_sequence(bgw_dependent_frame* dependentFrame, bgw_stream** stream);

/**
* @brief Checks if the indicated label (signalling the substream) has appeared previously in the bitstream.
* If not, it adds it to the list of different labels.
* @param streamPacketLabels Lists of IDs of the previously found labels.
* @param numDifferentLabels Number of different labels already found in the bitstream.
* @param newLabel New label.
* @return MP4NoErr on success, a negative integer on error.
*/
MP4Err bgw_check_new_label(u32** streamPacketLabels, u32* numDifferentLabels, u32 newLabel);

/**
* @brief Parses a stream packet of type WPS_SPT.
* @param packetPayload Payload of the stream packet.
* @param packetLength Length in bytes of the payload of the stream packet.
* @param packetLabel Substream indication of the stream packet.
* @param wps Output struct that will store the information about the WPS.
* @return MP4NoErr on success, a negative integer on error.
*/
MP4Err bgw_parse_waveform_parameter_set(u8* packetPayload, u32 packetLength, u64 packetLabel, bgw_waveform_parameter_set **wps);

/**
* @brief Parses a stream packet of type CGPS_SPT.
* @param packetPayload Payload of the stream packet.
* @param packetLength Length in bytes of the payload of the stream packet.
* @param cgps Output struct that will store the information about the CGPS.
* @return MP4NoErr on success, a negative integer on error.
*/
MP4Err bgw_parse_channel_group_parameter_set(u8* packetPayload, u32 packetLength, bgw_channel_group_parameter_set **cgps);

/**
* @brief Parses a stream packet of type CONFIG_SET_SPT.
* @param packetPayload Payload of the stream packet.
* @param packetLength Length in bytes of the payload of the stream packet.
* @param configurationSet Output struct that will store the information about the Configuration Set.
* @return MP4NoErr on success, a negative integer on error.
*/
MP4Err bgw_parse_configuration_set(u8* packetPayload, u32 packetLength, bgw_configuration_set **configurationSet);

/**
* @brief Parses a stream packet of type AM_SPT.
* @param packetPayload Payload of the stream packet.
* @param packetLength Length in bytes of the payload of the stream packet.
* @param configurationSet Output struct that will store the information about the Auxiliary Metadata.
* @return MP4NoErr on success, a negative integer on error.
*/
MP4Err bgw_parse_auxiliary_metadata(u8* packetPayload, u32 packetLength, bgw_auxiliary_metadata **auxiliaryMetadata);

/**
* @brief Parses a stream packet of type IF_SPT.
* @param packetPayload Payload of the stream packet.
* @param packetLength Length in bytes of the payload of the stream packet.
* @param configurationSet Output struct that will store the information about the independent frame.
* @return MP4NoErr on success, a negative integer on error.
*/
MP4Err bgw_parse_independent_frame(u8* packetPayload, u32 packetLength, bgw_independent_frame** independentFrame,
    bgw_stream* stream);

/**
* @brief Parses a stream packet of type DF_SPT.
* @param packetPayload Payload of the stream packet.
* @param packetLength Length in bytes of the payload of the stream packet.
* @param configurationSet Output struct that will store the information about the dependent frame.
* @param stream Struct containing the main information of the T.261 stream.
* @return MP4NoErr on success, a negative integer on error.
*/
MP4Err bgw_parse_dependent_frame(u8* packetPayload, u32 packetLength, bgw_dependent_frame **dependentFrame, bgw_stream* stream);

#endif