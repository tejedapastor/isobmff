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

#include <assert.h>
#include "bgw.h"

/**
 * @brief Fills the BGW decoderConfigRecord atom with the information parsed from the bitstream.
 * @param decoderConfigRecord Output BGW DecoderConfigRecord atom.
 * @param stream Struct containing the necessary information of the bitstream.
 * @param parameters Input parameters
 * @return MP4NoErr on success, a negative integer on error.
 */
MP4Err bgw_populate_decoder_config_record(ISOBGWConfigAtomPtr* decoderConfigRecord, 
  bgw_stream* stream, 
  bgw_params *parameters)
{
  MP4Err err;
  err = ISOBadDataErr;

  u8 cgInfoPresent = 0;
  u32 numChannelGroups = 0;
  u32 maxChannelCount = 0;
  u32 maxSamplingRate = 0;
  u32 numArrays = 0;

  bgw_waveform_parameter_set* wps = NULL;
  bgw_configuration_set* cs = NULL;
  bgw_decoder_config_record_channel_group** channelGroups;

  int i, j;

  (*decoderConfigRecord)->profile_level_idc = parameters->profile_level_idc;
  (*decoderConfigRecord)->normative_encoder_flag = parameters->normative_encoder_flag;
  (*decoderConfigRecord)->substream_present_flag = stream->numSubstreams >= 1;

  // The decoderConfigRecordAtom only supports 1 substream with 1 WPS. 
  // todo: extend to support multiple substreams
  bgw_wps_packet_info* wpsPacketInfo = stream->waveformParameterSetList[0];
  if(!wpsPacketInfo || !wpsPacketInfo->wps) BAILWITHERROR(MP4BadDataErr);

  wps = wpsPacketInfo->wps;

  if(stream->configurationSetList)
  {
    bgw_cs_packet_info* csPacketInfo = stream->configurationSetList[0]; // TODO: choose proper CS
    if(!csPacketInfo || !csPacketInfo->cs)
    {
      printf("No ConfigurationSet packet found in bitstream");
    } else
    {
      cs = csPacketInfo->cs;
    }
  }

  numChannelGroups = wps->numChannelGroups;
  cgInfoPresent = numChannelGroups > 0;

  if(cgInfoPresent) 
  {
    channelGroups = malloc(numChannelGroups * sizeof(bgw_decoder_config_record_channel_group*));
    if(!channelGroups) BAILWITHERROR(MP4BadDataErr);

    for(j = 0; j < numChannelGroups; j++)
    {
      bgw_decoder_config_record_channel_group* channelGroup = 
        calloc(1, sizeof(bgw_decoder_config_record_channel_group));
      if(!channelGroup) BAILWITHERROR(MP4BadDataErr);

      channelGroup->channel_group_id = j; // TODO: Review channel_group_id
      channelGroup->substream_id = wps->substreamId;
      channelGroup->cg_signal_type = cs ? cs->cs_wps[0]->cs_signal_type_idx[j] : 0;

      channelGroups[j] = channelGroup;
    }
  }

  if(wps->totalNumChannels > maxChannelCount)
  {
    maxChannelCount = wps->totalNumChannels;
  }

  if(cs)
  {
    
    for(j = 0; j < cs->cs_wps[0]->cs_num_sampling_freqs_in_wps; j++)
    {
      if(cs->cs_wps[0]->cs_sampling_freq[j] > maxSamplingRate)
      {
        maxSamplingRate = cs->cs_wps[0]->cs_sampling_freq[j];
      }
    }
  }
  (*decoderConfigRecord)->cg_info_present_flag = cgInfoPresent;
  (*decoderConfigRecord)->num_substreams = stream->numSubstreams;
  (*decoderConfigRecord)->num_channel_groups = numChannelGroups;

  if(numChannelGroups)
  {
    (*decoderConfigRecord)->channelGroups = calloc(numChannelGroups, sizeof(bgw_decoder_config_record_channel_group));
    if(!(*decoderConfigRecord)->channelGroups) BAILWITHERROR(MP4BadDataErr);

    for(i = 0; i < numChannelGroups; i++)
    {
      (*decoderConfigRecord)->channelGroups[i].substream_id = channelGroups[i]->substream_id;  
      (*decoderConfigRecord)->channelGroups[i].channel_group_id = channelGroups[i]->channel_group_id;
      (*decoderConfigRecord)->channelGroups[i].cg_signal_type = channelGroups[i]->cg_signal_type;
    }
  }
  
  (*decoderConfigRecord)->max_channel_count = maxChannelCount;
  (*decoderConfigRecord)->max_sampling_rate_numerator = maxSamplingRate;
  (*decoderConfigRecord)->max_sampling_rate_denumerator = 1;

  (*decoderConfigRecord)->num_of_arrays = 3;

  // WPS packet list
  (*decoderConfigRecord)->arrays[0].packet_type = BGW_WPS_SPT;
  (*decoderConfigRecord)->arrays[0].num_packets = stream->numWPS;

  for(i = 0; i < stream->numWPS; i++)
  {
    bgw_decoder_config_record_array_packet_data* wpsPacketData;
    wpsPacketData = calloc(1, sizeof(bgw_decoder_config_record_array_packet_data));
    if(!wpsPacketData) BAILWITHERROR(MP4BadDataErr);

    wpsPacketData->packet_length = stream->waveformParameterSetList[i]->packet_length;
    err = ISONewHandle(wpsPacketData->packet_length, &(wpsPacketData->packet));
    if(err) BAILWITHERROR(err);

    memcpy(wpsPacketData->packet, 
      stream->waveformParameterSetList[i]->packet_data, 
      stream->waveformParameterSetList[i]->packet_length);

    err = MP4AddListEntry(wpsPacketData->packet, (*decoderConfigRecord)->arrays[0].packetList);
    if(err) BAILWITHERROR(err);
    
  }
  
  // CGPS packet list
  (*decoderConfigRecord)->arrays[1].packet_type = BGW_CGPS_SPT;
  //(*decoderConfigRecord)->arrays[1].num_packets = stream->numCGPS;

  // Temporary action to avoid error when treating CGPS packets
  // @todo Find error source and fix
  (*decoderConfigRecord)->arrays[1].num_packets = 0;

  for(i = 0; i < stream->numCGPS; i++)
  {
    bgw_decoder_config_record_array_packet_data* cgpsPacketData;
    cgpsPacketData = calloc(1, sizeof(bgw_decoder_config_record_array_packet_data));
    if(!cgpsPacketData) BAILWITHERROR(MP4BadDataErr);

    cgpsPacketData->packet_length = stream->channelGroupParameterSetList[i]->packet_length;

    err = ISONewHandle(cgpsPacketData->packet_length, &(cgpsPacketData->packet));
    if(err) BAILWITHERROR(err);

    memcpy(cgpsPacketData->packet, 
      stream->channelGroupParameterSetList[i]->packet_data, 
      stream->channelGroupParameterSetList[i]->packet_length);

    //err = MP4AddListEntry(cgpsPacketData->packet, (*decoderConfigRecord)->arrays[1].packetList);
    //if(err) BAILWITHERROR(err);
  }
  
  // CS packet list
  (*decoderConfigRecord)->arrays[2].packet_type = BGW_CONFIG_SET_SPT;
  (*decoderConfigRecord)->arrays[2].num_packets = stream->numCS;

  for(i = 0; i < stream->numCS; i++)
  {
    bgw_decoder_config_record_array_packet_data* csPacketData;
    csPacketData = calloc(1, sizeof(bgw_decoder_config_record_array_packet_data));
    if(!csPacketData) BAILWITHERROR(MP4BadDataErr);

    csPacketData->packet_length = stream->configurationSetList[i]->packet_length;

    err = ISONewHandle(csPacketData->packet_length, &(csPacketData->packet));
    if(err) BAILWITHERROR(err);

    memcpy(csPacketData->packet, 
      stream->configurationSetList[i]->packet_data, 
      stream->configurationSetList[i]->packet_length);

    err = MP4AddListEntry(csPacketData->packet, (*decoderConfigRecord)->arrays[2].packetList);
    if(err) BAILWITHERROR(err);
  }

  bail:
    TEST_RETURN(err);
    return err;
}

/**
 * @brief Adds a sample containing a bgw_stream_packet to the media atom.
 * @param trak Track atom.
 * @param media Media atom.
 * @param firstSample Indication of first sample.
 * @param streamPacket Struct containing the data of the stream packet.
 * @param decoderConfigRecord BGW DecoderConfigRecord atom.
 * @param parameters Input parameters
 * @param cumulativeOffset Cumulative offset of the samples already inserted in the file.
 * @return MP4NoErr on success, a negative integer on error.
 */
static MP4Err bgw_add_samples_to_media(ISOTrack trak, ISOMedia media,
      u8 firstSample,
      bgw_stream_packet* streamPacket, 
      ISOBGWConfigAtomPtr decoderConfigRecord,
      bgw_params* parameters,
      u32 cumulativeOffset)
{
  MP4Err err;

  // Handle definition
  ISOHandle sampleDataH;
  ISOHandle sampleDurationH;
  ISOHandle sampleSizeH;
  ISOHandle sampleEntryH;
  ISOHandle sampleOffsetH;
  ISOHandle syncSampleH;

  err = MP4NoErr;

  err = ISONewHandle(streamPacket->total_length, &sampleDataH);
	if (err) BAILWITHERROR(err);

  err = ISOSetMediaLanguage(media, "und"); /* undetermined */
	if (err) BAILWITHERROR(err);

	err = ISONewHandle(1, &sampleEntryH);
	if (err) BAILWITHERROR(err);

  // Copy packet data to sample data
  memcpy((*sampleDataH), 
          streamPacket->stream_packet_header, 
          streamPacket->stream_packet_header_size);
  memcpy((*sampleDataH) + streamPacket->stream_packet_header_size, 
          streamPacket->syntax_structure_bytes, 
          streamPacket->num_bytes_in_syntax_structure);

  // Sample size
  err = ISONewHandle(sizeof(u32), &sampleSizeH);
  err = ISOGetHandleSize(sampleDataH,(u32*)*sampleSizeH);

  // Sample entry

  // Sample offset
  err = ISONewHandle(sizeof(u32), &sampleOffsetH);
  if (err) BAILWITHERROR(err);
  *(u32*)*sampleOffsetH = cumulativeOffset;

  // Sync sample
  err = ISONewHandle(sizeof(u32) * 2, &syncSampleH); // @todo: Check sync sample
	if (err) BAILWITHERROR(err);
  ((u32*)*syncSampleH)[0] = 1;
	((u32*)*syncSampleH)[1] = 1;

  u32 dataReferenceIndex = 1;

  // Create sample description with BGW decoderConfigRecord atom
  MP4GenericAtomRecord genericAtomRecord;
  genericAtomRecord.data = decoderConfigRecord;

	err = ISONewBGWSampleDescription(trak, sampleEntryH, dataReferenceIndex, 
    genericAtomRecord);
  if(err) goto bail;

  err = ISONewHandle(sizeof(u32), &sampleDurationH);
	if (err) BAILWITHERROR(err);
  *((u32*)*sampleDurationH) = parameters->sample_duration; // TODO: Retrieve from input params

	err = MP4AddMediaSamples(media, sampleDataH, 1,
                            sampleDurationH,
                            sampleSizeH,
                            firstSample ? sampleEntryH : NULL, sampleOffsetH, syncSampleH);
  if(err) goto bail;

  if (sampleEntryH) {
		err = ISODisposeHandle(sampleEntryH);
		sampleEntryH = NULL;
	}

  if (syncSampleH) err = ISODisposeHandle(syncSampleH);

	err = ISODisposeHandle(sampleDataH);
	if (err) BAILWITHERROR(err);
	err = ISODisposeHandle(sampleSizeH);
	if (err) BAILWITHERROR(err);
	err = ISODisposeHandle(sampleDurationH);
	if (err) BAILWITHERROR(err);

bail:
  TEST_RETURN(err);
  return err;
}

/**
 * @brief Generates an ISOBMFF MP4 file from the information parsed from the T.261 bitstream.
 * @param parameters CLI input parameters.
 * @param stream Struct containing the main information of the T.261 bitstream.
 * @return MP4NoErr on success, a negative integer on error.
 */
MP4Err bgw_create_MP4_file(bgw_params *parameters, bgw_stream* stream)
{
  MP4Err err;
  ISOMovie moov;
  ISOTrack trak;
  ISOMedia media;
  int index_packet;

  u32 initialObjectDescriptorID;
  u8 OD_profileAndLevel;
  u8 scene_profileAndLevel;
  u8 audio_profileAndLevel;
  u8 visual_profileAndLevel;
  u8 graphics_profileAndLevel;
  u64 mediaDuration;

  char *filename = parameters->output ? parameters->output : "bgw_mov.mp4";
  err = ISONoErr;

  initialObjectDescriptorID = 0;
  OD_profileAndLevel        = 0xff; /* none required */
  scene_profileAndLevel     = 0xff; /* none required */
  audio_profileAndLevel     = 0xff; /* none required */
  visual_profileAndLevel    = 0xff; /* none required */
  graphics_profileAndLevel  = 0xff; /* none required */
  
  // Create new MP4 movie with default values
  err = MP4NewMovie(&moov, initialObjectDescriptorID, OD_profileAndLevel, scene_profileAndLevel,
                    audio_profileAndLevel, visual_profileAndLevel, graphics_profileAndLevel);
  if(err) BAILWITHERROR(err);

  // @todo: Review necessary/compatible brands for BGW
  err = ISOSetMovieBrand(moov, MP4_FOUR_CHAR_CODE('m', 'p', '4', '1'), 0);
  if(err) BAILWITHERROR(err);

	err = ISOSetMovieCompatibleBrand(moov, MP4_FOUR_CHAR_CODE('b', 'g', 'w', '1'));
  if(err) BAILWITHERROR(err);

  // @todo: enable support for multiple tracks.

  // Create a new trak for the movie
  err = ISONewMovieTrackWithID(moov, ISONewTrackIsWaveform, 1, &trak);
  if(err) BAILWITHERROR(err);
  err = ISONewTrackMedia(trak, &media, ISOWaveformHandlerType, 30000, NULL);
  if(err) BAILWITHERROR(err);
  err = ISOBeginMediaEdits(media);
  if(err) BAILWITHERROR(err);

  // Create BGW DecoderConfigRecord Atom
  ISOBGWConfigAtomPtr decoderConfigRecord;
  err = MP4CreateBGWConfigAtom(&decoderConfigRecord);
  if(err) BAILWITHERROR(err);

  err = bgw_populate_decoder_config_record(&decoderConfigRecord, stream, parameters);
  if(err) BAILWITHERROR(err);

  // Add each packet as a new sample to the media atom
  // @todo Put full frame sequences in a single sample.
  u32 cumulativeOffset = 0;

  for(index_packet = 0; index_packet < stream->numStreamPackets; index_packet++)
  {
    err = bgw_add_samples_to_media(trak, media, index_packet == 0, 
      stream->stream_packets[index_packet], 
      decoderConfigRecord, parameters, cumulativeOffset);
    if(err) BAILWITHERROR(err);
    cumulativeOffset += stream->stream_packets[index_packet]->total_length;
  }

  // Get media durations
  // @todo Manage media durations
  err = ISOGetMediaDuration(media, &mediaDuration); if (err) goto bail;
  err = ISOEndMediaEdits(media);
  if(err) goto bail;

  // Write movie to MP4 file
  err = ISOWriteMovieToFile(moov, filename);
  if(err) goto bail;

  // Clean movie
  err = ISODisposeMovie(moov);
  if(err) goto bail;

  if(!err)
  {
    fprintf(stdout, "The MP4 file %s has been created successfully\n", filename);
  }
  return err;

bail:
  TEST_RETURN(err);
  return err;
}

/**
 * @brief Entry point.
 * @param argc Number of CLI arguments.
 * @param argv Arguments array.
 * @return MP4NoErr on success, a negative integer on error.
 */
int main(int argc, char *argv[])
{
  MP4Err err = MP4NoErr;

  printf("Parsing input parameters...\n");

  bgw_params *inputParams = calloc(1, sizeof(bgw_params));
  if(!parse_input_params(argc, argv, inputParams)) 
  {
    err = MP4BadParamErr;
    BAILWITHERROR(err);
  }
  
  if(!inputParams->input || !inputParams->output)
  {
    printf("Please specify the input and output files. Exiting...\n");
    err = MP4BadParamErr;
    BAILWITHERROR(err);
  }

  if(!inputParams->sample_duration)
  {
    // @todo Define a proper default sample duration
    inputParams->sample_duration = 10;
  }

  FILE *inputFile = fopen(inputParams->input, "rb");
  if(!inputFile) 
  {
    printf("The input file is not valid. Exiting...\n");
    err = MP4BadParamErr;
    BAILWITHERROR(err);
  }

  fprintf(stdout, "Parsing bitstream from file %s...\n", inputParams->input);

  bgw_stream* stream;
  err = bgw_parse_stream(inputFile, &stream);
  if(!stream) BAILWITHERROR(err);

  // Generate MP4 file with parsed information
  err = bgw_create_MP4_file(inputParams, stream);
  if(err) BAILWITHERROR(err);

bail:
  if(inputFile)
  {
    fclose(inputFile);
  }

  TEST_RETURN(err);
  return err;
}