/* TODO COPYRIGHT
 */
/*
  $Id: WaveformSampleEntryAtom.c,v 1.1.1.1 2002/09/20 08:53:35 julien Exp $
*/

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
