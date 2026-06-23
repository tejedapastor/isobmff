/***********************************************************************************
 
 This software module was originally developed by
 
 Fraunhofer HHI
 
 in the course of development of the T.261 for reference purposes and its
 performance may not have been optimized. This software module is an implementation
 of one or more tools as specified by the T.261 standard. ISO/IEC gives
 you a royalty-free, worldwide, non-exclusive, copyright license to copy, distribute,
 and make derivative works of this software module or modifications  thereof for use
 in implementations or products claiming conformance to the ISO/IEC T.261 standard
 and which satisfy any specified conformance criteria. Those intending to use this
 software module in products are advised that its use may infringe existing patents.
 ISO/IEC have no liability for use of this software module or modifications thereof.
 Copyright is not released for products that do not conform to the ISO/IEC 23003-4
 standard.
 
 Fraunhofer HHI retains full right to modify and use the code for its
 own purpose, assign or donate the code to a third party and to inhibit third parties
 from using the code for products that do not conform to MPEG-related ITU Recommenda-
 tions and/or ISO/IEC International Standards.
 
 This copyright notice must be included in all copies or derivative works.
 
 Copyright (c) ISO/IEC 2026.
 
 ***********************************************************************************/

#include "bgw.h"
#include "bgw_structures.h"

/* Escaped-value-coded numbers of bits*/
const int BGW_EV_LENGTH_BITS_3_8_8[] = {3, 8, 8};
const int BGW_EV_LENGTH_BITS_2_8_32[] = {2, 8, 32};
const int BGW_EV_LENGTH_BITS_11_24_24[] = {11, 24, 24};

/* Maximum stream packet header size */
const int BGW_MAX_PACKET_HEADER_SIZE = 15;

MP4Err bgw_parse_stream(FILE* input, bgw_stream **stream)
{
    MP4Err err = MP4NoErr;
    size_t startPos;
	BitBuffer bb;
    u32 numPackets = 0;
    u32 packetType, packetLength, packetLabel;
    u8* packetPayload;
    u32 bitsUsed = 0, bitsUsedHeader = 0, totalPacketBytes = 0, offsetBytes = 0;
    u8* bytes;

    bgw_stream* newStream;
    u32 numStreamPackets = 0;
    u32 numAllocatedStreamPackets = 0;

    newStream = calloc(1, sizeof(bgw_stream));
    if(!newStream) goto bail;

    newStream->stream_packets = calloc(8, sizeof(bgw_stream_packet));
    numAllocatedStreamPackets = 8;

    u32 numDifferentLabels = 0;
    u32* streamPacketLabels = NULL;
    u8 cgInfoPresent = 0;

    startPos = ftell(input);

    bytes = calloc(BGW_MAX_PACKET_HEADER_SIZE, 1);

    while(fread(bytes, BGW_MAX_PACKET_HEADER_SIZE, 1, input))
    {
        bitsUsedHeader = 0;
        totalPacketBytes = 0;

        err = BitBuffer_Init(&bb, bytes, BGW_MAX_PACKET_HEADER_SIZE); if (err) goto bail;

        // Packet type
        bitsUsed = read_escaped_value(
            &bb, 
            &packetType, 
            BGW_EV_LENGTH_BITS_3_8_8[0], 
            BGW_EV_LENGTH_BITS_3_8_8[1], 
            BGW_EV_LENGTH_BITS_3_8_8[2], 
            &err);
        bitsUsedHeader += bitsUsed;
        if(err) goto bail;

        *bytes = *bytes << bitsUsed;

        // Packet label
        bitsUsed = read_escaped_value(
            &bb, 
            &packetLabel, 
            BGW_EV_LENGTH_BITS_2_8_32[0], 
            BGW_EV_LENGTH_BITS_2_8_32[1],
            BGW_EV_LENGTH_BITS_2_8_32[2], 
            &err);
        bitsUsedHeader += bitsUsed;
        if(err) goto bail;

        *bytes = *bytes << bitsUsed;

        // Packet length
        bitsUsed = read_escaped_value(
            &bb,
            &packetLength, 
            BGW_EV_LENGTH_BITS_11_24_24[0],
            BGW_EV_LENGTH_BITS_11_24_24[1],
            BGW_EV_LENGTH_BITS_11_24_24[2],
            &err);
        bitsUsedHeader += bitsUsed;

        totalPacketBytes = (bitsUsedHeader + packetLength * 8)/8;

        packetPayload = calloc(1, packetLength);
        if(!packetPayload) goto bail;

        fseek(input, offsetBytes + bitsUsedHeader/8, SEEK_SET);
        fread(packetPayload, packetLength, 1, input);


        /* // Debug information 
        printf("New packet:\n");
        printf("- Type: %d\n", packetType);
        printf("- Label: %d\n", packetLabel);
        printf("- Payload length: %d\n", packetLength);
        printf("- Header length bits: %d\n", bitsUsedHeader);
        printf("- Package length bytes: %d\n", totalPacketBytes);*/

        offsetBytes += totalPacketBytes;
        fseek(input, offsetBytes, SEEK_SET);

        numPackets++;

        err = bgw_check_new_label(&streamPacketLabels, &numDifferentLabels, packetLabel);

        err = bgw_handle_packet_data(packetType, packetLength, packetPayload, packetLabel, &newStream);

        // Reallocate packet data in stream
        if(numStreamPackets == numAllocatedStreamPackets)
        {
            newStream->stream_packets = realloc(newStream->stream_packets, 2 * numAllocatedStreamPackets * sizeof(bgw_stream_packet));
            numAllocatedStreamPackets *= 2;
        }

        // Insert packet data in stream
        bgw_stream_packet* streamPacket = calloc(1, sizeof(bgw_stream_packet));
        streamPacket->num_bytes_in_syntax_structure = packetLength;
        streamPacket->syntax_structure_bytes = packetPayload;

        bgw_stream_packet_header* streamPacketHeader = calloc(1, sizeof(bgw_stream_packet_header));
        streamPacketHeader->stream_packet_type = packetType;
        streamPacketHeader->stream_packet_label = packetLabel;
        streamPacketHeader->stream_packet_length = packetLength;

        streamPacket->stream_packet_header = streamPacketHeader;

        streamPacket->total_length = totalPacketBytes;
        streamPacket->stream_packet_header_size = bitsUsedHeader / 8;

        newStream->stream_packets[numStreamPackets] = streamPacket;
        numStreamPackets++;
    }

    printf("Number of packets found in the bitstream: %d\n", numPackets);

    free(bytes);
    fclose(input);

    newStream->numStreamPackets = numStreamPackets;
    newStream->numAllocatedStreamPackets = numAllocatedStreamPackets;
    newStream->numSubstreams = numDifferentLabels;
    newStream->streamPacketLabels = streamPacketLabels;

    *stream = newStream;

    if(err) goto bail;

    bail:
        return MP4NoErr;
}

MP4Err bgw_handle_packet_data(u32 packetType, u32 packetLength, u8* packetPayload, u64 packetLabel, bgw_stream** stream)
{
    MP4Err err = MP4NoErr;
    int i;

    switch(packetType)
    {
        case BGW_FORBIDDEN_SPT:
            break;
        case BGW_WPS_SPT:
            {
                bgw_waveform_parameter_set* wps;
                err = bgw_parse_waveform_parameter_set(packetPayload, packetLength, packetLabel, &wps);
                if(err) goto bail;

                bgw_wps_packet_info* wpsPacketInfo;
                wpsPacketInfo = calloc(1, sizeof(bgw_wps_packet_info));
                if(!wpsPacketInfo)
                {
                    err = MP4BadDataErr;
                    goto bail;
                }

                wpsPacketInfo->packet_type = packetType;
                wpsPacketInfo->packet_length = packetLength;
                wpsPacketInfo->packet_data = packetPayload;
                wpsPacketInfo->wps = wps;

                if(!(*stream)->waveformParameterSetList)
                {
                    (*stream)->waveformParameterSetList = calloc(1, sizeof(bgw_wps_packet_info*));
                    if(!(*stream)->waveformParameterSetList)
                    {
                        err = MP4BadDataErr;
                        goto bail;
                    }
                    (*stream)->numWPS = 1;
                } else
                {
                    bgw_wps_packet_info** tmpWPSList = 
                                realloc((*stream)->waveformParameterSetList, ((*stream)->numWPS + 1) * sizeof(bgw_wps_packet_info*));
                    if(!tmpWPSList)
                    {
                        err = MP4BadDataErr;
                        goto bail;
                    }
                    (*stream)->waveformParameterSetList = tmpWPSList;
                }

                (*stream)->waveformParameterSetList[(*stream)->numWPS - 1] = wpsPacketInfo;

                break;
            }
        case BGW_CGPS_SPT:
            {
                bgw_channel_group_parameter_set* cgps;
                err = bgw_parse_channel_group_parameter_set(packetPayload, packetLength, &cgps);
                if(err) goto bail;

                bgw_cgps_packet_info* cgpsPacketInfo;
                cgpsPacketInfo = calloc(1, sizeof(bgw_cgps_packet_info));
                if(!cgpsPacketInfo)
                {
                    err = MP4BadDataErr;
                    goto bail;
                }

                cgpsPacketInfo->packet_type = packetType;
                cgpsPacketInfo->packet_length = packetLength;
                cgpsPacketInfo->packet_data = packetPayload;
                cgpsPacketInfo->cgps = cgps;

                if(!(*stream)->channelGroupParameterSetList)
                {
                    (*stream)->channelGroupParameterSetList = calloc(1, sizeof(bgw_cgps_packet_info*));
                    if(!(*stream)->channelGroupParameterSetList)
                    {
                        err = MP4BadDataErr;
                        goto bail;
                    }
                    (*stream)->numCGPS = 1;
                } else
                {
                    bgw_cgps_packet_info** tmpCGPSList = 
                                realloc((*stream)->channelGroupParameterSetList, ((*stream)->numCGPS + 1) * sizeof(bgw_cgps_packet_info*));
                    if(!tmpCGPSList)
                    {
                        err = MP4BadDataErr;
                        goto bail;
                    }

                    (*stream)->channelGroupParameterSetList = tmpCGPSList;
                }

                (*stream)->channelGroupParameterSetList[(*stream)->numCGPS - 1] = cgpsPacketInfo;

                break;
            }
        case BGW_TIMESTAMP_SPT:
            break;
        case BGW_IF_SPT:
            {
                bgw_independent_frame* independentFrame;
                err = bgw_parse_independent_frame(packetPayload, packetLength, &independentFrame, *stream); if(err) BAILWITHERROR(err);
                err = bgw_create_new_frame_sequence(independentFrame, stream); if(err) BAILWITHERROR(err);

                if(err) goto bail;
                break;
            }
                
        case BGW_DF_SPT:
            {
                bgw_dependent_frame* dependentFrame;
                err = bgw_parse_dependent_frame(packetPayload, packetLength, &dependentFrame, *stream); if(err) BAILWITHERROR(err);
                err = bgw_insert_df_in_frame_sequence(dependentFrame, stream); if(err) BAILWITHERROR(err);

                if(err) goto bail;
                break;
            }
        case BGW_SEGMENT_SPT:
        case BGW_AC_SPT:
        case BGW_FEATURE_SPT:
        case BGW_SYNC_SPT:
        case BGW_LOCAL_CRC16_SPT:
        case BGW_LOCAL_CRC32_SPT:
        case BGW_UUID_U_SPT:
        case BGW_UUID_S_SPT:
        case BGW_GLOBAL_CRC16_SPT:
        case BGW_GLOBAL_CRC32_SPT:
        case BGW_AUTH_START_SPT:
        case BGW_AUTH_SIG_SPT:
            break;
        case BGW_AM_SPT:
            {
                bgw_auxiliary_metadata* auxiliaryMetadata;
                err = bgw_parse_auxiliary_metadata(packetPayload, packetLength, &auxiliaryMetadata);
                if(err) goto bail;
                (*stream)->auxiliaryMetadata = auxiliaryMetadata;
                break;
            }
        case BGW_CONFIG_SET_SPT:
            {
                bgw_configuration_set* cs;
                err = bgw_parse_configuration_set(packetPayload, packetLength, &cs);
                if(err) goto bail;

                bgw_cs_packet_info* csPacketInfo;
                csPacketInfo = calloc(1, sizeof(bgw_cs_packet_info));
                if(!csPacketInfo)
                {
                    err = MP4BadDataErr;
                    goto bail;
                }

                csPacketInfo->packet_type = packetType;
                csPacketInfo->packet_length = packetLength;
                csPacketInfo->packet_data = packetPayload;
                csPacketInfo->cs = cs;

                if(!(*stream)->configurationSetList)
                {
                    (*stream)->configurationSetList = calloc(1, sizeof(bgw_configuration_set*));
                    if(!(*stream)->configurationSetList)
                    {
                        err = MP4BadDataErr;
                        goto bail;
                    }
                    (*stream)->numCS = 1;
                } else
                {
                    bgw_cs_packet_info** tmpCSList = 
                                realloc((*stream)->configurationSetList, ((*stream)->numCS + 1) * sizeof(bgw_configuration_set*));
                    if(!tmpCSList)
                    {
                        err = MP4BadDataErr;
                        goto bail;
                    }

                    (*stream)->configurationSetList = tmpCSList;
                }

                (*stream)->configurationSetList[(*stream)->numCS - 1] = csPacketInfo;

                break;
            }
        default:
        {}
    }
    bail:
        return err;
}

MP4Err bgw_create_new_frame_sequence(bgw_independent_frame* independentFrame, bgw_stream** stream)
{
    MP4Err err;

    // Create frame sequence
    if(!(*stream)->frameSequences)
    {
        (*stream)->frameSequences = malloc(sizeof(bgw_frame_sequence*));
        if(!(*stream)->frameSequences) BAILWITHERROR(MP4NoMemoryErr);
    } else
    {
        (*stream)->frameSequences = realloc(
            (*stream)->frameSequences, 
            ((*stream)->numFrameSequences + 1) * sizeof(bgw_frame_sequence*));
    }
    if(!(*stream)->frameSequences) BAILWITHERROR(MP4NoMemoryErr);

    bgw_frame_sequence* frameSequence;
    frameSequence = calloc(1, sizeof(bgw_frame_sequence));

    frameSequence->channelGroupId = independentFrame->if_channel_group_id;
    frameSequence->independentFrame = independentFrame;
    frameSequence->numDependentFrames = 0;
    
    (*stream)->frameSequences[(*stream)->numFrameSequences] = frameSequence;
    (*stream)->numFrameSequences++;

bail:
    TEST_RETURN(err);
    return err;
}

MP4Err bgw_insert_df_in_frame_sequence(bgw_dependent_frame* dependentFrame, bgw_stream** stream)
{
    MP4Err err;

    u32 numFrameSequences = (*stream)->numFrameSequences;
    if(!numFrameSequences) BAILWITHERROR(MP4BadDataErr);

    u32 numDependentFrames;

    if(!(*stream)->frameSequences[numFrameSequences - 1]) BAILWITHERROR(MP4BadDataErr);
    if(!(*stream)->frameSequences[numFrameSequences - 1]->numDependentFrames)
    {
        (*stream)->frameSequences[numFrameSequences - 1]->dependentFrames = calloc(1, sizeof(bgw_dependent_frame*));
        numDependentFrames = 0;
    } else
    {
        numDependentFrames = (*stream)->frameSequences[numFrameSequences - 1]->numDependentFrames;
        (*stream)->frameSequences[numFrameSequences - 1]->dependentFrames 
            = realloc((*stream)->frameSequences[numFrameSequences - 1]->dependentFrames, 
                (numDependentFrames + 1) * sizeof(bgw_dependent_frame*));
    }

    if(!(*stream)->frameSequences[numFrameSequences - 1]->dependentFrames) BAILWITHERROR(MP4NoMemoryErr);

    (*stream)->frameSequences[numFrameSequences - 1]->dependentFrames[numDependentFrames] = dependentFrame;
    (*stream)->frameSequences[numFrameSequences - 1]->numDependentFrames++;

bail:
    TEST_RETURN(err);
    return err;
}


MP4Err bgw_check_new_label(u32** streamPacketLabels, u32* numDifferentLabels, u32 newLabel)
{
    MP4Err err;
    err = MP4NoErr;

    if(newLabel == 0)
    {
        goto bail;
    }

    u8 labelFound = 0;
    for(int i = 0; i < *numDifferentLabels; i++)
    {
        if(newLabel == (*streamPacketLabels)[i])
        {
            labelFound = 1;
            break;
        }
    }

    if(!labelFound)
    {
        (*numDifferentLabels)++;
        *streamPacketLabels = realloc(*streamPacketLabels, sizeof(u32) * (*numDifferentLabels));
        if(!streamPacketLabels) goto bail;
        (*streamPacketLabels)[*numDifferentLabels - 1] = newLabel;
    }
bail:
    TEST_RETURN(err);
    return MP4NoErr;
}

MP4Err bgw_parse_waveform_parameter_set(u8* packetPayload, u32 packetLength, u64 packetLabel, bgw_waveform_parameter_set **wps)
{
    MP4Err err = MP4NoErr;
    u8 tmp8;
    u16 tmp16;
    u32 tmp32;
    u32 i, j;
    u32* annotationChannelNumSamplesArray = NULL;
    bgw_wps_channel_swaps_info* channelSwapsInfoList;
    u32* annotationChannelNumSamplesList;
    
    bgw_waveform_parameter_set* newWPS;
    newWPS = calloc(1, sizeof(bgw_waveform_parameter_set));
    if(!newWPS) goto bail;

    BitBuffer bb;
    err = BitBuffer_Init(&bb, packetPayload, packetLength); if (err) goto bail;

    tmp32 = GetBits(&bb, 4, &err);
    if (err) goto bail;

    newWPS->wps_waveform_parameter_set_id = (u8)tmp32;
    newWPS->numChannelGroups = 0;
    newWPS->totalNumChannels = 0;
    newWPS->substreamId = packetLabel;

    bgw_wps_channel_group_info* channelGroupInfo;
    newWPS->channelGroupsInfo = calloc(1, sizeof(bgw_wps_channel_group_info*));

    do
    {
        u8* channelGroupRerefEnableList;
        u8* channelGroupRerefModeList;
        u32* channelGroupRerefChannelIdxList;
        u32 numChannels;
        u32* channelGroupStartingPosList;

        channelGroupInfo = calloc(1, sizeof(bgw_wps_channel_group_info));
        if(!channelGroupInfo)
        {
            err = MP4BadDataErr;
            goto bail;
        }

        // Num channels in next group minus1
        tmp32 = read_golomb_uev(&bb, &err);
        if(err) goto bail;
        channelGroupInfo->wps_num_channels_in_next_group_minus1 = tmp32;

        // Num channel group repetitions
        tmp32 = read_golomb_uev(&bb, &err);
        if(err) goto bail;
        channelGroupInfo->wps_num_channel_group_repetitions = tmp32;

        channelGroupRerefEnableList = calloc(channelGroupInfo->wps_num_channel_group_repetitions, sizeof(u8));
        channelGroupRerefModeList = calloc(channelGroupInfo->wps_num_channel_group_repetitions, sizeof(u8));
        channelGroupRerefChannelIdxList = calloc(channelGroupInfo->wps_num_channel_group_repetitions, sizeof(u32));
        channelGroupStartingPosList = calloc(channelGroupInfo->wps_num_channel_group_repetitions, sizeof(u32));

        if(!channelGroupRerefEnableList || !channelGroupRerefModeList || !channelGroupRerefChannelIdxList
             || !channelGroupStartingPosList)
        {
            err = MP4BadDataErr;
            goto bail;
        }

        for(j = 0; j <= channelGroupInfo->wps_num_channel_group_repetitions; j++)
        {
            numChannels = channelGroupInfo->wps_num_channels_in_next_group_minus1 + 1;

            // Channel group reref enable
            tmp32 = GetBits(&bb, 1, &err);
            if (err) goto bail;
            channelGroupRerefEnableList[j] = (u8)tmp32;

            if(channelGroupRerefEnableList[j])
            {
                // Channel group reref mode
                tmp32 = GetBits(&bb, 2, &err);
                if (err) goto bail;
                channelGroupRerefModeList[newWPS->numChannelGroups] = (u8)tmp32;

                // Channel group reref channel idx
                tmp32 = read_golomb_uev(&bb, &err);
                if (err) goto bail;
                channelGroupRerefChannelIdxList[newWPS->numChannelGroups] = tmp32;
            }

            channelGroupStartingPosList[newWPS->numChannelGroups++] = newWPS->totalNumChannels;
            newWPS->totalNumChannels += channelGroupInfo->wps_num_channels_in_next_group_minus1 + 1;
        }

        channelGroupInfo->channel_group_reref_enable = channelGroupRerefEnableList;
        channelGroupInfo->channel_group_reref_mode = channelGroupRerefModeList;
        channelGroupInfo->channel_group_reref_channel_idx = channelGroupRerefChannelIdxList;
        channelGroupInfo->numChannels = numChannels;
        channelGroupInfo->channelGroupStartingPos = channelGroupStartingPosList;

        // More channel groups present flag
        tmp32 = GetBits(&bb, 1, &err);
        if (err) goto bail;
        channelGroupInfo->wps_more_channel_groups_present_flag = (u8)tmp32;

        newWPS->channelGroupsInfo[newWPS->numChannelGroups - 1] = channelGroupInfo;
        if(channelGroupInfo->wps_more_channel_groups_present_flag)
        {
            newWPS->channelGroupsInfo = realloc(newWPS->channelGroupsInfo, (newWPS->numChannelGroups + 1) * sizeof(bgw_wps_channel_group_info*));
        }
        
    } while (channelGroupInfo->wps_more_channel_groups_present_flag);
    
    // Channel reordering flag
    tmp32 = GetBits(&bb, 1, &err);
    if (err) goto bail;
    newWPS->wps_channel_reordering_flag = (u8)tmp32;

    if(newWPS->wps_channel_reordering_flag)
    {
        // Num channel swaps minus1
        tmp32 = read_golomb_uev(&bb, &err);
        if (err) goto bail;
        newWPS->wps_num_channel_swaps_minus1 = tmp32;

        channelSwapsInfoList = calloc(newWPS->wps_num_channel_swaps_minus1, sizeof(bgw_wps_channel_swaps_info));
        if(!channelSwapsInfoList)
        {
            err = MP4BadDataErr;
            goto bail;
        }

        for(i = 0; i <= newWPS->wps_num_channel_swaps_minus1; i++)
        {
            // Swap frst idx
            tmp32 = read_golomb_uev(&bb, &err);
            if (err) goto bail;
            channelSwapsInfoList[i].wps_swap_frst_idx = tmp32;

            // Swap scnd idx min frst idx min1
            tmp32 = read_golomb_uev(&bb, &err);
            if (err) goto bail;
            channelSwapsInfoList[i].wps_swap_scnd_idx_min_frst_idx_min1 = tmp32;
        }

        newWPS->channelSwapsInfo = channelSwapsInfoList;
    }

    tmp32 = read_golomb_uev(&bb, &err);
    if (err) goto bail;
    newWPS->wps_num_annotation_channels = tmp32;

    annotationChannelNumSamplesList = calloc(newWPS->wps_num_annotation_channels, sizeof(u32));
    if(!annotationChannelNumSamplesList) goto bail;

    for(j = 0; j < newWPS->wps_num_annotation_channels; j++)
    {
        annotationChannelNumSamplesList[j] = 0;
    }

    newWPS->annotationChannelNumSamples = annotationChannelNumSamplesList;

    *wps = newWPS;

    return err;
bail:
    TEST_RETURN(err);
    return err;
}

MP4Err bgw_parse_channel_group_parameter_set(u8* packetPayload, u32 packetLength, bgw_channel_group_parameter_set **cgps)
{
    MP4Err err = MP4NoErr;
    u8 tmp8;
    u16 tmp16;
    u32 tmp32;
    u32 i, j;

    bgw_channel_group_parameter_set* newCGPS;
    newCGPS = calloc(1, sizeof(bgw_channel_group_parameter_set));
    if(!newCGPS) goto bail;

    BitBuffer bb;
    err = BitBuffer_Init(&bb, packetPayload, packetLength); if (err) goto bail;

    // Channel group parameter set ID
    err = GetBytes(&bb, 1, &tmp8);
    if (err) goto bail;
    newCGPS->cgps_channel_group_parameter_set_id = tmp8;
    
    // Waveform parameter set ID
    tmp32 = GetBits(&bb, 4, &err);
    if (err) goto bail;
    newCGPS->cgps_waveform_parameter_set_id = (u8)tmp32;

    err = GetBytes(&bb, 2, (u8*)&tmp16);
    if (err) goto bail;

    // Length signal mode flag
    newCGPS->cgps_length_signal_mode_flag = tmp16 >> 15;
    tmp16 = tmp16 & 0x7FFF;
    // Frame length shift
    newCGPS->cgps_frame_length_shift = tmp16 >> 11;
    tmp16 = tmp16 & 0x07FF;
    // Max min block size
    newCGPS->cgps_max_min_block_size = tmp16 >> 5;
    tmp16 = tmp16 & 0x007F;
    // Deblocking_mode
    newCGPS->cgps_deblocking_mode = tmp16 >> 3;
    tmp16 = tmp16 & 0x0003;
    // Max bit depth
    newCGPS->cgps_max_bit_depth = tmp16;

    // Allow cross channel pred flag
    tmp32 = GetBits(&bb, 1, &err);
    if (err) goto bail;
    newCGPS->cgps_allow_cross_channel_pred_flag = (u8)tmp32;

    if(newCGPS->cgps_allow_cross_channel_pred_flag)
    {
        // CC pred filtering mode
        tmp32 = GetBits(&bb, 2, &err);
        if (err) goto bail;
        newCGPS->cgps_cc_pred_filtering_mode = (u8)tmp32;

        // Allow cc pred mult hyp flag
        tmp32 = GetBits(&bb, 1, &err);
        if (err) goto bail;
        newCGPS->cgps_allow_cc_pred_mult_hyp_flag = (u8)tmp32;
    }

    // Allow block matching pred flag
    tmp32 = GetBits(&bb, 1, &err);
    if (err) goto bail;
    newCGPS->cgps_allow_block_matching_pred_flag = (u8)tmp32;

    if(newCGPS->cgps_allow_block_matching_pred_flag)
    {
        // BM pred filtering mode
        tmp8 = GetBits(&bb, 2, &err);
        if (err) goto bail;
        newCGPS->cgps_bm_pred_filtering_mode = tmp8;
        
        // Allow BM pred mult hyp flag
        tmp8 = GetBits(&bb, 1, &err);
        if (err) goto bail;
        newCGPS->cgps_allow_bm_pred_mult_hyp_flag = tmp8;

        // Allow BM offset pred prev ch flag
        tmp8 = GetBits(&bb, 1, &err);
        if (err) goto bail;
        newCGPS->cgps_allow_bm_offset_pred_prev_ch_flag = tmp8;

        tmp8 = GetBits(&bb, 4, &err);
        if (err) goto bail;

        // Allow DC pred flag
        newCGPS->cgps_allow_dc_pred_flag = tmp8 >> 3;
        tmp8 = tmp8 & 0x07;
        // Allow line fit flag
        newCGPS->cgps_allow_dc_pred_flag = tmp8 >> 2;
        tmp8 = tmp8 & 0x03;
        // Allow lpf flag
        newCGPS->cgps_allow_lpf_flag = tmp8 >> 1;
        tmp8 = tmp8 & 0x01;
        // Allow sample pred fixed weights flag
        newCGPS->cgps_allow_sample_pred_fixed_weights_flag = tmp8;

        if(newCGPS->cgps_allow_lpf_flag)
        {
            tmp8 = GetBits(&bb, 1, &err);
            if (err) goto bail;
            newCGPS->cgps_lpf_allow_prev_ch_flag = tmp8;
        }

        tmp32 = GetBits(&bb, 20, &err);
        if (err) goto bail;

        // Allow transform skip flag
        newCGPS->cgps_allow_transform_skip_flag = tmp32 >> 19;
        tmp16 = tmp16 & 0x0007FFFF;
        // Residual quant mode
        newCGPS->cgps_residual_quant_mode = tmp32 >> 17;
        tmp16 = tmp16 & 0x0001FFFF;
        // Ch indep interval idx
        newCGPS->cgps_ch_indep_interval_idx = tmp32 >> 13;
        tmp16 = tmp16 & 0x00001FFF;
        // Max abs delta qp idx
        newCGPS->cgps_max_abs_delta_qp_idx = tmp32 >> 10;
        tmp16 = tmp16 & 0x000003FF;
        // qp
        newCGPS->cgps_qp = tmp32;

        if(newCGPS->cgps_allow_transform_skip_flag)
        {
            // Allow verbatim coding flag
            tmp8 = GetBits(&bb, 1, &err);
            if (err) goto bail;
            newCGPS->cgps_allow_verbatim_coding_flag = tmp8;
        }

        tmp8 = GetBits(&bb, 2, &err);
        if (err) goto bail;

        // Allow zero lsb flag
        newCGPS->cgps_allow_zero_lsb_flag = tmp8 >> 1;
        tmp8 = tmp8 & 0x01;
        // Allow lms flag
        newCGPS->cgps_allow_lms_flag = tmp8;

        if(newCGPS->cgps_allow_lms_flag)
        {
            tmp8 = GetBits(&bb, 6, &err);
            if (err) goto bail;

            // Allow lms split flag
            newCGPS->cgps_allow_lms_split_flag = tmp8 >> 5;
            tmp8 = tmp8 & 0x1F;
            // Lms ar order over four
            newCGPS->cgps_lms_ar_order_over_four = tmp8 >> 1;
            tmp8 = tmp8 & 0x01;
            // Allow cc lms flag
            newCGPS->cgps_allow_cc_lms_flag = tmp8;

            if (newCGPS->cgps_allow_cc_lms_flag)
            {
                // Max order cc lms minus1
                tmp8 = GetBits(&bb, 6, &err);
                if (err) goto bail;
                newCGPS->cgps_max_order_cc_lms_minus1 = tmp8;
            }
        }

        // Ctx init flag
        tmp8 = GetBits(&bb, 1, &err);
        if (err) goto bail;
        newCGPS->cgps_ctx_init_flag = tmp8;
    }

    *cgps = newCGPS;

    return err;

bail:
    return err;
}


MP4Err bgw_parse_configuration_set(u8* packetPayload, u32 packetLength, bgw_configuration_set **configurationSet)
{
    MP4Err err = MP4NoErr;
    u32 size;
    u8 tmp8;
    u16 tmp16;
    u32 tmp32;
    s32 tmps32;
    u32 i, sf, cg, st, ac;
    u32 bitsUsed = 0;
    
    bgw_configuration_set* newConfigurationSet;

    BitBuffer bb;
    err = BitBuffer_Init(&bb, packetPayload, packetLength); 
    if (err) BAILWITHERROR(err);

    newConfigurationSet = calloc(1, sizeof(bgw_configuration_set));

    tmp32 = GetBits(&bb, 4, &err);
    if (err) BAILWITHERROR(err);

    newConfigurationSet->cs_num_wps_ids = tmp32;

    newConfigurationSet->cs_wps = 
            calloc(newConfigurationSet->cs_num_wps_ids, sizeof(bgw_configuration_set_wps*));
    if(!newConfigurationSet->cs_wps) BAILWITHERROR(MP4BadDataErr);

    for (i = 0; i < newConfigurationSet->cs_num_wps_ids; i++)
    {
        bgw_configuration_set_wps* wps = 
            calloc(1, sizeof(bgw_configuration_set_wps));
        if(!wps) BAILWITHERROR(MP4BadDataErr);

        // WPS ID
        tmp32 = GetBits(&bb, 4, &err);
        if (err) BAILWITHERROR(err);
        wps->cs_wps_id = (u8)tmp32;

        // Label mapping flag
        tmp32 = GetBits(&bb, 1, &err);
        if (err) BAILWITHERROR(err);
        wps->cs_label_mapping_flag = (u8)tmp32;

        if(wps->cs_label_mapping_flag)
        {
            // Initial WPS ID label
            read_escaped_value(
                &bb, 
                &tmp32, 
                BGW_EV_LENGTH_BITS_2_8_32[0], 
                BGW_EV_LENGTH_BITS_2_8_32[1],
                BGW_EV_LENGTH_BITS_2_8_32[2], 
                &err);
            if(err) BAILWITHERROR(err);
            wps->cs_initial_wps_id_label = tmp32;

            if(newConfigurationSet->cs_num_wps_ids > 1)
            {
                // Target zero packet label
                read_escaped_value(
                    &bb, 
                    &tmp32, 
                    BGW_EV_LENGTH_BITS_2_8_32[0], 
                    BGW_EV_LENGTH_BITS_2_8_32[1],
                    BGW_EV_LENGTH_BITS_2_8_32[2], 
                    &err);
                if(err) BAILWITHERROR(err);
                wps->cs_target_zero_packet_label = tmp32;
            }
        }

        // Num channel groups in WPS
        tmp32 = read_golomb_uev(&bb, &err);
        if(err) BAILWITHERROR(err);
        wps->cs_num_channel_groups_in_wps = tmp32;

        // Num sampling freqs in WPS
        tmp32 = GetBits(&bb, 4, &err);
        if (err) BAILWITHERROR(err);
        wps->cs_num_sampling_freqs_in_wps = (u8)tmp32;

        wps->cs_sampling_freq = calloc(wps->cs_num_sampling_freqs_in_wps, sizeof(u32));
        for (sf = 0; sf < wps->cs_num_sampling_freqs_in_wps; sf++)
        {
            // Sampling freq
            tmp32 = read_golomb_uev(&bb, &err);
            if(err) BAILWITHERROR(err);
            wps->cs_sampling_freq[sf] = tmp32;
        }

        if(wps->cs_num_sampling_freqs_in_wps > 1)
        {
            wps->cs_sampling_freq_idx = calloc(wps->cs_num_channel_groups_in_wps, sizeof(u32));
            for(cg = 0; cg < wps->cs_num_channel_groups_in_wps; cg++)
            {
                //Sampling freq idx
                tmp32 = GetBits(&bb, ceil_log2(wps->cs_num_sampling_freqs_in_wps), &err);
                if(err) BAILWITHERROR(err);
                wps->cs_sampling_freq_idx[cg] = tmp32;
            }
        }

        // Num signal types in WPS
        tmp32 = GetBits(&bb, 4, &err);
        if (err) BAILWITHERROR(err);
        wps->cs_num_signal_types_in_wps = (u8)tmp32;
        wps->cs_signal_info = calloc(wps->cs_num_signal_types_in_wps, sizeof(bgw_configuration_set_wps_signal));
        for(st = 0; st < wps->cs_num_signal_types_in_wps; st++)
        {
            bgw_configuration_set_wps_signal *signalInfo = &(wps->cs_signal_info)[st];
         
            // Signal type
            read_escaped_value(
                &bb, 
                &tmp32, 
                BGW_EV_LENGTH_BITS_3_8_8[0], 
                BGW_EV_LENGTH_BITS_3_8_8[1],
                BGW_EV_LENGTH_BITS_3_8_8[2], 
                &err);
            if(err) BAILWITHERROR(err);
            signalInfo->cs_signal_type = tmp32;

            // Signal num annotation channels
            tmp32 = read_golomb_uev(&bb, &err);
            if(err) BAILWITHERROR(err);
            signalInfo->cs_signal_num_annotation_channels = tmp32;

            if (signalInfo->cs_signal_num_annotation_channels)
            {
                signalInfo->cs_signal_annotation_channel_id = calloc(signalInfo->cs_signal_num_annotation_channels, sizeof(u32));

                for(ac = 0; ac < signalInfo->cs_signal_num_annotation_channels; ac++)
                {
                    // Signal annotation channel ID
                    tmp32 = read_golomb_uev(&bb, &err);
                    if(err) BAILWITHERROR(err);
                    signalInfo->cs_signal_annotation_channel_id[ac] = tmp32;
                }
            }     
        }

        if(wps->cs_num_signal_types_in_wps > 1)
        {
            wps->cs_signal_type_idx = calloc(wps->cs_num_channel_groups_in_wps, sizeof(u32));

            for(cg = 0; cg < wps->cs_num_channel_groups_in_wps; cg++)
            {
                // Signal type idx
                tmp32 = GetBits(&bb, ceil_log2(wps->cs_num_signal_types_in_wps), &err);
                if(err) BAILWITHERROR(err);
                wps->cs_signal_type_idx[cg] = tmp32;
            }
        }

        // Signal info data flag in WPS flag
        tmp32 = GetBits(&bb, 1, &err);
        if(err) BAILWITHERROR(err);
        wps->cs_signal_info_data_flag_in_wps_flag = (u8)tmp32;

        if(wps->cs_signal_info_data_flag_in_wps_flag && wps->cs_num_channel_groups_in_wps)
        {
            wps->cs_signal_info_data = calloc(wps->cs_num_channel_groups_in_wps, sizeof(bgw_configuration_set_signal_info_data));
            for(cg = 0; cg < wps->cs_num_channel_groups_in_wps; cg++)
            {
                bgw_configuration_set_signal_info_data *signalInfoData = &(wps->cs_signal_info_data)[cg];

                tmp32 = GetBits(&bb, 1, &err);
                if(err) BAILWITHERROR(err);
                signalInfoData->cs_has_range_info_flag = (u8)tmp32;

                if(signalInfoData->cs_has_range_info_flag)
                {
                    // Digital min
                    tmps32 = read_golomb_sev(&bb, &err);
                    if(err) BAILWITHERROR(err);
                    signalInfoData->cs_digital_min = tmps32;

                    // Digital max
                    tmps32 = read_golomb_sev(&bb, &err);
                    if(err) BAILWITHERROR(err);
                    signalInfoData->cs_digital_max = tmps32;

                    // Analogue min
                    tmps32 = read_golomb_sev(&bb, &err);
                    if(err) BAILWITHERROR(err);
                    signalInfoData->cs_analogue_min = tmps32;

                    // Analogue max
                    tmps32 = read_golomb_sev(&bb, &err);
                    if(err) BAILWITHERROR(err);
                    signalInfoData->cs_analogue_max = tmps32;

                    // Analogue units
                    tmps32 = read_golomb_sev(&bb, &err);
                    if(err) BAILWITHERROR(err);
                    signalInfoData->cs_analogue_units = tmps32;
                }

                // Recording start time flag
                tmp32 = GetBits(&bb, 1, &err);
                if(err) BAILWITHERROR(err);
                signalInfoData->cs_recording_start_time_flag = (u8)tmp32;
            }
        }

        newConfigurationSet->cs_wps[i] = wps;
    }

    // Config extension flag
    tmp32 = GetBits(&bb, 1, &err);
    if(err) BAILWITHERROR(err);
    newConfigurationSet->cs_config_extension_flag = (u8)tmp32;

    if(newConfigurationSet->cs_config_extension_flag)
    {
        //TODO CONFIG EXTENSION DATA
    }

    *configurationSet = newConfigurationSet;


bail:
    return err;

}

MP4Err bgw_parse_auxiliary_metadata(u8* packetPayload, u32 packetLength, bgw_auxiliary_metadata **auxiliaryMetadata)
{
    MP4Err err = MP4NoErr;
    u32 size;
    u8 tmp8;
    u16 tmp16;
    u32 tmp32;
    u32 i;
    u32 j;
    u32 k;

    BitBuffer bb;
    err = BitBuffer_Init(&bb, packetPayload, packetLength); if (err) goto bail;

    bgw_auxiliary_metadata* newAuxiliaryMetadata;

    newAuxiliaryMetadata = calloc(1, sizeof(bgw_auxiliary_metadata));
    
    // Header CRC32
    err = GetBytes(&bb, 4, (u8*)&tmp32);
    if(err) goto bail;
    newAuxiliaryMetadata->am_header_crc32 = tmp32;
    
    // Extension present flag
    err = GetBytes(&bb, 1, (u8*)&tmp8);
    if(err) goto bail;
    newAuxiliaryMetadata->am_extension_present_flag = tmp8 >> 7;
    // Waveform type
    tmp8 = tmp8 & 0x7F;
    newAuxiliaryMetadata->am_waveform_type = tmp8 >> 5;
    // Length signal mode
    tmp8 = tmp8 & 0x1F;
    newAuxiliaryMetadata->am_length_signal_mode = tmp8 >> 4;
    // Emphasis flag
    tmp8 = tmp8 & 0x0F;
    newAuxiliaryMetadata->am_emphasis_flag = tmp8 >> 3;
    // Copyright flag
    tmp8 = tmp8 & 0x07;
    newAuxiliaryMetadata->am_copyright_flag = tmp8 >> 2;
    // Original flag
    tmp8 = tmp8 & 0x03;
    newAuxiliaryMetadata->am_original_flag = tmp8 >> 1;
    // Private flag
    tmp8 = tmp8 & 0x01;
    newAuxiliaryMetadata->am_private_flag = tmp8;

    // Stream max sampling rate minus 1
    err = GetBytes(&bb, 4, (u8*)&tmp32);
    if(err) goto bail;
    tmp32 = tmp32 >> 8;
    newAuxiliaryMetadata->am_stream_max_sampling_rate_minus1 = tmp32;

    // Stream max num channels minus 1
    err = GetBytes(&bb, 2, (u8*)&tmp16);
    if(err) goto bail;
    newAuxiliaryMetadata->am_stream_max_num_channels_minus1 = tmp16;

    if(newAuxiliaryMetadata->am_length_signal_mode)
    {
        // Stream num samples per channel
        err = GetBytes(&bb, 4, (u8*)&tmp32);
        if(err) goto bail;
        newAuxiliaryMetadata->am_stream_num_samples_per_ch = tmp32;
    }

    if(newAuxiliaryMetadata->am_waveform_type == BGW_WAVEFORM_TYPE_WT_BS2088)
    {
        // Metadata reserved flag
        err = GetBytes(&bb, 4, (u8*)&tmp32);
        newAuxiliaryMetadata->am_metadata_reserved_flag = tmp32 >> 31;
        // Metadata num bytes minus 1
        tmp32 = tmp32 & 0x7FFFFFFF;
        newAuxiliaryMetadata->am_metadata_num_bytes_minus1 = tmp32;

        err = GetBytes(&bb, newAuxiliaryMetadata->am_metadata_num_bytes_minus1 + 1, newAuxiliaryMetadata->am_metadata_payload_bytes);
        if(err) goto bail;
    }

    *auxiliaryMetadata = newAuxiliaryMetadata;

    if(err) goto bail;

bail:
    return err;
}

MP4Err bgw_parse_independent_frame(u8* packetPayload, u32 packetLength, bgw_independent_frame** independentFrame,
    bgw_stream* stream)
{
    MP4Err err = MP4NoErr;
    u32 size;
    u8 tmp8;
    u16 tmp16;
    u32 tmp32;
    u32 i;
    u32 j;
    u32 k;

    u32 numChannelGroups;
    u16 numChannels;
    bgw_independent_frame* newIF;

    BitBuffer bb;
    err = BitBuffer_Init(&bb, packetPayload, packetLength); if (err) goto bail;

    newIF = calloc(1, sizeof(bgw_independent_frame));

    // Channel group parameter set ID
    err = GetBytes(&bb, 1, &tmp8);
    if(err) goto bail;
    newIF->if_channel_group_parameter_set_id = tmp8;

    if(!stream->waveformParameterSetList || !stream->waveformParameterSetList[0]->wps) BAILWITHERROR(MP4BadDataErr);
    numChannelGroups = stream->waveformParameterSetList[0]->wps->numChannelGroups;

    if (numChannelGroups > 1)
    {
        // Channel group ID
        tmp32 = GetBits(&bb, ceil_log2(numChannelGroups), &err);
        if(err) goto bail;
        newIF->if_channel_group_id = tmp32;
    }

    numChannels = stream->waveformParameterSetList[0]->wps->channelGroupsInfo[newIF->if_channel_group_id]->numChannels;

    newIF->if_mean_per_channel = calloc(numChannels, sizeof(u16));

    for(i = 0; i < numChannels; i++)
    {
        // Mean per channel
        tmp32 = GetBits(&bb, 16, &err);
        if(err) goto bail;
        newIF->if_mean_per_channel[i] = (u16)tmp32;
    }

    // @todo: Calculate CrossChannelPredInputChDistMinus1 if necessary

    if(!stream->channelGroupParameterSetList) BAILWITHERROR(MP4BadDataErr);
    if (stream->channelGroupParameterSetList[0]->cgps->cgps_length_signal_mode_flag)
    {
        // Indep num samples per channel minus1
        tmp32 = GetBits(&bb, 32, &err);
        if(err) BAILWITHERROR(err)
        newIF->if_indep_num_samples_per_channel_minus1 = tmp32;
    }

    if (stream->channelGroupParameterSetList[0]->cgps->cgps_ctx_init_flag)
    {
        // CGPS ctx init flag
        tmp32 = GetBits(&bb, 1, &err);
        if(err) goto bail;
        newIF->if_ctx_init_mode_flag = (u8)tmp32;
    }

    //fprintf(stdout, "Samples in frame sequence: %u\n", newIF->if_indep_num_samples_per_channel_minus1);
    // @todo Review retrieval of sample numbers
    *independentFrame = newIF;

bail:
    TEST_RETURN(err);
    return err;
}

MP4Err bgw_parse_dependent_frame(u8* packetPayload, u32 packetLength, bgw_dependent_frame **dependentFrame, bgw_stream* stream)
{
    MP4Err err = MP4NoErr;
    u32 size;
    u8 tmp8;
    u16 tmp16;
    u32 tmp32;
    u32 i;
    u32 j;
    u32 k;

    u32 numChannelGroups;
    bgw_dependent_frame* newDF;

    BitBuffer bb;
    err = BitBuffer_Init(&bb, packetPayload, packetLength); if (err) goto bail;

    newDF = calloc(1, sizeof(bgw_dependent_frame));
    if(!newDF) BAILWITHERROR(MP4NoMemoryErr);

    if(!stream->waveformParameterSetList || !stream->waveformParameterSetList[0]->wps) BAILWITHERROR(MP4BadDataErr);
    numChannelGroups = stream->waveformParameterSetList[0]->wps->numChannelGroups;

    if(numChannelGroups > 1)
    {
        // Channel group ID
        tmp32 = GetBits(&bb, ceil_log2(numChannelGroups), &err);
        if(err) goto bail;
        newDF->df_channel_group_id = tmp32;
    }

    (*dependentFrame) = newDF;

bail:
    TEST_RETURN(err);
    return err;
}