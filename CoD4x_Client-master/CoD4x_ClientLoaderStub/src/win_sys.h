/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//c5f900 clc.state


#ifndef __WIN_SYS_H__
#define __WIN_SYS_H__

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "q_shared.h"



typedef struct{
	void (__stdcall	*AIL_set_DirectSound_HWND_int)(int, HWND);
}MilesSoundSystem_t;

extern MilesSoundSystem_t mss;

typedef enum
{
	AIL_debug_printf_enum,
	AIL_sprintf_enum,
	DLSClose_enum,
	DLSCompactMemory_enum,
	DLSGetInfo_enum,
	DLSLoadFile_enum,
	DLSLoadMemFile_enum,
	DLSMSSOpen_enum,
	DLSSetAttribute_enum,
	DLSUnloadAll_enum,
	DLSUnloadFile_enum,
	RIB_alloc_provider_handle_enum,
	RIB_enumerate_interface_enum,
	RIB_error_enum,
	RIB_find_file_provider_enum,
	RIB_free_provider_handle_enum,
	RIB_free_provider_library_enum,
	RIB_load_provider_library_enum,
	RIB_register_interface_enum,
	RIB_request_interface_enum,
	RIB_request_interface_entry_enum,
	RIB_type_string_enum,
	RIB_unregister_interface_enum,
	_AIL_3D_distance_factor_4_enum,
	_AIL_3D_doppler_factor_4_enum,
	_AIL_3D_rolloff_factor_4_enum,
	_AIL_DLS_close_8_enum,
	_AIL_DLS_compact_4_enum,
	_AIL_DLS_get_info_12_enum,
	_AIL_DLS_load_file_12_enum,
	_AIL_DLS_load_memory_12_enum,
	_AIL_DLS_open_28_enum,
	_AIL_DLS_sample_handle_4_enum,
	_AIL_DLS_unload_8_enum,
	_AIL_HWND_0_enum,
	_AIL_MIDI_handle_reacquire_4_enum,
	_AIL_MIDI_handle_release_4_enum,
	_AIL_MIDI_to_XMI_20_enum,
	_AIL_MMX_available_0_enum,
	_AIL_WAV_file_write_20_enum,
	_AIL_WAV_info_8_enum,
	_AIL_XMIDI_master_volume_4_enum,
	_AIL_active_sample_count_4_enum,
	_AIL_active_sequence_count_4_enum,
	_AIL_allocate_sample_handle_4_enum,
	_AIL_allocate_sequence_handle_4_enum,
	_AIL_auto_service_stream_8_enum,
	_AIL_background_0_enum,
	_AIL_background_CPU_percent_0_enum,
	_AIL_branch_index_8_enum,
	_AIL_calculate_3D_channel_levels_56_enum,
	_AIL_channel_notes_8_enum,
	_AIL_close_XMIDI_driver_4_enum,
	_AIL_close_digital_driver_4_enum,
	_AIL_close_filter_4_enum,
	_AIL_close_input_4_enum,
	_AIL_close_stream_4_enum,
	_AIL_compress_ADPCM_12_enum,
	_AIL_compress_ASI_20_enum,
	_AIL_compress_DLS_20_enum,
	_AIL_controller_value_12_enum,
	_AIL_create_wave_synthesizer_16_enum,
	_AIL_decompress_ADPCM_12_enum,
	_AIL_decompress_ASI_24_enum,
	_AIL_delay_4_enum,
	_AIL_destroy_wave_synthesizer_4_enum,
	_AIL_digital_CPU_percent_4_enum,
	_AIL_digital_configuration_16_enum,
	_AIL_digital_driver_processor_8_enum,
	_AIL_digital_handle_reacquire_4_enum,
	_AIL_digital_handle_release_4_enum,
	_AIL_digital_latency_4_enum,
	_AIL_digital_master_reverb_16_enum,
	_AIL_digital_master_reverb_levels_12_enum,
	_AIL_digital_master_volume_level_4_enum,
	_AIL_digital_output_filter_4_enum,
	_AIL_end_sample_4_enum,
	_AIL_end_sequence_4_enum,
	_AIL_enumerate_MP3_frames_4_enum,
	_AIL_enumerate_filter_properties_12_enum,
	_AIL_enumerate_filter_sample_properties_12_enum,
	_AIL_enumerate_filters_12_enum,
	_AIL_enumerate_output_filter_driver_properties_12_enum,
	_AIL_enumerate_output_filter_sample_properties_12_enum,
	_AIL_enumerate_sample_stage_properties_16_enum,
	_AIL_extract_DLS_28_enum,
	_AIL_file_error_0_enum,
	_AIL_file_read_8_enum,
	_AIL_file_size_4_enum,
	_AIL_file_type_8_enum,
	_AIL_file_type_named_12_enum,
	_AIL_file_write_12_enum,
	_AIL_filter_DLS_with_XMI_24_enum,
	_AIL_filter_property_20_enum,
	_AIL_find_DLS_24_enum,
	_AIL_find_filter_8_enum,
	_AIL_ftoa_4_enum,
	_AIL_get_DirectSound_info_12_enum,
	_AIL_get_input_info_4_enum,
	_AIL_get_preference_4_enum,
	_AIL_get_timer_highest_delay_0_enum,
	_AIL_init_sample_12_enum,
	_AIL_init_sequence_12_enum,
	_AIL_inspect_MP3_12_enum,
	_AIL_last_error_0_enum,
	_AIL_list_DLS_20_enum,
	_AIL_list_MIDI_20_enum,
	_AIL_listener_3D_orientation_28_enum,
	_AIL_listener_3D_position_16_enum,
	_AIL_listener_3D_velocity_16_enum,
	_AIL_listener_relative_receiver_array_8_enum,
	_AIL_load_sample_attributes_8_enum,
	_AIL_load_sample_buffer_16_enum,
	_AIL_lock_0_enum,
	_AIL_lock_channel_4_enum,
	_AIL_lock_mutex_0_enum,
	_AIL_map_sequence_channel_12_enum,
	_AIL_mem_alloc_lock_4_enum,
	_AIL_mem_free_lock_4_enum,
	_AIL_mem_use_free_4_enum,
	_AIL_mem_use_malloc_4_enum,
	_AIL_merge_DLS_with_XMI_16_enum,
	_AIL_midiOutClose_4_enum,
	_AIL_midiOutOpen_12_enum,
	_AIL_minimum_sample_buffer_size_12_enum,
	_AIL_ms_count_0_enum,
	_AIL_open_XMIDI_driver_4_enum,
	_AIL_open_digital_driver_16_enum,
	_AIL_open_filter_8_enum,
	_AIL_open_input_4_enum,
	_AIL_open_stream_12_enum,
	_AIL_output_filter_driver_property_20_enum,
	_AIL_pause_stream_8_enum,
	_AIL_platform_property_20_enum,
	_AIL_primary_digital_driver_4_enum,
	_AIL_process_digital_audio_24_enum,
	_AIL_quick_copy_4_enum,
	_AIL_quick_halt_4_enum,
	_AIL_quick_handles_12_enum,
	_AIL_quick_load_4_enum,
	_AIL_quick_load_and_play_12_enum,
	_AIL_quick_load_mem_8_enum,
	_AIL_quick_load_named_mem_12_enum,
	_AIL_quick_ms_length_4_enum,
	_AIL_quick_ms_position_4_enum,
	_AIL_quick_play_8_enum,
	_AIL_quick_set_low_pass_cut_off_8_enum,
	_AIL_quick_set_ms_position_8_enum,
	_AIL_quick_set_reverb_levels_12_enum,
	_AIL_quick_set_speed_8_enum,
	_AIL_quick_set_volume_12_enum,
	_AIL_quick_shutdown_0_enum,
	_AIL_quick_startup_20_enum,
	_AIL_quick_status_4_enum,
	_AIL_quick_type_4_enum,
	_AIL_quick_unload_4_enum,
	_AIL_redbook_close_4_enum,
	_AIL_redbook_eject_4_enum,
	_AIL_redbook_id_4_enum,
	_AIL_redbook_open_4_enum,
	_AIL_redbook_open_drive_4_enum,
	_AIL_redbook_pause_4_enum,
	_AIL_redbook_play_12_enum,
	_AIL_redbook_position_4_enum,
	_AIL_redbook_resume_4_enum,
	_AIL_redbook_retract_4_enum,
	_AIL_redbook_set_volume_level_8_enum,
	_AIL_redbook_status_4_enum,
	_AIL_redbook_stop_4_enum,
	_AIL_redbook_track_4_enum,
	_AIL_redbook_track_info_16_enum,
	_AIL_redbook_tracks_4_enum,
	_AIL_redbook_volume_level_4_enum,
	_AIL_register_EOB_callback_8_enum,
	_AIL_register_EOS_callback_8_enum,
	_AIL_register_ICA_array_8_enum,
	_AIL_register_SOB_callback_8_enum,
	_AIL_register_beat_callback_8_enum,
	_AIL_register_event_callback_8_enum,
	_AIL_register_falloff_function_callback_8_enum,
	_AIL_register_prefix_callback_8_enum,
	_AIL_register_sequence_callback_8_enum,
	_AIL_register_stream_callback_8_enum,
	_AIL_register_timbre_callback_8_enum,
	_AIL_register_timer_4_enum,
	_AIL_register_trace_callback_8_enum,
	_AIL_register_trigger_callback_8_enum,
	_AIL_release_all_timers_0_enum,
	_AIL_release_channel_8_enum,
	_AIL_release_sample_handle_4_enum,
	_AIL_release_sequence_handle_4_enum,
	_AIL_release_timer_handle_4_enum,
	_AIL_request_EOB_ASI_reset_12_enum,
	_AIL_resume_sample_4_enum,
	_AIL_resume_sequence_4_enum,
	_AIL_room_type_4_enum,
	_AIL_sample_3D_cone_16_enum,
	_AIL_sample_3D_distances_16_enum,
	_AIL_sample_3D_orientation_28_enum,
	_AIL_sample_3D_position_16_enum,
	_AIL_sample_3D_velocity_16_enum,
	_AIL_sample_51_volume_levels_28_enum,
	_AIL_sample_51_volume_pan_24_enum,
	_AIL_sample_buffer_info_20_enum,
	_AIL_sample_buffer_ready_4_enum,
	_AIL_sample_channel_levels_8_enum,
	_AIL_sample_exclusion_4_enum,
	_AIL_sample_granularity_4_enum,
	_AIL_sample_loop_block_12_enum,
	_AIL_sample_loop_count_4_enum,
	_AIL_sample_low_pass_cut_off_4_enum,
	_AIL_sample_ms_position_12_enum,
	_AIL_sample_obstruction_4_enum,
	_AIL_sample_occlusion_4_enum,
	_AIL_sample_playback_rate_4_enum,
	_AIL_sample_position_4_enum,
	_AIL_sample_processor_8_enum,
	_AIL_sample_reverb_levels_12_enum,
	_AIL_sample_stage_property_24_enum,
	_AIL_sample_status_4_enum,
	_AIL_sample_user_data_8_enum,
	_AIL_sample_volume_levels_12_enum,
	_AIL_sample_volume_pan_12_enum,
	_AIL_save_sample_attributes_8_enum,
	_AIL_send_channel_voice_message_20_enum,
	_AIL_send_sysex_message_8_enum,
	_AIL_sequence_loop_count_4_enum,
	_AIL_sequence_ms_position_12_enum,
	_AIL_sequence_position_12_enum,
	_AIL_sequence_status_4_enum,
	_AIL_sequence_tempo_4_enum,
	_AIL_sequence_user_data_8_enum,
	_AIL_sequence_volume_4_enum,
	_AIL_serve_0_enum,
	_AIL_service_stream_8_enum,
	_AIL_set_3D_distance_factor_8_enum,
	_AIL_set_3D_doppler_factor_8_enum,
	_AIL_set_3D_rolloff_factor_8_enum,
	_AIL_set_DirectSound_HWND_8_enum,
	_AIL_set_XMIDI_master_volume_8_enum,
	_AIL_set_digital_driver_processor_12_enum,
	_AIL_set_digital_master_reverb_16_enum,
	_AIL_set_digital_master_reverb_levels_12_enum,
	_AIL_set_digital_master_volume_level_8_enum,
	_AIL_set_error_4_enum,
	_AIL_set_file_async_callbacks_20_enum,
	_AIL_set_file_callbacks_16_enum,
	_AIL_set_input_state_8_enum,
	_AIL_set_listener_3D_orientation_28_enum,
	_AIL_set_listener_3D_position_16_enum,
	_AIL_set_listener_3D_velocity_20_enum,
	_AIL_set_listener_3D_velocity_vector_16_enum,
	_AIL_set_listener_relative_receiver_array_12_enum,
	_AIL_set_named_sample_file_20_enum,
	_AIL_set_preference_8_enum,
	_AIL_set_redist_directory_4_enum,
	_AIL_set_room_type_8_enum,
	_AIL_set_sample_3D_cone_16_enum,
	_AIL_set_sample_3D_distances_16_enum,
	_AIL_set_sample_3D_orientation_28_enum,
	_AIL_set_sample_3D_position_16_enum,
	_AIL_set_sample_3D_velocity_20_enum,
	_AIL_set_sample_3D_velocity_vector_16_enum,
	_AIL_set_sample_51_volume_levels_28_enum,
	_AIL_set_sample_51_volume_pan_24_enum,
	_AIL_set_sample_address_12_enum,
	_AIL_set_sample_adpcm_block_size_8_enum,
	_AIL_set_sample_channel_levels_12_enum,
	_AIL_set_sample_exclusion_8_enum,
	_AIL_set_sample_file_12_enum,
	_AIL_set_sample_info_8_enum,
	_AIL_set_sample_loop_block_12_enum,
	_AIL_set_sample_loop_count_8_enum,
	_AIL_set_sample_low_pass_cut_off_8_enum,
	_AIL_set_sample_ms_position_8_enum,
	_AIL_set_sample_obstruction_8_enum,
	_AIL_set_sample_occlusion_8_enum,
	_AIL_set_sample_playback_rate_8_enum,
	_AIL_set_sample_position_8_enum,
	_AIL_set_sample_processor_12_enum,
	_AIL_set_sample_reverb_levels_12_enum,
	_AIL_set_sample_user_data_12_enum,
	_AIL_set_sample_volume_levels_12_enum,
	_AIL_set_sample_volume_pan_12_enum,
	_AIL_set_sequence_loop_count_8_enum,
	_AIL_set_sequence_ms_position_8_enum,
	_AIL_set_sequence_tempo_12_enum,
	_AIL_set_sequence_user_data_12_enum,
	_AIL_set_sequence_volume_12_enum,
	_AIL_set_speaker_configuration_16_enum,
	_AIL_set_speaker_reverb_levels_16_enum,
	_AIL_set_stream_loop_block_12_enum,
	_AIL_set_stream_loop_count_8_enum,
	_AIL_set_stream_ms_position_8_enum,
	_AIL_set_stream_position_8_enum,
	_AIL_set_stream_user_data_12_enum,
	_AIL_set_timer_divisor_8_enum,
	_AIL_set_timer_frequency_8_enum,
	_AIL_set_timer_period_8_enum,
	_AIL_set_timer_user_8_enum,
	_AIL_shutdown_0_enum,
	_AIL_size_processed_digital_audio_16_enum,
	_AIL_speaker_configuration_20_enum,
	_AIL_speaker_reverb_levels_16_enum,
	_AIL_start_all_timers_0_enum,
	_AIL_start_sample_4_enum,
	_AIL_start_sequence_4_enum,
	_AIL_start_stream_4_enum,
	_AIL_start_timer_4_enum,
	_AIL_startup_0_enum,
	_AIL_stop_all_timers_0_enum,
	_AIL_stop_sample_4_enum,
	_AIL_stop_sequence_4_enum,
	_AIL_stop_timer_4_enum,
	_AIL_stream_info_20_enum,
	_AIL_stream_loop_count_4_enum,
	_AIL_stream_ms_position_12_enum,
	_AIL_stream_position_4_enum,
	_AIL_stream_sample_handle_4_enum,
	_AIL_stream_status_4_enum,
	_AIL_stream_user_data_8_enum,
	_AIL_true_sequence_channel_8_enum,
	_AIL_unlock_0_enum,
	_AIL_unlock_mutex_0_enum,
	_AIL_update_listener_3D_position_8_enum,
	_AIL_update_sample_3D_position_8_enum,
	_AIL_us_count_0_enum,
	_DLSMSSGetCPU_4_enum,
	_MIX_RIB_MAIN_8_enum,
	_MSSDisableThreadLibraryCalls_4_enum,
	_RIB_enumerate_providers_12_enum,
	_RIB_find_file_dec_provider_20_enum,
	_RIB_find_files_provider_20_enum,
	_RIB_find_provider_12_enum,
	_RIB_load_application_providers_4_enum,
	_RIB_load_static_provider_library_8_enum,
	_RIB_provider_system_data_8_enum,
	_RIB_provider_user_data_8_enum,
	_RIB_set_provider_system_data_12_enum,
	_RIB_set_provider_user_data_12_enum,
	AIL_TOP_COUNT
}mss32index_t;

extern void* mss32importprocs[AIL_TOP_COUNT];

typedef DWORD threadid_t;


#endif
