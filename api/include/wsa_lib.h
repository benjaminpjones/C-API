#ifndef __WSA_LIB_H__
#define __WSA_LIB_H__

#include "wsa_commons.h"


#define MAX_STR_LEN 512

#define NUM_RF_GAINS 5	// including 0 but not use
#define MHZ 1000000ULL

#define VRT_HEADER_SIZE 5
#define VRT_TRAILER_SIZE 1


// *****
// SCPI related registers/bits
// *****

// Status Byte Register SBR, use with SRE
#define SCPI_SBR_EVTAVL 0x04	// Error/event queue available
#define SCPI_SBR_QSR 0x08		// Questionable Status register
#define SCPI_SBR_MSGAVL 0x10	// Message available
#define SCPI_SBR_ESR 0x20		// Event Status register
#define SCPI_SBR_RQS 0x40		// Request Service register
#define SCPI_SBR_OSR 0x80		// Operational Status register


// Standard Event Status Register ESR, use with ESE
#define SCPI_ESR_OPC 0x00	// Operation complete
#define SCPI_ESR_QYE 0x04	// Query error
#define SCPI_ESR_DDE 0x08	// Device dependent error
#define SCPI_ESR_EXE 0x10	// Execution error
#define SCPI_ESR_CME 0x20	// Command error
#define SCPI_ESR_PON 0x80	// Power ON


// Operation Status Register OSR
#define SCPI_OSR_CALI 0x0001	// RFE Calibrating
#define SCPI_OSR_SETT 0x0002	// Settling
#define SCPI_OSR_SWE  0x0008	// Sweeping
//#define SCPI_OSR_MEAS 0x0010	// Measuring?
#define SCPI_OSR_TRIG 0x0020	// Triggering
#define SCPI_OSR_CORR 0x0080	//Correcting which? IQ or DC ?


// Questionable Status Register QSR
#define SCPI_QSR_POW 0x0002		// Battery power ?
#define SCPI_QSR_TEMP 0x0010	// Temperature ?
#define SCPI_QSR_FREQ 0x0020	// Frequency unlocked ?
#define SCPI_QSR_PHAS 0x0040	// IQ out of phase ?
#define SCPI_QSR_CALI 0x0100	//RFE Calibration ?


//*****
// WSA specific values
//*****
#define WSA4000 "WSA4000"
#define WSA4000_INST_BW 125000000ULL
#define WSA4000_MIN_SAMPLE_SIZE 128
#define WSA4000_MAX_SAMPLE_SIZE (2560 * 1024) //32768 // RANDOM NUMBER FOR NOW. CHECK VRT & DDR SIZE.


// *****
// RFE0440 SPECIFIC
// *****
#define WSA_RFE0440 "RFE0440"
#define WSA_RFE0440_MAX_FREQ 4000000000ULL
#define WSA_RFE0440_MIN_FREQ 200000000ULL
#define WSA_RFE0440_FREQRES	10000ULL
#define WSA_RFE0440_ABS_AMP_HIGH -15
#define WSA_RFE0440_ABS_AMP_MED 0
#define WSA_RFE0440_ABS_AMP_LOW 13
#define WSA_RFE0440_ABS_AMP_VLOW 20


// *****
// RFE0560 SPECIFIC
// *****
#define WSA_RFE0560 "RFE0560"
#define WSA_RFE0560_MAX_FREQ 11000ULL // MHz here b/c of large # issue
#define WSA_RFE0560_MIN_FREQ 100000ULL  // Hz
#define WSA_RFE0560_MAX_IF_GAIN 34
#define WSA_RFE0560_MIN_IF_GAIN -10
#define WSA_RFE0560_MAX_DECIMATION 1023
#define WSA_RFE0560_MIN_DECIMATION 16
#define WSA_RFE0560_FREQRES	100000ULL // to read in the register
#define WSA_RFE0560_MAX_ANT_PORT 2


// TODO: TO BE DETERMINE W/ NIKHIL FOR THE FOLLOWING #S -> Read from eeprom
#define WSA_RFE0560_ABS_AMP_HIGH -15
#define WSA_RFE0560_ABS_AMP_MED 0
#define WSA_RFE0560_ABS_AMP_LOW 13
#define WSA_RFE0560_ABS_AMP_VLOW 20

// Control commands syntax supported types
#define SCPI "SCPI"	/* SCPI control commands syntax */


enum wsa_gain {
	WSA_GAIN_HIGH = 1,
	WSA_GAIN_MED,
	WSA_GAIN_LOW,
	WSA_GAIN_VLOW
};

// ////////////////////////////////////////////////////////////////////////////
// STRUCTS DEFINES                                                           //
// ////////////////////////////////////////////////////////////////////////////

struct wsa_descriptor {
	char prod_name[50];
	char prod_serial[20];
	char prod_version[20];
	char rfe_name[50];
	char rfe_version[20];
	char fw_version[20];
	char intf_type[20];
	uint64_t inst_bw;
	uint32_t max_sample_size;
	uint64_t max_tune_freq;
	uint64_t min_tune_freq;
	uint64_t freq_resolution;
	int32_t max_if_gain;
	int32_t min_if_gain;
	int32_t min_decimation;
	int32_t max_decimation;
	float abs_max_amp[NUM_RF_GAINS];
};


struct wsa_time {
	uint32_t sec;
	uint64_t psec;
};


// Temporary define for now. Will need to be expanded for more details...
struct wsa_frame_header {
	uint32_t sample_size; 
	struct wsa_time time_stamp;
};


struct wsa_socket {
	int32_t cmd;
	int32_t data;
};


struct wsa_device {
	struct wsa_descriptor descr;
	struct wsa_socket sock;
	//struct wsa_trig *trig_list; 
};


struct wsa_resp {
	int64_t status;
	char output[MAX_STR_LEN];
};


// ////////////////////////////////////////////////////////////////////////////
// List of functions                                                         //
// ////////////////////////////////////////////////////////////////////////////
int16_t wsa_connect(struct wsa_device *dev, char *cmd_syntax, 
					char *intf_method);
int16_t wsa_disconnect(struct wsa_device *dev);

int16_t wsa_verify_addr(const char *sock_addr, const char *sock_port);

int16_t wsa_send_command(struct wsa_device *dev, char *command);
int16_t wsa_send_command_file(struct wsa_device *dev, char *file_name);
int16_t wsa_send_query(struct wsa_device *dev, char *command, 
						struct wsa_resp *resp);
int16_t wsa_read_frame(struct wsa_device *dev, struct wsa_frame_header *header, 
				 char *data_buf, uint32_t sample_size, uint32_t time_out);
int32_t wsa_decode_frame(char *data_buf, int16_t *i_buf, int16_t *q_buf, 
						 uint32_t sample_size);
int16_t wsa_read_status(struct wsa_device *dev, char *output);
const char *wsa_get_error_msg(int16_t err_code);

#endif