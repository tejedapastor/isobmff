/**
 * @file BGWConfigAtom.c
 * @brief BGW configuration atom implementation
 * @version 0.1
 *
 * @copyright @TODO
 *
 */

#include "MP4Atoms.h"
#include "MP4Descriptors.h"
#include <stdlib.h>
#include <string.h>

static void destroy(MP4AtomPtr s)
{
  MP4Err err;
  u32 i;
  ISOBGWConfigAtomPtr self;
  err  = MP4NoErr;
  self = (ISOBGWConfigAtomPtr)s;
  if(self == NULL) BAILWITHERROR(MP4BadParamErr)

  if(self->num_of_arrays)
  {
    for(i = 0; i < 3; i++)
    {
      err = MP4DeleteLinkedList(self->arrays[i].packetList);
      if(err) goto bail;
      self->arrays[i].packetList = NULL;
    }
  }
  if(self->super) self->super->destroy(s);
bail:
  TEST_RETURN(err);
  return;
}

static MP4Err serialize(struct MP4Atom *s, char *buffer)
{
  MP4Err err;
  u32 x;
  u32 ui, array_index, packet_index;
  
  ISOBGWConfigAtomPtr self = (ISOBGWConfigAtomPtr)s;
  err = MP4NoErr;

  err = MP4SerializeCommonFullAtomFields((MP4FullAtomPtr)s, buffer);
  if(err) goto bail;
  buffer += self->bytesWritten;

  /* profileLevelIdc(8) + normativeEncoderFlag(1) + substreamPresentFlag(1) 
    + cgInfoPresentFlag(1) + numSubstreams(5) */
  x = (self->profile_level_idc << 8) | (self->normative_encoder_flag << 7) | (self->substream_present_flag << 6) 
      | (self->cg_info_present_flag << 5) | self->num_substreams;
  PUT16_V(x);

  /* numChannelGroups(16) */
  PUT16_V(self->num_channel_groups);

  if(self->num_channel_groups)
  {
    for(ui = 0; ui < self->num_channel_groups; ui++)
    {
      if(self->substream_present_flag)
      {
        PUT16_V(self->channelGroups[ui].substream_id);
      }

      PUT16_V(self->channelGroups[ui].channel_group_id);

      if(self->cg_info_present_flag)
      {
        PUT8_V(self->channelGroups[ui].cg_signal_type);
      }
    }
  }

  PUT16_V(self->max_channel_count);
  PUT16_V(self->max_sampling_rate_numerator);
  PUT16_V(self->max_sampling_rate_denumerator);

  PUT8_V(self->num_of_arrays);

  for(array_index = 0; array_index < self->num_of_arrays; array_index++)
  {
    /* packetType(3) + numPackets(13) */
    x = (self->arrays[array_index].packet_type << 13) | self->arrays[array_index].num_packets;
    PUT16_V(x);

    for(packet_index = 0; packet_index < self->arrays[array_index].num_packets; packet_index++)
    {
      MP4Handle b;
      u32 the_size;
      err = MP4GetListEntry(self->arrays[array_index].packetList, packet_index, (char **)&b);
      if(err) goto bail;

      err = MP4GetHandleSize(b, &the_size);
      if(err) goto bail;

      PUT16_V(the_size);
      PUTBYTES(*b, the_size);
    }
  }

bail:
  TEST_RETURN(err);
  return err;
}

static MP4Err calculateSize(struct MP4Atom *s)
{
  MP4Err err;
  ISOBGWConfigAtomPtr self = (ISOBGWConfigAtomPtr)s;
  u32 i, j, k;

  err = MP4NoErr;

  err = MP4CalculateFullAtomFieldSize((MP4FullAtomPtr)s);
  if(err) goto bail;

  /* profile_level_idc */
  self->size += 1;

  /* normative_encoder_flag(1) + substream_present_flag(1) + cg_info_present_flag(1) + num_substreams(5) */
  self->size += 1;

  /* num_channel_groups(16) */
  self->size += 2;

  for(i = 0; i < self->num_channel_groups; i++)
  {
    if(self->substream_present_flag)
    {
      /* substream_id(16) */
      self->size += 2;
    }

    /* channel_group_id(16) */
    self->size += 2;

    if(self->cg_info_present_flag)
    {
      /* cg_signal_type(8) */
      self->size += 1;
    }
  }

  /* max_channel_count(16) + max_sampling_rate_numerator(16) + max_sample_rate_denumerator(16) */
  self->size += 6;

  /* num_of_arrays(8) */
  self->size += 1;

  for(j = 0; j < self->num_of_arrays; j++)
  {
    /* packet_type(3) + num_packets(13) */
    self->size += 2;

    for(k = 0; k < self->arrays[j].num_packets; k++)
    {
      MP4Handle b;
      u32 the_size;

      err = MP4GetListEntry(self->arrays[j].packetList, k, (char **)&b);
      if(err) goto bail;

      err = MP4GetHandleSize(b, &the_size);
      if(err) goto bail;

      self->size += 2;
      self->size += the_size;
    }
  }

  bail:
    TEST_RETURN(err);
    return err;
}

static MP4Err createFromInputStream(MP4AtomPtr s, MP4AtomPtr proto, MP4InputStreamPtr inputStream)
{
  MP4Err err;
  ISOBGWConfigAtomPtr self = (ISOBGWConfigAtomPtr)s;
  u32 i, j;
  u32 x;

  err = MP4NoErr;
  if(self == NULL) BAILWITHERROR(MP4BadParamErr);
  err = self->super->createFromInputStream(s, proto, (char *)inputStream);
  if(err) goto bail;

  /* profile_level_idc(8) */
  GET8_V(x);
  self->profile_level_idc = x;

  /* normative_encoder_flag(1), substream_present_flag(1), cg_info_present_flag(1), num_substreams(5)*/
  GET8_V(x);
  self->normative_encoder_flag = (x >> 7);
  self->substream_present_flag = (x & 0x40 >> 6);
  self->cg_info_present_flag = (x & 0x1F);

  /* num_channel_groups(16) */
  GET16_V(x);
  self->num_channel_groups = x;

  if(self->num_channel_groups)
  {
    self->channelGroups = calloc(self->num_channel_groups, 5);

    for(i = 0; i < self->num_channel_groups; i++)
    {
      if(self->substream_present_flag)
      {
        /* substream_id(16) */
        GET16_V(x);
        self->channelGroups[i].substream_id = x; 
      }

      /* channel_group_id(16) */
      GET16_V(x);
      self->channelGroups[i].channel_group_id = x;

      if(self->cg_info_present_flag)
      {
        /* cg_signal_type(8) */
        GET8_V(x);
        self->channelGroups[i].cg_signal_type = x;
      }
    }
  }

  /* max_channel_count(16) */
  GET16_V(x);
  self->max_channel_count = x;

  /* max_sampling_rate_numerator(16) */
  GET16_V(x);
  self->max_sampling_rate_numerator = x;

  /* max_sampling_rate_denumerator(16) */
  GET16_V(x);
  self->max_sampling_rate_denumerator = x;

  /* num_of_arrays(8) */
  GET8_V(x);
  self->num_of_arrays = x;

  for(i = 0; i < self->num_of_arrays; i++)
  {
    GET16_V(x);
    self->arrays[i].packet_type = x >> 11;
    self->arrays[i].num_packets = x & 0x1FFF;

    err = MP4MakeLinkedList(&self->arrays[i].packetList);

    for(j = 0; j < self->arrays[i].num_packets; j++)
    {
      MP4Handle packet;
      u16 packetLength;

      GET16_V(packetLength);
      err = MP4NewHandle(packetLength, &packet);
      if(err) goto bail;
      GETBYTES_V_MSG(packetLength, *packet, "Packet");
      err = MP4AddListEntry((void *)packet, self->arrays[i].packetList);
      if(err) goto bail;
    }
  }
  
bail:
  TEST_RETURN(err);
  return err;
}

MP4Err MP4CreateBGWConfigAtom(ISOBGWConfigAtomPtr *outAtom)
{
    MP4Err err;
    ISOBGWConfigAtomPtr self;
    u32 i;

    u32 packetType[3] = {1, 2, 19};
    self = (ISOBGWConfigAtomPtr)calloc(1, sizeof(ISOBGWConfigAtom));
    TESTMALLOC(self);

    err = MP4CreateFullAtom((MP4AtomPtr)self);
    if(err) goto bail;
    self->type = ISOBGWConfigAtomType;
    self->name = "BgwConfigurationBox";
    self->createFromInputStream = (cisfunc)createFromInputStream;
    self->destroy = destroy;
    self->calculateSize = calculateSize;
    self->serialize = serialize;

    u32 arrayLength = sizeof(packetType)/sizeof(packetType[0]);

    for(i = 0; i < arrayLength; i++)
    {
        err = MP4MakeLinkedList(&self->arrays[i].packetList);
        if(err) goto bail;
        self->arrays[i].packet_type = packetType[i];
        self->arrays[i].num_packets = 0;
    }

    *outAtom = self;
bail:
    TEST_RETURN(err);
    return err;
}