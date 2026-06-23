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

#ifndef __BGW_STRUCTURES_H__
#define __BGW_STRUCTURES_H__

#include "MP4Movies.h"
#include "MP4Atoms.h"

typedef struct 
{
	u8 *ptr;
	u32 length;
	u8 *cptr;
	u8 cbyte;
	u32 curbits;
	u32 bits_left;

	u8 prevent_emulation;	/* true or false */
	u8 emulation_position;	/* 0 usually, 1 after 1 zero byte, 2 after 2 zero bytes,
													3 after 00 00 03, and the 3 gets stripped */
} BitBuffer;

typedef struct 
{
  u32 stream_packet_type;
  u32 stream_packet_label;
  u32 stream_packet_length;
} bgw_stream_packet_header;

typedef struct
{
  u32 total_length;
  u32 stream_packet_header_size;
  bgw_stream_packet_header* stream_packet_header;
  u32 num_bytes_in_syntax_structure;
  u8 *syntax_structure_bytes;
} bgw_stream_packet;

typedef struct 
{
  u64 substream_id;                  /* if (substream_present_flag) unsigned int(16)[num_channel_groups] */
  u16 channel_group_id;              /* unsigned int(16)[num_channel_groups] */
  u8 cg_signal_type;                 /* if (cg_info_present_flag) unsigned int(8)[num_channel_groups] */
} bgw_decoder_config_record_channel_group;

typedef struct
{
  u32 packet_length;  /* unsigned int(16)[num_of_arrays][num_packets] */
  MP4Handle packet;   /* bit(8*packet_length)[num_of_arrays][num_packets] */
} bgw_decoder_config_record_array_packet_data;

typedef struct 
{
  u8 packet_type;                                       /* unsigned int(3)[num_of_arrays] */
  u16 num_packets;                                      /* unsigned int(13)[num_of_arrays] */
  bgw_decoder_config_record_array_packet_data* packets;
} bgw_decoder_config_record_array;

typedef struct 
{
  u32 wps_num_channels_in_next_group_minus1;                  /* ue(v) */
  u32 wps_num_channel_group_repetitions;                      /* ue(v) */
  u8* channel_group_reref_enable;                             /* u(1) */
  u8* channel_group_reref_mode;                               /* u(2) */
  u32* channel_group_reref_channel_idx;                       /* ue(v) */
  u8 wps_more_channel_groups_present_flag;                    /* unsigned int(1) */
  u32 numChannels;
  u32* channelGroupStartingPos;
} bgw_wps_channel_group_info;

typedef struct
{
  u32 wps_swap_frst_idx;                                     /* ue(v) */
  u32 wps_swap_scnd_idx_min_frst_idx_min1;                   /* ue(v) */
} bgw_wps_channel_swaps_info;

typedef struct 
{
  u8 wps_waveform_parameter_set_id;                           /* unsigned int(4) */
  bgw_wps_channel_group_info** channelGroupsInfo;
  u8 wps_channel_reordering_flag;                             /* unsigned int(1) */
  u32 wps_num_channel_swaps_minus1;                           /* ue(v) */
  u32 wps_num_annotation_channels;                            /* ue(v) */
  bgw_wps_channel_swaps_info* channelSwapsInfo;
  u32 numChannelGroups;
  u32 totalNumChannels;
  u32* channelGroupStartingPos;
  u32* annotationChannelNumSamples;
  u64 substreamId;
} bgw_waveform_parameter_set;

typedef struct
{
  u8 cgps_channel_group_parameter_set_id;       /* unsigned int(8) */
  u8 cgps_waveform_parameter_set_id;            /* unsigned int(4) */
  u8 cgps_length_signal_mode_flag;              /* unsigned int(1) */
  u8 cgps_frame_length_shift;                   /* unsigned int(4) */
  u8 cgps_max_min_block_size;                   /* unsigned int(6) */
  u8 cgps_deblocking_mode;                      /* unsigned int(2) */
  u8 cgps_max_bit_depth;                        /* unsigned int(3) */
  u8 cgps_allow_cross_channel_pred_flag;        /* unsigned int(1) */
  u8 cgps_cc_pred_filtering_mode;               /* unsigned int(2) */
  u8 cgps_allow_cc_pred_mult_hyp_flag;          /* unsigned int(1) */
  u8 cgps_allow_block_matching_pred_flag;       /* unsigned int(1) */
  u8 cgps_bm_pred_filtering_mode;               /* unsigned int(2) */
  u8 cgps_allow_bm_pred_mult_hyp_flag;          /* unsigned int(1) */
  u8 cgps_allow_bm_offset_pred_prev_ch_flag;    /* unsigned int(1) */
  u8 cgps_allow_dc_pred_flag;                   /* unsigned int(1) */
  u8 cgps_allow_line_fit_flag;                  /* unsigned int(1) */
  u8 cgps_allow_lpf_flag;                       /* unsigned int(1) */
  u8 cgps_allow_sample_pred_fixed_weights_flag; /* unsigned int(1) */
  u8 cgps_lpf_allow_prev_ch_flag;               /* unsigned int(1) */
  u8 cgps_allow_transform_skip_flag;            /* unsigned int(1) */
  u8 cgps_residual_quant_mode;                  /* unsigned int(2) */
  u8 cgps_ch_indep_interval_idx;                /* unsigned int(4) */
  u8 cgps_max_abs_delta_qp_idx;                 /* unsigned int(3) */
  u8 cgps_qp;                                   /* unsigned int(10) */
  u8 cgps_allow_verbatim_coding_flag;           /* unsigned int(1) */
  u8 cgps_allow_zero_lsb_flag;                  /* unsigned int(1) */
  u8 cgps_allow_lms_flag;                       /* unsigned int(1) */
  u8 cgps_allow_lms_split_flag;                 /* unsigned int(1) */
  u8 cgps_lms_ar_order_over_four;               /* unsigned int(4) */
  u8 cgps_allow_cc_lms_flag;                    /* unsigned int(1) */
  u8 cgps_max_order_cc_lms_minus1;              /* unsigned int(6) */
  u8 cgps_ctx_init_flag;                        /* unsigned int(1) */

  u32 depChMask;
  u32 maxAbsDeltaQP;
} bgw_channel_group_parameter_set;

typedef struct
{
  u32 cs_signal_type;                     /* ev(3,8,8) */
  u32 cs_signal_num_annotation_channels;  /* ue(v) */
  u32* cs_signal_annotation_channel_id;   /* ue(v) */
} bgw_configuration_set_wps_signal;

typedef struct 
{
  u8 cs_has_range_info_flag;               /* unsigned int(1) */
  s32 cs_digital_min;                      /* se(v) */
  s32 cs_digital_max;                      /* se(v) */
  s32 cs_analogue_min;                     /* se(v) */
  s32 cs_analogue_max;                     /* se(v) */
  s32 cs_analogue_units;                   /* st(v) */
  u8 cs_recording_start_time_flag;         /* unsigned int(1) */
} bgw_configuration_set_signal_info_data;

typedef struct
{
  u8 cs_features_in_cg_id_flag;             /* unsigned int(1) */
  u32 cs_num_features_in_cg_id;             /* ue(v) */
  u8 cs_enable_high_res_quality_metrics_flag;  /* unsigned int(1) */
  u8 cs_has_custom_quality_lut_flag;           /* unsigned int(1) */
  u32 cs_quality_metric;                        /* unsigned int(3) or st(v) */
  u32 cs_quality_num_bands;                     /* ue(v) */
  u8* cs_quality_lut;                           /* unsigned int(7) */
  u8 cs_uses_quality_threshold_flag;            /* unsigned int(1) */
  u8 cs_has_custom_quality_threshold_flag;      /* unsigned int(1) */
  u8 cs_quality_threshold;                      /* unsigned int(7) */
} bgw_config_extension_data_wps_channel_group;

typedef struct
{
  u8 cs_features_in_wps_flag;               /* unsigned int(1) */
  bgw_config_extension_data_wps_channel_group* cs_config_extension_data_wps_channel_group;
  u8 cs_segment_payload_frames_in_wps;      /* unsigned int(2) */
  u8* cs_segment_payload_frames_in_cg_id_flag; /* unsigned int(1) */
} bgw_config_extension_data_wps;

typedef struct {
  bgw_config_extension_data_wps* cs_extension_data_wps;
} bgw_config_extension_data;

typedef struct 
{
  u8 cs_wps_id;                                                       /* unsigned int(4) */
  u8 cs_label_mapping_flag;                                           /* unsigned int(1) */
  u64 cs_initial_wps_id_label;                                        /* ev(2,8,32) */
  u64 cs_target_zero_packet_label;                                    /* ev(2,8,32) */
  u32 cs_num_channel_groups_in_wps;                                   /* ue(v) */
  u8 cs_num_sampling_freqs_in_wps;                                    /* unsigned int(4) */
  u32* cs_sampling_freq;                                              /* ue(v) */
  u32* cs_sampling_freq_idx;                                          /* u(v) */
  u8 cs_num_signal_types_in_wps;                                      /* unsigned int(4) */
  bgw_configuration_set_wps_signal* cs_signal_info;
  u32* cs_signal_type_idx;                                            /* u(v) */
  u8 cs_signal_info_data_flag_in_wps_flag;                            /* unsigned int(1) */
  bgw_configuration_set_signal_info_data* cs_signal_info_data;
} bgw_configuration_set_wps;

typedef struct 
{
  u8 cs_num_wps_ids;                          /* unsigned int(4) */
  bgw_configuration_set_wps** cs_wps;
  u8 cs_config_extension_flag;                /* unsigned int(1) */
} bgw_configuration_set;

typedef struct 
{
  u32 am_header_crc32;                    /* unsigned int(32) */
  u8 am_extension_present_flag;           /* unsigned int(1) */
  u8 am_waveform_type;                    /* unsigned int(2) */
  u8 am_length_signal_mode;               /* unsigned int(1) */
  u8 am_emphasis_flag;                    /* unsigned int(1) */
  u8 am_copyright_flag;                   /* unsigned int(1) */
  u8 am_original_flag;                    /* unsigned int(1) */
  u8 am_private_flag;                     /* unsigned int(1) */
  u32 am_stream_max_sampling_rate_minus1; /* unsigned int(24) */
  u16 am_stream_max_num_channels_minus1;  /* unsigned int(16) */
  u32 am_stream_num_samples_per_ch;       /* unsigned int(32) */
  u8 am_metadata_reserved_flag;           /* unsigned int(1) */
  u32 am_metadata_num_bytes_minus1;       /* unsigned int(31) */
  u8* am_metadata_payload_bytes;          /* unsigned int(8) */
  u8 am_signal_type;                      /* unsigned int(8) */
} bgw_auxiliary_metadata;

typedef struct 
{
  u8 if_channel_group_parameter_set_id;         /* unsigned int(8) */
  u32 if_channel_group_id;                      /* u(v) */
  u16* if_mean_per_channel;                     /* unsigned int(16) */
  u32 if_indep_num_samples_per_channel_minus1;  /* unsigned int(32) */
  u8 if_ctx_init_mode_flag;                     /* unsigned int(1) */
} bgw_independent_frame;

typedef struct
{
  u32 df_channel_group_id;   /* u(v) */
} bgw_dependent_frame;

typedef struct 
{
  u32 channelGroupId;
  bgw_independent_frame* independentFrame;
  u32 numDependentFrames;
  bgw_dependent_frame** dependentFrames;
} bgw_frame_sequence;

typedef struct 
{
  bgw_waveform_parameter_set* wps;
  u8 packet_type;
  u32 packet_length;
  u8* packet_data;
} bgw_wps_packet_info;

typedef struct 
{
  bgw_channel_group_parameter_set* cgps;
  u8 packet_type;
  u32 packet_length;
  u8* packet_data;
} bgw_cgps_packet_info;

typedef struct 
{
  bgw_configuration_set* cs;
  u8 packet_type;
  u32 packet_length;
  u8* packet_data;
} bgw_cs_packet_info;

typedef struct 
{
  u32 numWPS;
  bgw_wps_packet_info** waveformParameterSetList;
  u32 numCGPS;
  bgw_cgps_packet_info** channelGroupParameterSetList;
  u32 numCS;
  bgw_cs_packet_info** configurationSetList;
  bgw_auxiliary_metadata* auxiliaryMetadata;
  u32 numFrameSequences;
  bgw_frame_sequence** frameSequences;

  u32 numStreamPackets;
  bgw_stream_packet** stream_packets;
  u32 numAllocatedStreamPackets;

  u32 numSubstreams;
  u32* streamPacketLabels;
} bgw_stream;

typedef struct 
{
	char *input;
	char *output;
  u8 profile_level_idc;
  u8 normative_encoder_flag;
  u8 substream_present_flag;
  u8 cg_info_present_flag;
  u32 sample_duration;
} bgw_params;

#endif
