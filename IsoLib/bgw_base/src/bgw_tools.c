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

#include <math.h>
#include <string.h>
#include "ISOMovies.h"
#include "bgw_structures.h"
#include "bgw_tools.h"

u32 DEFAULT_SAMPLE_DURATION = 10;

MP4Err bit_buffer_init(BitBuffer *bb, u8 *p, u32 length) {
	int err = MP4NoErr;

	if (length > 0x0fffffff) {
		err = MP4BadParamErr;
		goto bail;
	}

	bb->ptr = (void*)p;
	bb->length = length;

	bb->cptr = (void*)p;
	bb->cbyte = *bb->cptr;
	bb->curbits = 8;

	bb->bits_left = length * 8;

	bb->prevent_emulation = 1;
	bb->emulation_position = (bb->cbyte == 0 ? 1 : 0);

bail:
	return err;
}

u32 ceil_log2(u32 x) {
	u32 ret = 0;
	while (x>((u32)1 << ret)) ret++;
	return ret;
}

u32 get_bits(BitBuffer *bb, u32 nBits, MP4Err *errout) {
	MP4Err err = MP4NoErr;
	int myBits;
	int myValue;
	int myResidualBits;
	int leftToRead;

	myValue = 0;
	if (nBits>bb->bits_left) {
		err = MP4EOF;
		goto bail;
	}

	if (bb->curbits <= 0) {
		bb->cbyte = *++bb->cptr;
		bb->curbits = 8;

		if (bb->prevent_emulation != 0) {
			if ((bb->emulation_position >= 2) && (bb->cbyte == 3)) {
				bb->cbyte = *++bb->cptr;
				bb->bits_left -= 8;
				bb->emulation_position = bb->cbyte ? 0 : 1;
				if (nBits>bb->bits_left) {
					err = MP4EOF;
					goto bail;
				}
			} else if (bb->cbyte == 0) bb->emulation_position += 1;
			else bb->emulation_position = 0;
		}
	}

	if (nBits > bb->curbits)
		myBits = bb->curbits;
	else
		myBits = nBits;

	myValue = (bb->cbyte >> (8 - myBits));
	myResidualBits = bb->curbits - myBits;
	leftToRead = nBits - myBits;
	bb->bits_left -= myBits;

	bb->curbits = myResidualBits;
	bb->cbyte = ((bb->cbyte) << myBits) & 0xff;

	if (leftToRead > 0) {
		u32 newBits;
		newBits = get_bits(bb, leftToRead, &err);
		myValue = (myValue << leftToRead) | newBits;
	}

bail:
	if (errout) *errout = err;
	return myValue;
}

MP4Err get_bytes(BitBuffer *bb, u32 nBytes, u8 *p) {
	MP4Err err = MP4NoErr;
	unsigned int i;

	for (i = 0; i < nBytes; i++) {
		*p++ = (u8)get_bits(bb, 8, &err);
		if (err) break;
	}

	return err;
}

u32 read_golomb_uev(BitBuffer *bb, MP4Err *errout) 
{
	MP4Err err = MP4NoErr;

	u32 power = 1;
	u32 value = 0;
	u32 leading = 0;
	u32 nbits = 0;

	leading = get_bits(bb, 1, &err);  if (err) goto bail;

	while (leading == 0) {
		power = power << 1;
		nbits++;
		leading = get_bits(bb, 1, &err);  if (err) goto bail;
	}

	if (nbits > 0) {
		value = get_bits(bb, nbits, &err); if (err) goto bail;
	}

bail:
	if (errout) *errout = err;
	return (power - 1 + value);
}

s32 read_golomb_sev(BitBuffer *bb, MP4Err *errout)
{
	MP4Err err = MP4NoErr;

	u32 codeNum = read_golomb_uev(bb, errout);

	if (codeNum < 0) 
	{
		err = MP4BadParamErr;
		goto bail;
	}

	return pow(-1, codeNum+1) * ceil(codeNum/2);

bail:
	return err;
}

u32 read_escaped_value(BitBuffer *bb, u32 *readValue, int k, int m, int n, MP4Err *errout)
{
	u32 value, addValue;
	u32 bitsUsed = 0;

	value = get_bits(bb, k, errout);
	if(*errout) goto bail;
	bitsUsed += k;

	if(value == (1<<k)-1)
	{
		addValue = get_bits(bb, m, errout);
		if(*errout) goto bail;
		bitsUsed += m;

		value += addValue;
		if(addValue == (1<<m)-1)
		{
			addValue = get_bits(bb, n, errout);
			if(*errout) goto bail;
			bitsUsed += n;

			value += addValue;
		}
	}
	*readValue = value;
	return bitsUsed;
bail:
	return -1;
}

char* read_string_stv(BitBuffer *bb, MP4Err *errout)
{
    size_t capacity = 16;
    size_t len = 0;
    char* buf = malloc(capacity);
    if(!buf) { *errout = MP4NoMemoryErr; return NULL; }

    while(1)
    {
        MP4Err e = MP4NoErr;
        u32 byte = get_bits(bb, 8, &e);
        if(e) { free(buf); *errout = e; return NULL; }
        if(byte == 0x00) break;  /* null terminator — consumed but not stored */
        if(len + 1 >= capacity)
        {
            capacity *= 2;
            char* tmp = realloc(buf, capacity);
            if(!tmp) { free(buf); *errout = MP4NoMemoryErr; return NULL; }
            buf = tmp;
        }
        buf[len++] = (char)byte;
    }
    buf[len] = '\0';
    return buf;
}

int parse_input_params(int argc, char* argv[], bgw_params *parameters) {

	int paramIndex = 1;

	while(paramIndex < argc)
	{
		if (argc - 1 == paramIndex) 
		{
			return 0;
		}

		if(argv[paramIndex][0] == '-')
		{
			switch(argv[paramIndex][1])
			{
				case 'i':
					{
						u32 inputLen = strlen(argv[paramIndex + 1]);
						parameters->input = malloc(inputLen + 1);
						memcpy(parameters->input, argv[paramIndex + 1], inputLen + 1);
						break;
					}
				case 'o':
					{
						u32 outputLen = strlen(argv[paramIndex + 1]);
						parameters->output = malloc(outputLen + 1);
						memcpy(parameters->output, argv[paramIndex + 1], outputLen + 1);
						break;
					}
				case 'p':
					{
						parameters->profile_level_idc = atoi(argv[paramIndex + 1]);
						break;
					}
				case 'n':
					{
						parameters->normative_encoder_flag = atoi(argv[paramIndex + 1]);
						break;
					}
				case 'd':
					{
						parameters->sample_duration = atoi(argv[paramIndex + 1]);
						break;
					}
				default:
					break;
			}

			paramIndex += 2;
		} else
		{
			paramIndex++;
		}
	}

	return 1;
}