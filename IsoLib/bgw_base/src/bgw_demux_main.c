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

#include <stdio.h>
#include <assert.h>
#include "bgw.h"

/**
 * @brief Generates a file containing a T.261 bitstream compliant with the Draft 5 of the standard.
 * @param parameters Input parameters.
 * @param filename Path to the MP4 file.
 * @return MP4NoErr on success, a negative integer on error.
 */
MP4Err bgw_create_T261_bitstream(bgw_params *parameters, char *filename) {

  MP4Err err;

  u32 trackNumber = 1;
  u32 handlerType;
  ISOMovie moov;
  ISOTrack trak;
  ISOMedia media;
  ISOTrackReader reader;
  ISOHandle sampleH;
  ISOHandle decoderConfigH;
  ISOHandle sampleEntryH;
  ISOHandle outPacketH;

  u32 trackCount = 0;
	u32 mediaTimeScale = 0;
	u32 totalSamples = 0;
	u64 duration = 0;
  
  int i;

	FILE *out;

	out = fopen(parameters->output, "wb");
  if(!out)
  {
    printf("The output file is not valid. Exiting...\n");
    BAILWITHERROR(MP4BadParamErr);
  }

  err = ISONewHandle(1, &sampleEntryH); if(err) BAILWITHERROR(err);

  // Open in debug mode until definitive version
  // @todo Check if debug mode is useful/required
  err = ISOOpenMovieFile(&moov, filename, MP4OpenMovieDebug); if(err) BAILWITHERROR(err);

  err = MP4GetMovieTrackCount(moov, &trackCount); if(err) BAILWITHERROR(err);

  if(trackCount > 1)
  {
    printf("Current only one track is supported");
    BAILWITHERROR(MP4BadParamErr);
  }

  // Only one track is supported
  // @todo Enable support for multiple tracks
  err = ISOGetMovieIndTrack(moov, trackNumber, &trak);

	err = ISOGetTrackMedia(trak, &media); if (err) BAILWITHERROR(err);
	err = ISOGetMediaHandlerDescription(media, &handlerType, NULL); if (err) BAILWITHERROR(err);
	err = ISOCreateTrackReader(trak, &reader); if (err) BAILWITHERROR(err);
	err = ISONewHandle(0, &sampleH); if (err) BAILWITHERROR(err);
  
  ISOGetMediaSampleCount(media, &totalSamples); 

	// Get sample description from the trak
	err = MP4TrackReaderGetCurrentSampleDescription(reader, sampleEntryH); if (err) BAILWITHERROR(err);

  // Copy decoderConfigRecord atom from sampleEntry
  u32 configsize;
  MP4GetHandleSize(sampleEntryH, &configsize);
  err = ISONewHandle(configsize, &decoderConfigH); if(err) BAILWITHERROR(err);
  memcpy((*decoderConfigH), (*sampleEntryH), configsize);
  MP4GetHandleSize(decoderConfigH, &configsize);

  u32 outSize;
  u32 outSampleFlags;
  s32 outCTS, outDTS;

	err = ISONewHandle(1, &outPacketH); if (err) goto bail;

  // Write stream packets to output file
  for(i = 0; i < totalSamples; i++)
  {
    err = MP4TrackReaderGetNextAccessUnit(reader, outPacketH, &outSize, &outSampleFlags, &outCTS, &outDTS);
    if(err) BAILWITHERROR(err);
    fwrite(*outPacketH, outSize, 1, out);
  }

  if(!err)
  {
    fprintf(stdout, "The file %s containing the T.261 bitstream has been created successfully\n", parameters->output);
  }

bail:
  if(out)
  {
    fclose(out);
  }
    
  TEST_RETURN(err);
  return err;
}

/**
 * @brief Entry point.
 * @param argc Number of CLI arguments.
 * @param argv Arguments array.
 * @return MP4NoErr on success, a negative integer on error.
 */
int main(int argc, char* argv[])
{
  MP4Err err = MP4NoErr;

  bgw_params *inputParams = calloc(1, sizeof(bgw_params));
  if(!parse_input_params(argc, argv, inputParams)) goto bail;

  if(!inputParams->input || !inputParams->output)
  {
    printf("Please specify the input and output files. Exiting...\n");
    err = MP4BadParamErr;
    BAILWITHERROR(err);
  }

  FILE *inputFile = fopen(inputParams->input, "rb");
  if(!inputFile) 
  {
    printf("The input file is not valid. Exiting...\n");
    err = MP4BadParamErr;
    BAILWITHERROR(err);
  }

  fprintf(stdout, "Creating T.261 bitstream from file %s...\n", inputParams->input);

	err = bgw_create_T261_bitstream(inputParams, inputParams->input);
  if(err) BAILWITHERROR(err);

	return err;

bail:

  if(inputFile)
  {
    fclose(inputFile);
  }

  TEST_RETURN(err);
  return err;
}