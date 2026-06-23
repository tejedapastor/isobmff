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

#include "MP4Atoms.h"
#include <stdlib.h>
#include <string.h>

static void destroy(MP4AtomPtr s)
{
  MP4Err err;
  MP4WaveformSampleEntryAtomPtr self;
  err  = MP4NoErr;
  self = (MP4WaveformSampleEntryAtomPtr)s;
  if(self == NULL) BAILWITHERROR(MP4BadParamErr)
  DESTROY_ATOM_LIST_F(ExtensionAtomList)
  if(self->super) self->super->destroy(s);
bail:
  TEST_RETURN(err);

  return;
}

static MP4Err serialize(struct MP4Atom *s, char *buffer)
{
  MP4Err err;
  MP4WaveformSampleEntryAtomPtr self = (MP4WaveformSampleEntryAtomPtr)s;
  err                              = MP4NoErr;

  err = MP4SerializeCommonBaseAtomFields(s, buffer);
  if(err) goto bail;
  buffer += self->bytesWritten;
  /* class SampleEntry */
  PUTBYTES(self->reserved, 6);
  PUT16(dataReferenceIndex);

  SERIALIZE_ATOM_LIST(ExtensionAtomList);
  assert(self->bytesWritten == self->size);
bail:
  TEST_RETURN(err);

  return err;
}

static MP4Err calculateSize(struct MP4Atom *s)
{
  MP4Err err;
  MP4WaveformSampleEntryAtomPtr self = (MP4WaveformSampleEntryAtomPtr)s;
  err                              = MP4NoErr;

  err = MP4CalculateBaseAtomFieldSize(s);
  if(err) goto bail;
  
  // Reserved + dataReferenceIndex
  self->size += 6 + 2;
  ADD_ATOM_LIST_SIZE(ExtensionAtomList);
bail:
  TEST_RETURN(err);

  return err;
}

static MP4Err createFromInputStream(MP4AtomPtr s, MP4AtomPtr proto, MP4InputStreamPtr inputStream)
{
  MP4Err err;
  MP4WaveformSampleEntryAtomPtr self = (MP4WaveformSampleEntryAtomPtr)s;

  err = MP4NoErr;
  if(self == NULL) BAILWITHERROR(MP4BadParamErr)
  err = self->super->createFromInputStream(s, proto, (char *)inputStream);
  if(err) goto bail;

  /* class SampleEntry */
  GETBYTES(6, reserved);
  GET16(dataReferenceIndex);

  GETATOM_LIST(ExtensionAtomList);

bail:
  TEST_RETURN(err);

  return err;
}

MP4Err MP4CreateWaveformSampleEntryAtom(MP4WaveformSampleEntryAtomPtr *outAtom)
{
  MP4Err err;
  MP4WaveformSampleEntryAtomPtr self;

  self = (MP4WaveformSampleEntryAtomPtr)calloc(1, sizeof(MP4WaveformSampleEntryAtom));
  TESTMALLOC(self)

  err = MP4CreateBaseAtom((MP4AtomPtr)self);
  if(err) goto bail;
  self->type = MP4WaveformSampleEntryAtomType;
  self->name = "waveform sample entry";
  err        = MP4MakeLinkedList(&self->ExtensionAtomList);
  if(err) goto bail;
  self->createFromInputStream = (cisfunc)createFromInputStream;
  self->destroy               = destroy;
  self->calculateSize         = calculateSize;
  self->serialize             = serialize;

  *outAtom = self;
bail:
  TEST_RETURN(err);
  return err;
}
