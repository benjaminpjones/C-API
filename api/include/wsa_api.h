#ifndef __WSA_API_H__
#define __WSA_API_H__


#include "wsa_lib.h"


// ////////////////////////////////////////////////////////////////////////////
// WSA RELATED FUNCTIONS                                                     //
// ////////////////////////////////////////////////////////////////////////////

int16_t wsa_open(struct wsa_device *dev, char *intf_method);
void wsa_close(struct wsa_device *dev);

int16_t wsa_check_addr(char *ip_addr);
int16_t wsa_check_addrandport(char *ip_addr, char *port);
int16_t wsa_is_connected(struct wsa_device *dev);
const char *wsa_get_err_msg(int16_t err_code);

int16_t wsa_set_command_file(struct wsa_device *dev, char *file_name);


// ////////////////////////////////////////////////////////////////////////////
// AMPLITUDE SECTION                                                         //
// ////////////////////////////////////////////////////////////////////////////

int16_t wsa_get_abs_max_amp(struct wsa_device *dev, enum wsa_gain gain, 
						  float *value);


// ////////////////////////////////////////////////////////////////////////////
// DATA ACQUISITION SECTION                                                  //
// ////////////////////////////////////////////////////////////////////////////

int16_t wsa_capture_block(struct wsa_device* const device);
int16_t wsa_read_iq_packet (struct wsa_device* const device, 
		struct wsa_vrt_packet_header* const header, 
		struct wsa_vrt_packet_trailer* const trailer,
		struct wsa_reciever_packet* const reciever,
		struct wsa_digitizer_packet* const digitizer,
		int16_t* const i_buffer, 
		int16_t* const q_buffer,
		const uint16_t samples_per_packet, uint8_t* context_is);

int16_t wsa_read_iq_packet_matlab (struct wsa_device* const device, 
		struct wsa_vrt_packet_header* const header, 
		struct wsa_vrt_packet_trailer* const trailer,
		int16_t* const i_buffer, 
		int16_t* const q_buffer,
		const uint16_t samples_per_packet, uint8_t* context_is,
	int32_t* rec_indicator_field, int32_t* rec_reference_point, int64_t* rec_frequency, int16_t* rec_gain_if, int16_t* rec_gain_rf,	int32_t* rec_temperature,
	int32_t* dig_indicator_field, int64_t* dig_bandwidth, int32_t* dig_reference_level, int64_t* dig_rf_frequency_offset);


int16_t wsa_get_samples_per_packet(struct wsa_device* device, uint16_t* samples_per_packet);
int16_t wsa_set_samples_per_packet(struct wsa_device *dev, uint16_t samples_per_packet);
int16_t wsa_get_packets_per_block(struct wsa_device* device, uint32_t* packets_per_block);
int16_t wsa_set_packets_per_block(struct wsa_device *dev, uint32_t packets_per_block);

int16_t wsa_get_decimation(struct wsa_device *dev, int32_t *rate);
int16_t wsa_set_decimation(struct wsa_device *dev, int32_t rate);


// ////////////////////////////////////////////////////////////////////////////
// FREQUENCY SECTION                                                         //
// ////////////////////////////////////////////////////////////////////////////

int16_t wsa_get_freq(struct wsa_device *dev, int64_t *cfreq);
int16_t wsa_set_freq(struct wsa_device *dev, int64_t cfreq);

int16_t wsa_get_freq_shift(struct wsa_device *dev, float *fshift);
int16_t wsa_set_freq_shift(struct wsa_device *dev, float fshift);


// ////////////////////////////////////////////////////////////////////////////
// GAIN SECTION                                                              //
// ////////////////////////////////////////////////////////////////////////////

int16_t wsa_get_gain_if(struct wsa_device *dev, int32_t *gain);
int16_t wsa_set_gain_if(struct wsa_device *dev, int32_t gain);

int16_t wsa_get_gain_rf(struct wsa_device *dev, enum wsa_gain *gain);
int16_t wsa_set_gain_rf(struct wsa_device *dev, enum wsa_gain gain);


// ////////////////////////////////////////////////////////////////////////////
// RFE CONTROL SECTION                                                       //
// ////////////////////////////////////////////////////////////////////////////

int16_t wsa_get_antenna(struct wsa_device *dev, int32_t *port_num);
int16_t wsa_set_antenna(struct wsa_device *dev, int32_t port_num);

int16_t wsa_get_bpf_mode(struct wsa_device *dev, int32_t *mode);
int16_t wsa_set_bpf_mode(struct wsa_device *dev, int32_t mode);


// ////////////////////////////////////////////////////////////////////////////
// DEVICE STATUS SECTION                                                     //
// ////////////////////////////////////////////////////////////////////////////

int16_t wsa_get_firm_v(struct wsa_device *dev);

// ////////////////////////////////////////////////////////////////////////////
// TRIGGER SECTION                                                           //
// ////////////////////////////////////////////////////////////////////////////

int16_t wsa_set_trigger_level(struct wsa_device* dev, int64_t start_frequency, int64_t stop_frequency, int64_t amplitude);
int16_t wsa_get_trigger_level(struct wsa_device* dev, int64_t* start_frequency, int64_t* stop_frequency, int64_t* amplitude);
int16_t wsa_set_trigger_enable(struct wsa_device* dev, int32_t enable);
int16_t wsa_get_trigger_enable(struct wsa_device* dev, int32_t* enable);


// ////////////////////////////////////////////////////////////////////////////
// CONTEXT PACKET SECTION                                                          //
// ////////////////////////////////////////////////////////////////////////////

int16_t wsa_get_context_digitizer(struct wsa_device* dev, int32_t packets_per_block, int32_t samples_per_packet);


#endif

