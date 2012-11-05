/**
 * @mainpage Introduction
 *
 * This documentation describes in details the wsa_api library.  The wsa_api 
 * provides functions to set/get particular settings or to acquire 
 * data from the WSA.  \n
 * The wsa_api encodes the commands into SCPI syntax scripts, which 
 * are sent to a WSA through the wsa_lib library.  Subsequently, it decodes 
 * any responses or packets coming back from the WSA through the wsa_lib.
 * Thus, the API abstracts away the control protocol (such as SCPI) from 
 * the user.
 *
 * Data frames passing back from the wsa_lib are in VRT format.  This 
 * API will extract the information and the actual data frames within
 * the VRT packet and makes them available in structures and buffers 
 * for users.
 *
 *
 * @section limitation Limitations in Release v1.1
 * The following features are not yet supported with the CLI:
 *  - VRT trailer extraction. Bit fields are yet to be defined.
 *  - Data streaming. Currently only supports block mode.
 *  - DC correction.  
 *  - IQ correction.  
 *  - Automatic finding of a WSA box(s) on a network.
 *  - Triggers.
 *  - Gain calibrarion. TBD with triggers.
 *  - USB interface method.
 *
 * @section usage How to use the library
 * The wsa_api is designed using mixed C/C++ languages.  To use the 
 * library, you need to include the header file, wsa_api.h, in files that 
 * will use any of its functions to access a WSA, and a link to 
 * the wsa_api.lib.  \b wsa_api also depends on the others *.h files provided
 * in the \b include folder  
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "thinkrf_stdint.h"
#include "wsa_error.h"
#include "wsa_commons.h"
#include "wsa_lib.h"
#include "wsa_api.h"
#include "wsa_client.h"


#define MAX_RETRIES_READ_FRAME 5



// ////////////////////////////////////////////////////////////////////////////
// Local functions                                                           //
// ////////////////////////////////////////////////////////////////////////////
int16_t wsa_verify_freq(struct wsa_device *dev, uint64_t freq);
enum wsa_gain gain_rf_strtonum(char *str);


// Verify if the frequency is valid (within allowed range)
int16_t wsa_verify_freq(struct wsa_device *dev, uint64_t freq)
{
	// verify the frequency value
	if (freq < dev->descr.min_tune_freq || freq > dev->descr.max_tune_freq)
		return WSA_ERR_FREQOUTOFBOUND;

	return 0;
}

// Convert gain RF string to an enum type
enum wsa_gain gain_rf_strtonum(char *str)
{
	enum wsa_gain gain;
	
	if (strstr(str, "HIGH") != NULL)
		gain = WSA_GAIN_HIGH;
	else if (strstr(str, "MED") != NULL)
		gain = WSA_GAIN_MED;
	else if (strstr(str, "VLOW") != NULL)
		gain = WSA_GAIN_VLOW;
	else if (strstr(str, "LOW") != NULL)
		gain = WSA_GAIN_LOW;
	else
		gain = (enum wsa_gain) NULL;

	return gain;
}


// ////////////////////////////////////////////////////////////////////////////
// WSA RELATED FUNCTIONS                                                     //
// ////////////////////////////////////////////////////////////////////////////

/**
 * Establishes a connection of choice specified by the interface method to 
 * the WSA.\n At success, the handle remains open for future access by other 
 * library methods until wsa_close() is called. When unsuccessful, the WSA 
 * will be closed automatically and an error is returned.
 *
 * @param dev - A pointer to the WSA device structure to be opened.
 * @param intf_method - A char pointer to store the interface method to the 
 * WSA. \n Possible methods: \n
 * - With LAN, use: "TCPIP::<Ip address of the WSA>::37001" \n
 * - With USB, use: "USB" (check if supported with the WSA version used). \n
 *
 * @return 0 on success, or a negative number on error.
 * @par Errors:
 * Situations that will generate an error are:
 * - the interface method does not exist for the WSA product version.
 * - the WSA is not detected (has not been connected or powered up).
 * -
 */
int16_t wsa_open(struct wsa_device *dev, char *intf_method)
{
	int16_t result = 0;		// result returned from a function

	// Start the WSA connection
	// NOTE: API will always assume SCPI syntax
	result = wsa_connect(dev, SCPI, intf_method);

	return result;
}


/**
 * Closes the device handle if one is opened and stops any existing data 
 * capture.
 *
 * @param dev - A pointer to a WSA device structure to be closed.
 *
 * @return none
 */
void wsa_close(struct wsa_device *dev)
{
	wsa_disconnect(dev);
}


/**
 * Verify if the given IP address or host name is valid for the WSA.  Default 
 * ports used are 37001 and 37000 for command and data ports, respectively.
 * 
 * @param ip_addr - A char pointer to the IP address or host name to be 
 * verified.
 * 
 * @return 0 if the IP is valid, or a negative number on error.
 */
int16_t wsa_check_addr(char *ip_addr) 
{
	int16_t result = 0;

	// Check with command port
	result = wsa_verify_addr(ip_addr, "37001");  //TODO make this dynamic
	if (result < 0)
		return result;

	// check with data port
	result = wsa_verify_addr(ip_addr, "37000");
	if (result < 0)
		return result;

	return 0;
}

/**
 * Verify if the given IP address or host name at the given port is valid for 
 * the WSA.
 * 
 * @param ip_addr - A char pointer to the IP address or host name to be 
 * verified.
 * @param port - A char pointer to the port to
 * 
 * @return 0 if the IP is valid, or a negative number on error.
 */
int16_t wsa_check_addrandport(char *ip_addr, char *port) 
{
	return wsa_verify_addr(ip_addr, port);
}



/**
 * Returns a message string associated with the given error code \b err_code.
 * 
 * @param err_code - The negative WSA error code, returned from a WSA function.
 *
 * @return A char pointer to the error message string.
 */
const char *wsa_get_err_msg(int16_t err_code)
{
	return wsa_get_error_msg(err_code);
}



/**
 * Read command line(s) stored in the given \b file_name and send each line
 * to the WSA.
 *
 * @remarks 
 * - Assuming each command line is for a single function followed by
 * a new line.
 * - Currently read only SCPI commands. Other types of commands, TBD.
 *
 * @param dev - A pointer to the WSA device structure.
 * @param file_name - A pointer to the file name
 *
 * @return Number of command lines at success, or a negative error number.
 */
int16_t wsa_set_command_file(struct wsa_device *dev, char *file_name)
{
	return wsa_send_command_file(dev, file_name);
}


// ////////////////////////////////////////////////////////////////////////////
// AMPLITUDE SECTION                                                         //
// ////////////////////////////////////////////////////////////////////////////

/**
 * Gets the absolute maximum RF input level (dBm) for the WSA at 
 * the given gain setting.\n
 * Operating the WSA device at the absolute maximum may cause damage to the 
 * device.
 *
 * @param dev - A pointer to the WSA device structure.
 * @param gain - The gain setting of \b wsa_gain type at which the absolute 
 * maximum amplitude input level is to be retrieved.
 * @param value - A float pointer to store the absolute maximum RF input 
 * level in dBm for the given RF gain
 *
 * @return 0 on successful or negative error number.
 */
int16_t wsa_get_abs_max_amp(struct wsa_device *dev, enum wsa_gain gain, 
						  float *value)
{
	// TODO Check version of WSA & return the correct info here
	if (gain < WSA_GAIN_VLOW || gain > WSA_GAIN_HIGH) {		
		return WSA_ERR_INVRFGAIN;
	}
	else {
		*value = dev->descr.abs_max_amp[gain];
	}

	return 0;
}



// ////////////////////////////////////////////////////////////////////////////
// DATA ACQUISITION SECTION                                                  //
// ////////////////////////////////////////////////////////////////////////////

/**
 * Request read data access from the wsa
 * @param dev - A pointer to the WSA device structure.
 * @param status - returns if the read access was acquired (1 if access granted, 0 if access is denied)
 *@return 0 on success, or a negative number on error.
 */
int16_t wsa_system_request_acquisition_access(struct wsa_device *dev, int16_t* status)
{
	struct wsa_resp query;		// store query results

	wsa_send_query(dev, "SYSTem:LOCK:REQuest? ACQuisition\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	if (strcmp(query.output, "1") == 0)
		*status = 1;
	else if (strcmp(query.output, "0") == 0)
		*status = 0;

	return 0;

}

/**
 * Determine if read data access has been claimed by current connection
 * @param dev - A pointer to the WSA device structure.
 * @param status - returns if the read access is claimed  (1 if current connection has access, 0 if current connection does not have access)
 *@return 0 on success, or a negative number on error.
 */
int16_t wsa_system_read_status(struct wsa_device *dev, int16_t* status) 
{
	struct wsa_resp query;		// store query results

	wsa_send_query(dev, ":SYSTem:LOCK:HAVE? ACQuisition\n", &query);	
	if (query.status <= 0)
		return (int16_t) query.status;

	if (strcmp(query.output, "1") == 0)
		*status = 1;
	else if (strcmp(query.output, "0") == 0)
		*status = 0;

	return 0;
}

/**
 * Instruct the WSA to capture a block of signal data
 * and store it in internal memory.
 * \n
 * Before calling this method, set the size of the block
 * using the methods \b wsa_set_samples_per_packet
 * and \b wsa_set_packets_per_block
 * \n
 * After this method returns, read the data using
 * the method \b wsa_read_vrt_packet.
 *
 * @param device - A pointer to the WSA device structure.
 *
 * @return  0 on success or a negative value on error
 */
int16_t wsa_capture_block(struct wsa_device* const device)
{
	int16_t result = 0;

	result = wsa_send_command(device, "TRACE:BLOCK:DATA?\n");
	doutf(DHIGH, "Error in wsa_capture_block: %d - %s\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


/**
 * Reads one VRT packet containing raw IQ data. 
 * Each packet consists of a header, a data payload, and a trailer.
 * The number of samples expected in the packet is indicated by
 * the \b samples_per_packet parameter.
 *
 * Note that to read a complete capture block, you must call this
 * method as many times as you set using the method \b wsa_set_packets_per_block
 *
 * Each I and Q sample is a 16-bit (2-byte) signed 2-complement integer.
 * The \b i_buffer and \b q_buffer pointers will be populated with
 * the decoded IQ payload data.
 *
 * For example, if the VRT packet contains the payload
 * @code 
 *		I1 Q1 I2 Q2 I3 Q3 I4 Q4 ... = <2 bytes I><2 bytes Q><...>
 * @endcode
 *
 * then \b i_buffer and \b q_buffer will contain:
 * @code 
 *		i_buffer[0] = I1
 *		i_buffer[1] = I2
 *		i_buffer[2] = I3
 *		i_buffer[3] = I4
 *		...
 *
 *		q_buffer[0] = Q1
 *		q_buffer[1] = Q2
 *		q_buffer[2] = Q3
 *		q_buffer[3] = Q4
 *		...
 * @endcode
 *
 * @remarks This function does not set the \b samples_per_packet on the WSA.
 * It is the caller's responsibility to configure the WSA with the correct 
 * \b samples_per_packet before initiating the capture by calling the method
 * \b wsa_set_samples_per_packet
 *
 * @param device - A pointer to the WSA device structure.
 * @param header - A pointer to \b wsa_vrt_packet_header structure to store 
 *		the VRT header information
 * @param trailer - A pointer to \b wsa_vrt_packet_trailer structure to store 
 *		the VRT trailer information
 * @param i_buffer - A 16-bit signed integer pointer for the unscaled, 
 *		I data buffer with size specified by samples_per_packet.
 * @param q_buffer - A 16-bit signed integer pointer for the unscaled 
 *		Q data buffer with size specified by samples_per_packet.
 * @param samples_per_packet - A 16-bit unsigned integer sample size (i.e. number of
 *		{I, Q} sample pairs) per VRT packet to be captured.
 *
 * @return  0 on success or a negative value on error
 */
int16_t wsa_read_vrt_packet (struct wsa_device* const dev, 
		struct wsa_vrt_packet_header* const header, 
		struct wsa_vrt_packet_trailer* const trailer,
		struct wsa_receiver_packet* const receiver,
		struct wsa_digitizer_packet* const digitizer,
		int16_t* const i_buffer, 
		int16_t* const q_buffer,
		int32_t samples_per_packet)		
{
	uint8_t* data_buffer;
	int16_t result = 0;
	int64_t frequency = 0;

	// allocate the data buffer
	data_buffer = (uint8_t*) malloc(samples_per_packet * BYTES_PER_VRT_WORD * sizeof(uint8_t));
			
	result = wsa_read_vrt_packet_raw(dev, header, trailer, receiver, digitizer, data_buffer);
	doutf(DMED, "wsa_read_vrt_packet_raw returned %hd\n", result);
	if (result < 0)
	{
		doutf(DHIGH, "Error in wsa_read_vrt_packet: %s\n", wsa_get_error_msg(result));
		if (result == WSA_ERR_NOTIQFRAME)
			wsa_system_abort_capture(dev);

		free(data_buffer);
		return result;
	} 
	
	if (header->stream_id == IF_DATA_STREAM_ID) 
	{
		// Note: don't rely on the value of result
		result = (int16_t) wsa_decode_frame(data_buffer, i_buffer, q_buffer, samples_per_packet);
	}
	
	free(data_buffer);

	return 0;
}


/**
 * Sets the number of samples per packet to be received
 * 
 * @param dev - A pointer to the WSA device structure.
 * @param samples_per_packet - The sample size to set.
 *
 * @return 0 if success, or a negative number on error.
 */
int16_t wsa_set_samples_per_packet(struct wsa_device *dev, int32_t samples_per_packet)
{
	int16_t result;
	char temp_str[50];

	if ((samples_per_packet < WSA4000_MIN_SAMPLES_PER_PACKET) || 
		(samples_per_packet > WSA4000_MAX_SAMPLES_PER_PACKET))
		return WSA_ERR_INVSAMPLESIZE;

	sprintf(temp_str, "TRACE:SPPACKET %hu\n", samples_per_packet);
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_samples_per_packet: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


/**
 * Gets the number of samples that will be returned in each
 * VRT packet when \b wsa_read_vrt_packet is called
 * 
 * @param device - A pointer to the WSA device structure.
 * @param samples_per_packet - A uint16_t pointer to store the samples per packet
 *
 * @return 0 if successful, or a negative number on error.
 */
int16_t wsa_get_samples_per_packet(struct wsa_device* device, int32_t* samples_per_packet)
{
	struct wsa_resp query;		// store query results
	long temp;

	wsa_send_query(device, "TRACE:SPPACKET?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the number & make sure no error
	if (to_int(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}

	// Verify the validity of the return value
	if ((temp < WSA4000_MIN_SAMPLES_PER_PACKET) || 
		(temp > WSA4000_MAX_SAMPLES_PER_PACKET))
	{
		printf("Error: WSA returned %ld.\n", temp);
		return WSA_ERR_RESPUNKNOWN;
	}

	*samples_per_packet = (int32_t) temp;

	return 0;
}


/**
 * Sets the number of VRT packets per each capture block.
 * The number of samples in each packet is set by
 * the method wsa_set_samples_per_packet.
 * After capturing the block with the method wsa_capture_block,
 * read back the data by calling wsa_read_vrt_packet
 * \b packets_per_block number of times
 * 
 * @param dev - A pointer to the WSA device structure.
 * @param packets_per_block - number of packets
 *
 * @return 0 if success, or a negative number on error.
 */
int16_t wsa_set_packets_per_block(struct wsa_device *dev, int32_t packets_per_block)
{
	int16_t result;
	char temp_str[50];
	
	if (packets_per_block < WSA4000_MIN_PACKETS_PER_BLOCK)
		return WSA_ERR_INVNUMBER;
	else if (packets_per_block > WSA4000_MAX_PACKETS_PER_BLOCK) 
		return WSA_ERR_INVCAPTURESIZE;

	sprintf(temp_str, "TRACE:BLOCK:PACKETS %u\n", packets_per_block);
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_packets_per_block: %d - %s.\n", result, wsa_get_error_msg(result));

	return result;
}


/**
 * Gets the number of VRT packets to be captured
 * when \b wsa_capture_block is called
 * 
 * @param device - A pointer to the WSA device structure.
 * @param packets_per_block - A uint32_t pointer to store the number of packets
 *
 * @return 0 if successful, or a negative number on error.
 */
int16_t wsa_get_packets_per_block(struct wsa_device* device, int32_t* packets_per_block)
{
	struct wsa_resp query;		// store query results
	long temp;

	wsa_send_query(device, "TRACE:BLOCK:PACKETS?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the number & make sure no error
	if (to_int(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}

	*packets_per_block = (int32_t) temp;

	return 0;
}


/**
 * Gets the decimation rate currently set in the WSA. If rate is 0, it means
 * decimation is off (or no decimation).
 * 
 * @param dev - A pointer to the WSA device structure.
 * @param rate - A pointer to the decimation rate of integer type.
 *
 * @return The sample size if success, or a negative number on error.
 */
int16_t wsa_get_decimation(struct wsa_device *dev, int32_t *rate)
{
	struct wsa_resp query;		// store query results
	long temp;

	wsa_send_query(dev, ":SENSE:DEC?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// convert & make sure no error
	if (to_int(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}

	// make sure the returned value is valid
	if (((temp != 0) && (temp < dev->descr.min_decimation)) || 
		(temp > dev->descr.max_decimation)) 
	{
		printf("Error: WSA returned %ld.\n", temp);
		return WSA_ERR_RESPUNKNOWN;
	}

	*rate = (int32_t) temp;

	return 0;
}


/**
 * Sets the decimation rate. 
 * @note: The rate here implies that at every given 'rate' (samples), 
 * only one sample is stored. Ex. if rate = 100, for a trace 
 * frame of 1000, only 10 samples is stored???
 *
 * Rate supported: 0, 4 - 1024. Rate of 0 is equivalent to no decimation.
 * 
 * @param dev - A pointer to the WSA device structure.
 * @param rate - The decimation rate to set.
 *
 * @return 0 if success, or a negative number on error.
 */
int16_t wsa_set_decimation(struct wsa_device *dev, int32_t rate)
{
	int16_t result;
	char temp_str[50];

	// TODO get min & max rate
	if (((rate != 0) && (rate < dev->descr.min_decimation)) || 
		(rate > dev->descr.max_decimation))
		return WSA_ERR_INVDECIMATIONRATE;

	sprintf(temp_str, "SENSE:DEC %d \n", rate);
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_decimation: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


/**
 * Flush current data in the socket 
 * This function is used to remove remaining sweep data after a sweep is stopped
 * @param dev - A pointer to the WSA device structure.
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_flush_data(struct wsa_device *dev) 
{
	int16_t result = 0;
	int32_t size = 0;
	char status[40];

	// check if the wsa is already sweeping
	result = wsa_get_sweep_status(dev, status);
	if (result < 0)
		return result;

	if (strcmp(status, "RUNNING") == 0) 
		return WSA_ERR_SWEEPALREADYRUNNING;

	result = wsa_send_command(dev, "SWEEP:FLUSH\n");
	doutf(DHIGH, "In wsa_flush_data: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


/**
 * Abort the current capture
 * This function is used to abort the current capture
 * @param dev - A pointer to the WSA device structure.
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_system_abort_capture(struct wsa_device *dev)
{
	int16_t result = 0;
	char status[40];
	int32_t size = 0;

	// check if the wsa is already sweeping
	result = wsa_get_sweep_status(dev, status);
	if (result < 0)
		return result;

	if (strcmp(status, "RUNNING") == 0) 
		return WSA_ERR_SWEEPALREADYRUNNING;

	result = wsa_send_command(dev, "SYSTEM:ABORT\n");
	doutf(DHIGH, "In wsa_system_abort_capture: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
// FREQUENCY SECTION                                                         //
///////////////////////////////////////////////////////////////////////////////

/**
 * Retrieves the center frequency that the WSA is running at.
 *
 * @param dev - A pointer to the WSA device structure.
 * @param cfreq - A long integer pointer to store the frequency in Hz.
 *
 * @return 0 on successful or a negative number on error.
 */
int16_t wsa_get_freq(struct wsa_device *dev, int64_t *cfreq)
{	
	struct wsa_resp query;		// store query results
	double temp;

	wsa_send_query(dev, "FREQ:CENT?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the number & make sure no error
	if (to_double(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}

	// Verify the validity of the return value
	if (temp < dev->descr.min_tune_freq || temp > dev->descr.max_tune_freq) 
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}
	
	*cfreq = (int64_t) temp;

	return 0;
}


/**
 * Sets the WSA to the desired center frequency, \b cfreq.
 * @remarks \b wsa_set_freq() will return error if trigger mode is already
 * running.
 * See the \b descr component of \b wsa_dev structure for maximum/minimum
 * frequency values.
 *
 * @param dev - A pointer to the WSA device structure.	
 * @param cfreq - The center frequency to set, in Hz
 * @return 0 on success, or a negative number on error.
 * @par Errors:
 * - Frequency out of range.
 * - Set frequency when WSA is in trigger mode.
 * - Incorrect frequency resolution (check with data sheet).
 */
int16_t wsa_set_freq(struct wsa_device *dev, int64_t cfreq)
{
	int16_t result = 0;
	char temp_str[50];

	result = wsa_verify_freq(dev, cfreq);
	if (result < 0)
		return result;

	sprintf(temp_str, "FREQ:CENT %lld Hz\n", cfreq);
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_freq: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}

/**
 * Retrieves the frequency shift value
 *
 * @param dev - A pointer to the WSA device structure.
 * @param fshift - A float pointer to store the frequency in Hz.
 *
 * @return 0 on successful or a negative number on error.
 */
int16_t wsa_get_freq_shift(struct wsa_device *dev, float *fshift)
{
	struct wsa_resp query;		// store query results
	double temp;
	double range = (double) dev->descr.inst_bw;

	wsa_send_query(dev, "FREQ:SHIFT?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the number & make sure no error
	if (to_double(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}


	if (temp < -range || temp > range) 
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}

	*fshift = (float) temp;

	return 0;
}


/**
 * Sets frequency shift value
 *
 * @param dev - A pointer to the WSA device structure.	
 * @param fshift - The center frequency to set, in Hz
 * @return 0 on success, or a negative number on error.
 * @par Errors:
 * - Frequency out of range.
 * - Set frequency when WSA is in trigger mode.
 * - Incorrect frequency resolution (check with data sheet).
 */
int16_t wsa_set_freq_shift(struct wsa_device *dev, float fshift)
{
	int16_t result = 0;
	char temp_str[50];
	int64_t range = dev->descr.inst_bw;

	// verify the value bwn -125 to 125MHz, "inclusive"
	if (fshift < (-range) || fshift > range)
		return WSA_ERR_FREQOUTOFBOUND;

	sprintf(temp_str, "FREQ:SHIFt %f Hz\n", fshift);
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_freq_shift: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


// ////////////////////////////////////////////////////////////////////////////
// GAIN SECTION                                                              //
// ////////////////////////////////////////////////////////////////////////////

/**
 * Gets the current IF gain value of the RFE in dB.
 *
 * @param dev - A pointer to the WSA device structure.
 * @param gain - An integer pointer to store the IF gain value.
 *
 * @return The gain value in dB, or a large negative number on error.
 */
int16_t wsa_get_gain_if(struct wsa_device *dev, int32_t *gain)
{
	struct wsa_resp query;		// store query results
	long int temp;

	if (strcmp(dev->descr.rfe_name, WSA_RFE0440) == 0)
		return WSA_ERR_INVRFESETTING;

	wsa_send_query(dev, "INPUT:GAIN:IF?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the number & make sure no error
	if (to_int(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}
	
	// Verify the validity of the return value
	if (temp < dev->descr.min_if_gain || temp > dev->descr.max_if_gain) 
	{
		printf("Error: WSA returned %ld.\n", temp);
		return WSA_ERR_RESPUNKNOWN;
	}
	
	*gain = (int32_t) temp;

	return 0;
}


/**
 * Sets the gain value in dB for the variable IF gain stages of the RFE, which 
 * is additive to the primary RF quantized gain stages (wsa_set_gain_rf()).
 *
 * @param dev - A pointer to the WSA device structure.
 * @param gain - The gain level in dB.
 * @remarks See the \b descr component of \b wsa_dev structure for 
 * maximum/minimum IF gain values.
 *
 * @return 0 on success, or a negative number on error.
 * @par Errors:
 * - Gain level out of range.
 */
int16_t wsa_set_gain_if(struct wsa_device *dev, int32_t gain)
{
	int16_t result = 0;
	char temp_str[50];

	if (strcmp(dev->descr.rfe_name, WSA_RFE0440) == 0)
		return WSA_ERR_INVRFESETTING;

	if (gain < dev->descr.min_if_gain || gain > dev->descr.max_if_gain)
		return WSA_ERR_INVIFGAIN;

	sprintf(temp_str, "INPUT:GAIN:IF %d dB\n", gain);
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_gain_if: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


/**
 * Gets the current quantized RF front end gain setting of the RFE.
 *
 * @param dev - A pointer to the WSA device structure.
 * @param gain - A \b wsa_gain pointer type to store the current RF gain value.
 *
 * @return 0 on successful, or a negative number on error.
 */
int16_t wsa_get_gain_rf(struct wsa_device *dev, enum wsa_gain *gain)
{
	struct wsa_resp query;		// store query results

	wsa_send_query(dev, "INPUT:GAIN:RF?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;
	*gain = gain_rf_strtonum(query.output);

	return 0;
}

/**
 * Sets the quantized \b gain (sensitivity) level for the RFE of the WSA.
 *
 * @param dev - A pointer to the WSA device structure.
 * @param gain - The gain setting of type wsa_gain to set for WSA. \n
 * Valid gain settings are:
 * - WSA_GAIN_HIGH
 * - WSA_GAIN_MED
 * - WSA_GAIN_LOW 
 * - WSA_GAIN_VLOW
 * 
 * @return 0 on success, or a negative number on error.
  * @par Errors:
 * - Gain setting not allow.
 */
int16_t wsa_set_gain_rf (struct wsa_device *dev, enum wsa_gain gain)
{
	int16_t result = 0;
	char temp_str[50];

	if (gain > WSA_GAIN_VLOW || gain < WSA_GAIN_HIGH)
		return WSA_ERR_INVRFGAIN;

	strcpy(temp_str, "INPUT:GAIN:RF ");
	switch(gain) {
		case(WSA_GAIN_HIGH):	strcat(temp_str, "HIGH"); break;
		case(WSA_GAIN_MED):		strcat(temp_str, "MED"); break;
		case(WSA_GAIN_LOW):		strcat(temp_str, "LOW"); break;
		case(WSA_GAIN_VLOW):	strcat(temp_str, "VLOW"); break;
		default:		strcat(temp_str, "ERROR"); break;
	}
	strcat(temp_str, "\n");

	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_gain_rf: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


// ////////////////////////////////////////////////////////////////////////////
// RFE CONTROL SECTION                                                       //
// ////////////////////////////////////////////////////////////////////////////


/**
 * Gets which antenna port is currently in used with the RFE board.
 * 
 * @param dev - A pointer to the WSA device structure.
 * @param port_num - An integer pointer to store the antenna port number.
 *
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_get_antenna(struct wsa_device *dev, int32_t *port_num)
{
	struct wsa_resp query;		// store query results
	long temp;

	if (strcmp(dev->descr.rfe_name, WSA_RFE0440) == 0)
		return WSA_ERR_INVRFESETTING;

	wsa_send_query(dev, "INPUT:ANTENNA?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the number & make sure no error
	if (to_int(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}

	// Verify the validity of the return value
	if (temp < 1 || temp > WSA_RFE0560_MAX_ANT_PORT) 
	{
		printf("Error: WSA returned %ld.\n", temp);
		return WSA_ERR_RESPUNKNOWN;
	}

	*port_num = (int32_t) temp;

	return 0;
}


/**
 * Sets the antenna port to be used for the RFE board.
 *
 * @param dev - A pointer to the WSA device structure.
 * @param port_num - An integer port number to set. \n
 * Available ports: 1, 2.  Or see product datasheet for ports availability.
 * 
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_set_antenna(struct wsa_device *dev, int32_t port_num)
{
	int16_t result = 0;
	char temp_str[30];

	if (strcmp(dev->descr.rfe_name, WSA_RFE0440) == 0)
		return WSA_ERR_INVRFESETTING;

	if (port_num < 1 || port_num > WSA_RFE0560_MAX_ANT_PORT) // TODO replace the max
		return WSA_ERR_INVANTENNAPORT;

	sprintf(temp_str, "INPUT:ANTENNA %d\n", port_num);
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_antenna: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


/**
 * Gets the current mode of the RFE's preselect BPF stage.
 * 
 * @param dev - A pointer to the WSA device structure.
 * @param mode - An integer pointer to store the current BPF mode: 
 * 1 = on, 0 = off.
 *
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_get_bpf_mode(struct wsa_device *dev, int32_t *mode)
{
	struct wsa_resp query;		// store query results
	long temp;

	if (strcmp(dev->descr.rfe_name, WSA_RFE0440) == 0)
		return WSA_ERR_INVRFESETTING;

	wsa_send_query(dev, "INP:FILT:PRES?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the number & make sure no error
	if (to_int(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}
	
	// Verify the validity of the return value
	if (temp < 0 || temp > 1) 
	{
		printf("Error: WSA returned %ld.\n", temp);
		return WSA_ERR_RESPUNKNOWN;
	}
		
	*mode = (int16_t) temp;

	return 0;
}


/**
 * Sets the RFE's preselect band pass filter (BPF) stage on or off (bypassing).
 * 
 * @param dev - A pointer to the WSA device structure.
 * @param mode - An integer mode of selection: 0 - Off, 1 - On.
 *
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_set_bpf_mode(struct wsa_device *dev, int32_t mode)
{
	int16_t result = 0;
	char temp_str[50];

	if (strcmp(dev->descr.rfe_name, WSA_RFE0440) == 0)
		return WSA_ERR_INVRFESETTING;

	if (mode < 0 || mode > 1)
		return WSA_ERR_INVFILTERMODE;

	sprintf(temp_str, "INPUT:FILT:PRES %d\n", mode);
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_bpf_mode: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


// ////////////////////////////////////////////////////////////////////////////
// DEVICE SETTINGS					                                         //
// ////////////////////////////////////////////////////////////////////////////

int16_t wsa_get_fw_ver(struct wsa_device *dev)
{
	struct wsa_resp query;		// store query results

	int16_t i = 0;
	
	if (strcmp(dev->descr.rfe_name, WSA_RFE0440) == 0)
		return WSA_ERR_INVRFESETTING;
	
	wsa_send_query(dev, "*IDN?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// TODO: decode base on the , position... there is a clean way to do it
	// you can't assume there only that many characters, i.e. 44 to 49
	printf("\t\t- Firmware Version: ");
	for(i=44; i<49;i++)
		printf("%c", query.output[i]);

	printf("\n");

	return 0;
}



// ////////////////////////////////////////////////////////////////////////////
// TRIGGER CONTROL SECTION                                                   //
// ////////////////////////////////////////////////////////////////////////////


/**
 * Sets the WSA to use basic a level trigger
 *
 * @param dev - A pointer to the WSA device structure.	
 * @param start_freq - The lowest frequency at which a signal should be detected
 * @param stop_freq - The highest frequency at which a signal should be detected
 * @param amplitude - The minimum amplitutde of a signal that will satisfy the trigger
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_set_trigger_level(struct wsa_device *dev, int64_t start_freq, int64_t stop_freq, int32_t amplitude)
{
	int16_t result = 0;
	char temp_str[50];

	result = wsa_verify_freq(dev, start_freq);
	if (result == WSA_ERR_FREQOUTOFBOUND)
		return WSA_ERR_STARTOOB;
	else if (result < 0)
		return result;

	result = wsa_verify_freq(dev, stop_freq);
	if (result == WSA_ERR_FREQOUTOFBOUND)
		return WSA_ERR_STOPOOB;
	else if (result < 0)
		return result;

	sprintf(temp_str, ":TRIG:LEVEL %lld,%lld,%ld\n", start_freq, stop_freq, amplitude);
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_trigger_level: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


/**
 * Retrieves the basic level trigger settings
 *
 * @param dev - A pointer to the WSA device structure.
 * @param start_freq - A long integer pointer to store the start frequency in Hz.
 * @param stop_freq - A long integer pointer to store the stop frequency in Hz.
 * @param amplitude - A long integer pointer to store the signal amplitude in dBm.
 *
 * @return 0 on successful or a negative number on error.
 */
int16_t wsa_get_trigger_level(struct wsa_device* dev, int64_t* start_freq, int64_t* stop_freq, int32_t* amplitude)
{
	struct wsa_resp query;		// store query results
	double temp;
	char* strtok_result;

	wsa_send_query(dev, ":TRIG:LEVEL?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;
	
	// Convert the 1st number & make sure no error
	strtok_result = strtok(query.output, ",");
	if (to_double(strtok_result, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}

	// Verify the validity of the return value
	if (temp < dev->descr.min_tune_freq || temp > dev->descr.max_tune_freq)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}
	
	*start_freq = (int64_t) temp;
	
	// Convert the 2nd number & make sure no error
	strtok_result = strtok(NULL, ",");
	if (to_double(strtok_result, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}

	// Verify the validity of the return value
	if (temp < dev->descr.min_tune_freq || temp > dev->descr.max_tune_freq) 
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}
	
	*stop_freq = (int64_t) temp;
	
	strtok_result = strtok(NULL, ",");
	// Convert the number & make sure no error
	if (to_double(strtok_result, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}
	
	*amplitude = (int32_t) temp;

	return 0;
}


/**
 * Sets the WSA's capture mode to triggered (trigger on)
 * or freerun (trigger off).
 * 
 * @param dev - A pointer to the WSA device structure.
 * @param enable - Trigger mode of selection: 0 - Off, 1 - On.
 *
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_set_trigger_enable(struct wsa_device* dev, int32_t enable)
{
	int16_t result = 0;
	char temp_str[50];

	if (enable < 0 || enable > 1)
		return WSA_ERR_INVTRIGGERMODE;

	sprintf(temp_str, ":TRIGGER:ENABLE %d\n", enable);
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_trigger_enable: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


/**
 * Gets the current trigger mode of the WSA
 * 
 * @param dev - A pointer to the WSA device structure.
 * @param enable - An integer pointer to store the current mode: 
 * 1 = triggered (trigger on), 0 = freerun (trigger off).
 *
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_get_trigger_enable(struct wsa_device* dev, int32_t* enable)
{
	struct wsa_resp query;		// store query results
	long temp;

	wsa_send_query(dev, ":TRIG:ENABLE?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the number & make sure no error
	if (to_int(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}
	
	// Verify the validity of the return value
	if (temp < 0 || temp > 1) 
	{
		printf("Error: WSA returned %ld.\n", temp);
		return WSA_ERR_RESPUNKNOWN;
	}
		
	*enable = (int16_t) temp;

	return 0;
}

// ////////////////////////////////////////////////////////////////////////////
// PLL Reference CONTROL SECTION                                             //
// ////////////////////////////////////////////////////////////////////////////

/**
 * Gets the PLL Reference Source
 *
 * @param dev - A pointer to the WSA device structure.	
 * @param pll_ref - An integer pointer to store the current PLL Source
 *
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_get_reference_pll(struct wsa_device* dev, char* pll_ref)
{
	struct wsa_resp query;


	if (strcmp(dev->descr.rfe_name, WSA_RFE0560) != 0)
	    return WSA_ERR_INVRFESETTING;

	wsa_send_query(dev, "SOURCE:REFERENCE:PLL?\n", &query);	
	if (query.status <= 0)
		return (int16_t) query.status;

	strcpy(pll_ref,query.output);

	return 0;
}

/**
 * Sets the PLL Reference Source
 * @param dev - A pointer to the WSA device structure.	
 * @param pll_ref - An integer used to store the value of the reference source to be set
  * @return 0 on success, or a negative number on error.
 */
int16_t wsa_set_reference_pll(struct wsa_device* dev, char* pll_ref)
{
	int16_t result = 0;
	char temp_str[30];

	if (strcmp(pll_ref, "INT") == 0 || strcmp(pll_ref, "EXT") == 0)
		sprintf(temp_str, "SOURCE:REFERENCE:PLL %s\n", pll_ref); 
	else
		return WSA_ERR_INVPLLREFSOURCE;
	
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_reference_pll: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


/**
* Reset the PLL Reference Source
* @param dev - A pointer to the WSA device structure.
*/

int16_t wsa_reset_reference_pll(struct wsa_device* dev)
{
	int16_t result = 0;
	char temp_str[30];
	
	sprintf(temp_str, "SOURCE:REFERENCE:PLL:RESET\n");
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_reset_reference_pll: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


/**
* get Lock the PLL Reference, returns if the lock reference is locked
* or unlocked
* @param dev - A pointer to the WSA device structure.
* @param lock_ref returns 1 if locked, 0 if unlocked
*/

int16_t wsa_get_lock_ref_pll(struct wsa_device* dev, int32_t* lock_ref)
{
	struct wsa_resp query;
	int16_t result = 0;
	double temp;

	wsa_send_query(dev, "LOCK:REFerence?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	if (to_double(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %ld.\n", temp);
		return WSA_ERR_RESPUNKNOWN;
	}

	*lock_ref = (int32_t) temp;

	return 0;
}


// ////////////////////////////////////////////////////////////////////////////
// Sweep Functions	(still in beta			                                             //
// ////////////////////////////////////////////////////////////////////////////


/**
 * Get the antenna port in the user's sweep entry
 * @param dev - A pointer to the WSA device structure.
 * @param port_num - An integer pointer to store the antenna location: 
 * 1 = antenna1, 2 = antenna2.
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_get_sweep_antenna(struct wsa_device *dev, int32_t *port_num) 
{
	struct wsa_resp query;		// store query results
	long temp;

	if (strcmp(dev->descr.rfe_name, WSA_RFE0440) == 0)
		return WSA_ERR_INVRFESETTING;

	wsa_send_query(dev, "SWEEP:ENTRY:ANTENNA?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the number & make sure no error
	if (to_int(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}

	// Verify the validity of the return value
	if (temp < 1 || temp > WSA_RFE0560_MAX_ANT_PORT) 
	{
		printf("Error: WSA returned %ld.\n", temp); 
		return WSA_ERR_RESPUNKNOWN;
	}

	*port_num = (int32_t) temp;

	return 0;
}

/**
 * Set the antenna in the user's sweep entry
 * @param dev - A pointer to the WSA device structure.
 * @param port_num - An integer to store the antenna location: 
 * 1 = antenna1, 2 = antenna2.
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_set_sweep_antenna(struct wsa_device *dev, int32_t port_num) 
{
	int16_t result = 0;
	char temp_str[30];

	if (strcmp(dev->descr.rfe_name, WSA_RFE0440) == 0)
		return WSA_ERR_INVRFESETTING;

	if (port_num < 1 || port_num > WSA_RFE0560_MAX_ANT_PORT) // TODO replace the max
		return WSA_ERR_INVANTENNAPORT;

	sprintf(temp_str, "SWEEP:ENTRY:ANTENNA %d\n", port_num);
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_sweep_antenna: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


/**
 * Get the if gain in the user's sweep entry
 * @param dev - A pointer to the WSA device structure.
 * @param gain - An integer  to store the gain value: 
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_get_sweep_gain_if(struct wsa_device *dev, int32_t *gain)
{
	struct wsa_resp query;		// store query results
	long int temp;

	if (strcmp(dev->descr.rfe_name, WSA_RFE0440) == 0)
		return WSA_ERR_INVRFESETTING;

	wsa_send_query(dev, "SWEEP:ENTRY:GAIN:IF?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the number & make sure no error
	if (to_int(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}
	
	// Verify the validity of the return value
	if (temp < dev->descr.min_if_gain || temp > dev->descr.max_if_gain) 
	{
		printf("Error: WSA returned %ld.\n", temp);
		return WSA_ERR_RESPUNKNOWN;
	}
	
	*gain = (int32_t) temp;

	return 0;
}


/**
 * Set the if gain in the user's sweep entry
 * @param dev - A pointer to the WSA device structure.
 * @param gain - An integer  to store the gain value: 
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_set_sweep_gain_if(struct wsa_device *dev, int32_t gain)
{
	int16_t result = 0;
	char temp_str[50];
	
	if (strcmp(dev->descr.rfe_name, WSA_RFE0440) == 0)
		return WSA_ERR_INVRFESETTING;

	if (gain < dev->descr.min_if_gain || gain > dev->descr.max_if_gain)
		return WSA_ERR_INVIFGAIN;

	sprintf(temp_str, "SWEEP:ENTRY:GAIN:IF %d\n", gain);
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_sweep_gain_if: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


/**
 * Get the rf gain in the user's sweep entry
 * @param dev - A pointer to the WSA device structure.
 * @param gain - An integer  to store the gain value: 
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_get_sweep_gain_rf(struct wsa_device *dev, enum wsa_gain *gain)
{	
	struct wsa_resp query;		// store query results

	wsa_send_query(dev, "SWEEP:ENTRY:GAIN:RF?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;
	*gain = gain_rf_strtonum(query.output);

	return 0;

}

/**
 * Set the rf gain in the user's sweep entry
 * @param dev - A pointer to the WSA device structure.
 * @param gain - An integer  to store the gain value: 
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_set_sweep_gain_rf(struct wsa_device *dev, enum wsa_gain gain)
{
	int16_t result = 0;
	char temp_str[50];

	if (gain > WSA_GAIN_VLOW || gain < WSA_GAIN_HIGH)
		return WSA_ERR_INVRFGAIN;

	strcpy(temp_str, "SWEEP:ENTRY:GAIN:RF ");
	switch(gain) {
		case(WSA_GAIN_HIGH):	strcat(temp_str, "HIGH"); break;
		case(WSA_GAIN_MED):	strcat(temp_str, "MED"); break;
		case(WSA_GAIN_LOW):		strcat(temp_str, "LOW"); break;
		case(WSA_GAIN_VLOW):	strcat(temp_str, "VLOW"); break;
		default:		strcat(temp_str, "ERROR"); break;
	}
	strcat(temp_str, "\n");

	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_sweep_gain_rf: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}

/**
 * Gets the number of samples per packet in the
 * user's sweep entry
 * @param device - A pointer to the WSA device structure.
 * @param samples_per_packet - A uint16_t pointer to store the samples per packet
 * @return 0 if successful, or a negative number on error.
 */
int16_t wsa_get_sweep_samples_per_packet(struct wsa_device* device, int32_t* samples_per_packet)
{
	struct wsa_resp query;		// store query results
	long temp;

	wsa_send_query(device, "SWEEP:ENTRY:SPPACKET?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the number & make sure no error
	if (to_int(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}

	// Verify the validity of the return value
	if ((temp < WSA4000_MIN_SAMPLES_PER_PACKET) || 
		(temp > WSA4000_MAX_SAMPLES_PER_PACKET))
	{
		printf("Error: WSA returned %ld.\n", temp);
		return WSA_ERR_RESPUNKNOWN;
	}

	*samples_per_packet = (uint32_t) temp;

	return 0;
}

/**
 * Sets the number of samples per packet in the user's
 * sweep entry
 * @param dev - A pointer to the WSA device structure.
 * @param samples_per_packet - The sample size to set.
 * @return 0 if success, or a negative number on error.
 */
int16_t wsa_set_sweep_samples_per_packet(struct wsa_device* device, int32_t samples_per_packet)
{
	int16_t result;
	char temp_str[50];

	if ((samples_per_packet < WSA4000_MIN_SAMPLES_PER_PACKET) || 
		(samples_per_packet > WSA4000_MAX_SAMPLES_PER_PACKET))
		return WSA_ERR_INVSAMPLESIZE;

	sprintf(temp_str, "SWEEP:ENTRY:SPPACKET %hu\n", samples_per_packet);
	result = wsa_send_command(device, temp_str);
	doutf(DHIGH, "In wsa_set_sweep_samples_per_packet: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}

/**
 * Gets the packets per block in the user's
 * sweep entry
 * @param device - A pointer to the WSA device structure.
 * @param packets_per_block - A uint32_t pointer to store the number of packets
 * @return 0 if successful, or a negative number on error.
 */
int16_t wsa_get_sweep_packets_per_block(struct wsa_device* device, uint32_t* packets_per_block)
{
	struct wsa_resp query;		// store query results
	long temp;

	wsa_send_query(device, "SWEEP:ENTRY:PPBLOCK?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the number & make sure no error
	if (to_int(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}

	*packets_per_block = (uint32_t) temp;

	return 0;
}

/**
 * Sets the number of packets per block in the user's
 * sweep entry 
 * @param dev - A pointer to the WSA device structure.
 * @param packets_per_block - number of packets
 * @return 0 if success, or a negative number on error.
 */
int16_t wsa_set_sweep_packets_per_block(struct wsa_device* device, uint32_t packets_per_block)
{
	int16_t result;
	char temp_str[50];

	if (packets_per_block < WSA4000_MIN_PACKETS_PER_BLOCK)
		return WSA_ERR_INVNUMBER;
	else if (packets_per_block > WSA4000_MAX_PACKETS_PER_BLOCK) 
		return WSA_ERR_INVCAPTURESIZE;

	sprintf(temp_str, "SWEEP:ENTRY:PPBLOCK %u\n", packets_per_block);
	result = wsa_send_command(device, temp_str);
	doutf(DHIGH, "In wsa_set_sweep_packets_per_block: %d - %s.\n", result, wsa_get_error_msg(result));

	return result;
}


/**
 * Gets the decimation rate currently set in the user's
 * sweep list 
 * @param dev - A pointer to the WSA device structure.
 * @param rate - A pointer to the decimation rate of integer type.
 * @return The sample size if success, or a negative number on error.
 */
int16_t wsa_get_sweep_decimation(struct wsa_device* device, int32_t* rate)
{
	struct wsa_resp query;		// store query results
	long temp;

	wsa_send_query(device, "SWEEP:ENTRY:DECIMATION?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// convert & make sure no error
	if (to_int(query.output, &temp) < 0)
	{
		return WSA_ERR_RESPUNKNOWN;
	}

	// make sure the returned value is valid
	if (((temp != 0) && (temp < device->descr.min_decimation)) || 
		(temp > device->descr.max_decimation))
	{
		printf("Error: WSA returned %ld.\n", temp);
		return WSA_ERR_RESPUNKNOWN;
	}
	*rate = (int32_t) temp;

	return 0;
}

/**
 * Sets the decimation rate from the user's sweep list. 
 * @note: The rate here implies that at every given 'rate' (samples), 
 * only one sample is stored. Ex. if rate = 100, for a trace 
 * frame of 1000, only 10 samples is stored???
 * Rate supported: 0, 4 - 1024. Rate of 0 is equivalent to no decimation.
 * @param device - A pointer to the WSA device structure.
 * @param rate - The decimation rate to set.
 * @return 0 if success, or a negative number on error.
 */
int16_t wsa_set_sweep_decimation(struct wsa_device* device, int32_t rate)
{
	int16_t result;
	char temp_str[50];

	// TODO get min & max rate
	if (((rate != 0) && (rate < device->descr.min_decimation)) || 
		(rate > device->descr.max_decimation))
		return WSA_ERR_INVDECIMATIONRATE;

	sprintf(temp_str, "SWEEP:ENTRY:DECIMATION %d\n", rate);
	result = wsa_send_command(device, temp_str);
	doutf(DHIGH, "In wsa_set_sweep_decimation: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}

/**
 * Retrieves the center frequency from the.
 * from the user's sweep entry
 * @param dev - A pointer to the WSA device structure.
 * @param start_freq - A long integer pointer to store the initial frequency in Hz.
  * @param stop_freq - A long integer pointer to store the final frequency in Hz.
  * @return 0 on successful or a negative number on error.
 */
int16_t wsa_get_sweep_freq(struct wsa_device* device, int64_t* start_freq, int64_t* stop_freq)
{
	struct wsa_resp query;	// store query results
	double temp;
	char* strtok_result;

	wsa_send_query(device, "SWEEP:ENTRY:FREQ:CENTER?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	strtok_result = strtok(query.output, ",");
	// Convert the number & make sure no error
	if (to_double(strtok_result, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}

	*start_freq = (int64_t) temp;
	strtok_result = strtok(NULL, ",");
	// Convert the number & make sure no error
	if (to_double(strtok_result, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}

	*stop_freq = (int64_t) temp;

	return 0;
}
/**
 * Sets the center frequency in the user's
 * sweep entry.
 * @param dev - A pointer to the WSA device structure.	
 * @param start_freq - The initial center frequency to set, in Hz
  * @param stop_freq - The final center frequency to set, in Hz
 * @return 0 on success, or a negative number on error.
 * @par Errors:
 * - Frequency out of range.
 * - Set frequency when WSA is in trigger mode.
 * - Incorrect frequency resolution (check with data sheet).
 */
int16_t wsa_set_sweep_freq(struct wsa_device* device, int64_t start_freq, int64_t stop_freq )
{
	int16_t result = 0;
	char temp_str[50];
	
	// verify start freq value
	result = wsa_verify_freq(device, start_freq);
	if (result == WSA_ERR_FREQOUTOFBOUND)
		return WSA_ERR_STARTOOB;
	else if (result < 0)
		return result;

	// verify start freq value
	result = wsa_verify_freq(device, stop_freq);
	if (result == WSA_ERR_FREQOUTOFBOUND)
		return WSA_ERR_STOPOOB;
	else if (result < 0)
		return result;
		
	// make sure stop_freq is larger than start
	if (stop_freq <= start_freq)
		return  WSA_ERR_INVSTOPFREQ;
		
	sprintf(temp_str, "SWEEP:ENTRY:FREQ:CENT %lld Hz, %lld Hz\n", start_freq, stop_freq);
	result = wsa_send_command(device, temp_str);
	doutf(DHIGH, "In wsa_set_sweep_freq: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}

/**
 * Retrieves the frequency shift value from the user's 
 * sweep entry
 * @param dev - A pointer to the WSA device structure.
 * @param fshift - A float pointer to store the frequency in Hz.
 * @return 0 on successful or a negative number on error.
 */
int16_t wsa_get_sweep_freq_shift(struct wsa_device* device, float* fshift)
{
	struct wsa_resp query;		// store query results
	double temp;

	wsa_send_query(device, "SWEEP:ENTRY:FREQ:SHIFT?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the number & make sure no error
	if (to_double(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}

	*fshift = (float) temp;

	return 0;
}
/**
 * Sets the frequency shift value in the user's
 * sweep entry
 * @param dev - A pointer to the WSA device structure.
 * @param fshift - A float pointer to store the frequency in Hz.
 * @return 0 on successful or a negative number on error.
 */
int16_t wsa_set_sweep_freq_shift(struct wsa_device* device, float fshift)
{
	int16_t result = 0;
	char temp_str[50];
	int64_t range = device->descr.inst_bw;

	// verify the value bwn -125 to 125MHz, "inclusive"
	if (fshift < (-range) || fshift > range)
		return WSA_ERR_FREQOUTOFBOUND;

	sprintf(temp_str, "SWEEP:ENTRY:FREQ:SHIFt %f Hz\n", fshift);
	result = wsa_send_command(device, temp_str);
	doutf(DHIGH, "In wsa_set_sweep_freq_shift: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}

/**
 * Sets the frequency step value in the user's
 * sweep entry
 * @param dev - A pointer to the WSA device structure.
 * @param fstep - A float pointer to store the frequency in Hz.
 * @return 0 on successful or a negative number on error.
 */
int16_t wsa_set_sweep_freq_step(struct wsa_device* device, int64_t step)
{
	int16_t result = 0;
	char temp_str[50];

	sprintf(temp_str, "SWEEP:ENTRY:FREQ:STEP %lld Hz\n", step);
	result = wsa_send_command(device, temp_str);
	doutf(DHIGH, "In wsa_set_sweep_freq_step: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}

/**
 * Retrieves the frequency step value from the user's 
 * sweep entry
 * @param dev - A pointer to the WSA device structure.
 * @param fstep - A float pointer to store the frequency in Hz.
 * @return 0 on successful or a negative number on error.
 */
int16_t wsa_get_sweep_freq_step(struct wsa_device* device, int64_t* fstep)
{
	struct wsa_resp query;		// store query results
	double temp;

	wsa_send_query(device, "SWEEP:ENTRY:FREQ:STEP?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the number & make sure no error
	if (to_double(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}
	
	*fstep = (int64_t) temp;

	return 0;
}

/**
 * Sets the dwell time to the sweep entry template
 *
 * @param dev - A pointer to the WSA device structure.
 * @param seconds - An integer to store the seconds value.
 * @param microseconds - An integer to store the microseconds value.
 *
 * @return 0 on successful or a negative number on error.
 */
int16_t wsa_set_sweep_dwell(struct wsa_device* device, int32_t seconds, int32_t microseconds)
{
	int16_t result = 0;
	char temp_str[50];

	if ((seconds < 0) || (microseconds < 0))
	return WSA_ERR_INVALID_DWELL;

	sprintf(temp_str, "SWEEP:ENTRY:DWELL %u,%u\n", seconds, microseconds);
	result = wsa_send_command(device, temp_str);
	doutf(DHIGH, "In wsa_set_sweep_dwell: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}

/**
 * retrieves the dwell time in the sweep entry template
 *
 * @param dev - A pointer to the WSA device structure.
 * @param seconds - An integer pointer to store the seconds value.
 * @param microseconds - An integer pointer to store the microseconds value.
 *
 * @return 0 on successful or a negative number on error.
 */
int16_t wsa_get_sweep_dwell(struct wsa_device* device, int32_t* seconds, int32_t* microseconds)
{
	struct wsa_resp query;		// store query results
	double temp =5;
	char* strtok_result;

	wsa_send_query(device, "SWEEP:ENTRY:DWELL?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the 1st number & make sure no error
	strtok_result = strtok(query.output, ",");
	if (to_double(strtok_result, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}
	*seconds = (int32_t) temp;
	
	// Convert the 2nd number & make sure no error
	strtok_result = strtok(NULL, ",");
	if (to_double(strtok_result, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}
	*microseconds = (int32_t) temp;
	
	return 0;
}


/**
 * Sets the user's sweep entry to use basic a level trigger
 * @param dev - A pointer to the WSA device structure.	
 * @param start_freq - The lower bound frequency range in Hz
 * @param stop_freq - The upper bound frequency range in Hz
 * @param amplitude - The minimum amplitude threshold of a signal that a
 *        trigger to occur, in dBm
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_set_sweep_trigger_level(struct wsa_device *dev, int64_t start_freq, int64_t stop_freq, int32_t amplitude)
{
	int16_t result = 0;
	char temp_str[50];

	result = wsa_verify_freq(dev, start_freq);
	if (result == WSA_ERR_FREQOUTOFBOUND)
		return WSA_ERR_STARTOOB;

	result = wsa_verify_freq(dev, stop_freq);
	if (result == WSA_ERR_FREQOUTOFBOUND)
		return WSA_ERR_STOPOOB;

	if (stop_freq <= start_freq)
		return WSA_ERR_INVSTOPFREQ;

	sprintf(temp_str, "SWEEP:ENTRY:TRIGGER:LEVEL %lld,%lld,%ld\n", start_freq, stop_freq, amplitude);
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_sweep_trigger_level: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


/**
 * Retrieves the user's sweep entry basic level trigger settings
 * @param dev - A pointer to the WSA device structure.
 * @param start_freq - A long integer pointer to store the start frequency in Hz.
 * @param stop_freq - A long integer pointer to store the stop frequency in Hz.
 * @param amplitude - A long integer pointer to store the signal amplitude in dBm.
 * @return 0 on successful or a negative number on error.
 */
int16_t wsa_get_sweep_trigger_level(struct wsa_device* dev, int64_t* start_freq, int64_t* stop_freq, int32_t* amplitude)
{
	struct wsa_resp query;		// store query results
	double temp;
	char* strtok_result;
	
	wsa_send_query(dev, "SWEEP:ENTRY:TRIGGER:LEVEL?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the 1st number & make sure no error
	strtok_result = strtok(query.output, ",");
	if (to_double(strtok_result, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}
	*start_freq = (int64_t) temp;

	// Convert the 2nd number & make sure no error
	strtok_result = strtok(NULL, ",");
	if (to_double(strtok_result, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}
	*stop_freq = (int64_t) temp;

	// Convert the 3rd number & make sure no error
	strtok_result = strtok(NULL, ",");
	if (to_double(strtok_result, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}	
	*amplitude = (int32_t) temp;

	return 0;
}


/**
 * Sets the user's sweep entry capture mode to triggered (trigger on)
 * or freerun (trigger off).
 * @param dev - A pointer to the WSA device structure.
 * @param enable - Trigger mode of selection: 0 - Off, 1 - On.
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_set_sweep_trigger_type(struct wsa_device* dev, int32_t enable)
{
	int16_t result = 0;
	char temp_str[50];

	if (enable < 0 || enable > 1)
		return WSA_ERR_INVTRIGGERMODE;

	if (!enable) 
		sprintf(temp_str, "SWEEP:ENTRY:TRIGGER:TYPE NONE\n");
	else
		sprintf(temp_str, "SWEEP:ENTRY:TRIGGER:TYPE LEVEL\n");

	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_set_sweep_trigger_type: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}

/**
 * retrieves the current user's sweep entry capture mode of the WSA
 * @param dev - A pointer to the WSA device structure.
 * @param enable - An integer pointer to store the current mode: 
 * 1 = triggered (trigger on), 0 = freerun (trigger off).
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_get_sweep_trigger_type(struct wsa_device* dev, int32_t* enable)
{
	struct wsa_resp query;
	
	wsa_send_query(dev, "SWEEP:ENTRY:TRIGGER:TYPE?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	if (strcmp(query.output, "LEVEL") == 0)
		*enable = 1;
	else if (strcmp(query.output, "NONE") == 0)
		*enable = 0;
	else
		*enable = -1; // TODO: handle error

	return 0;
}

/**
 * Note this function has not been implemented yet in the wsa
 * retrieves the current sweep list's iteration
 * @param dev - A pointer to the WSA device structure.
 * @param interation - An integer pointer to store the number of iterations 
 * that the sweep will go
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_get_sweep_iteration(struct wsa_device* device, int32_t* interation)
{
	struct wsa_resp query;		// store query results
	double temp;

	wsa_send_query(device, "SWEEP:LIST:ITERATION?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the number & make sure no error
	if (to_double(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}

	*interation = (int32_t) temp;

	return 0;
}


/**
 *Note this function has not been implemented yet in the wsa
 * sets the current sweeps iteraiton
 * @param dev - A pointer to the WSA device structure.
 * @param interation - An integer pointer to store the number of iterations 
 * that the sweep will go
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_set_sweep_iteration(struct wsa_device* device, int32_t interation)
{
	int16_t result;
	char temp_str[50];
	sprintf(temp_str, "SWEEP:LIST:ITERATION %u\n", interation);

	result = wsa_send_command(device, temp_str);
	if (result < 0)
	doutf(DHIGH, "In wsa_set_sweep_iteration: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}


/**
 * retrieves the current sweep list's status
 * @param dev - A pointer to the WSA device structure.
 * @param status - An integer pointer to store status,
 * 1 - running, 0 - stopped
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_get_sweep_status(struct wsa_device* device, char* status)
{
	struct wsa_resp query;	// store query results

	wsa_send_query(device, "SWEEP:LIST:STATUS?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// check if the wsa returns an invalid sweep status
	if (strcmp(query.output, "STOPPED") != 0 && strcmp(query.output, "RUNNING") != 0)
		return WSA_ERR_SWEEPMODEUNDEF;

	strcpy(status, query.output);
	return 0;
}


/**
 * retrieves the current sweep list's size
 * @param dev - A pointer to the WSA device structure.
 * @param size - An integer pointer to store the size
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_get_sweep_entry_size(struct wsa_device* device, int32_t* size)
{
	struct wsa_resp query;		// store query results
	double temp;

	wsa_send_query(device, "SWEEP:ENTRY:COUNT?\n", &query);
	if (query.status <= 0)
		return (int16_t) query.status;

	// Convert the number & make sure no error
	if (to_double(query.output, &temp) < 0)
	{
		printf("Error: WSA returned %s.\n", query.output);
		return WSA_ERR_RESPUNKNOWN;
	}
	
	*size = (int32_t) temp;
		
	return 0;
}

/**
 * delete an entry in the sweep list
 *
 * @param dev - A pointer to the WSA device structure.
 * @param id - An integer containing the ID of the entry to be deleted
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_sweep_entry_delete(struct wsa_device *dev, int32_t id) 
{
	int16_t result;
	int32_t size;
	char temp_str[50];
	
	// check if id is out of bounds
	result = wsa_get_sweep_entry_size(dev, &size);
	if (result < 0)
		return WSA_ERR_SWEEPENTRYDELETEFAIL;
	if (id < 1 || id > size)
		return WSA_ERR_SWEEPIDOOB;

	sprintf(temp_str, "SWEEP:ENTRY:DELETE %u\n", id);
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_sweep_entry_delete: %d - %s.\n", result, wsa_get_error_msg(result));

	return result;
}

/**
 * delete all entries in the sweep list
 *
 * @param dev - A pointer to the WSA device structure.
 * @param id - An integer containing the ID of the entry to be deleted
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_sweep_entry_delete_all(struct wsa_device *dev) 
{
	int16_t result;
	char temp_str[50];
	
	sprintf(temp_str, "SWEEP:ENTRY:DELETE ALL\n");
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_sweep_entry_delete: %d - %s.\n", result, wsa_get_error_msg(result));

	return result;
}

/**
 * copy the settings of an entry with the specified ID in the sweep list 
 * to the entry template
 *
 * @param dev - A pointer to the WSA device structure.
 * @param id - An integer representing the ID of the entry to be copied
 *
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_sweep_entry_copy(struct wsa_device *dev, int32_t id) 
{
	int16_t result = 0;
	char temp_str[50];
	int32_t size = 0;

	// check if the id is out of bounds
	if (id < 0 || id > size)
		return WSA_ERR_SWEEPIDOOB;

	result = wsa_get_sweep_entry_size(dev, &size);
	if (result < 0)
		return result;
	if (size == 0)
		return  WSA_ERR_SWEEPLISTEMPTY;

	sprintf(temp_str, "SWEEP:ENTRY:COPY %u\n", id);
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_sweep_entry_copy: %d - %s.\n", result, wsa_get_error_msg(result));

	return result;
}

/**
 * start sweeping through the current sweep list
 *
 * @param dev - A pointer to the WSA device structure.
 *
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_sweep_start(struct wsa_device *dev) 
{	
	int16_t result = 0;
	char status[40];
	int32_t size = 0;

	// check if the wsa is already sweeping
	result = wsa_get_sweep_status(dev, status);
	if (result < 0)
		return result;
	if (strcmp(status, "RUNNING") == 0)
		return WSA_ERR_SWEEPALREADYRUNNING;

	// check if the sweep list is empty
	result = wsa_get_sweep_entry_size(dev, &size);
	if (result < 0)
		return result;
	if (size <= 0) 
		return WSA_ERR_SWEEPLISTEMPTY;
	
	result = wsa_send_command(dev, "SWEEP:LIST:START\n");
	doutf(DHIGH, "In wsa_sweep_start: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}

/**
 * stop sweeping
 * 
 * @param dev - A pointer to the WSA device structure.
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_sweep_stop(struct wsa_device *dev) 
{
	int16_t result = 0;
	int32_t vrt_header_bytes = 2 * BYTES_PER_VRT_WORD;
	uint8_t* vrt_header_buffer;
	int32_t bytes_received = 0;
	uint32_t timeout = 360;
	clock_t start_time;
	clock_t end_time;

	result = wsa_send_command(dev, "SWEEP:LIST:STOP\n");
	doutf(DHIGH, "In wsa_sweep_stop: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
 
	// flush remaining sweep data in the WSA
	result = wsa_flush_data(dev);
	if (result < 0)
		return result;

	start_time = clock();
	end_time = 5000 + start_time;
	
	// read the left over packets from the socket
	while(clock() <= end_time) 
	{
		vrt_header_buffer = (uint8_t*) malloc(vrt_header_bytes * sizeof(uint8_t));
		
		result = wsa_sock_recv_data(dev->sock.data, 
									vrt_header_buffer, 
									vrt_header_bytes, 
									timeout, 
									&bytes_received);
		
		//if (result < 0)
		//	return result;
	
		free(vrt_header_buffer);
	}
	
	return 0;
}

/**
 * resume sweeping through the current sweep list starting
 * from the entry ID that was stopped
 *
 * @param dev - A pointer to the WSA device structure.
 *
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_sweep_resume(struct wsa_device *dev) 
{
	int16_t result = 0;
	char status[40];
	int32_t size = 0;

	result = wsa_get_sweep_status(dev, status);
	if (result < 0)
		return result;

	if (strcmp(status, "RUNNING") == 0) 
		return WSA_ERR_SWEEPALREADYRUNNING;

	result = wsa_get_sweep_entry_size(dev, &size);
	if (result < 0)
		return result;
	if (size <= 0)
		return WSA_ERR_SWEEPLISTEMPTY;

	result = wsa_send_command(dev, "SWEEP:LIST:RESUME\n");
	doutf(DHIGH, "In wsa_sweep_resume: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}

/**
 * reset the current settings in the entry template to default values
 *
 * @param dev - A pointer to the WSA device structure.
 *
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_sweep_entry_new(struct wsa_device *dev) 
{
	int16_t result = 0;
	
	result = wsa_send_command(dev, "SWEEP:ENTRY:NEW\n");
	doutf(DHIGH, "In wsa_sweep_entry_new: %d - %s.\n", result, wsa_get_error_msg(result));
	
	return result;
} 

/**
 * Save the sweep entry to a specified ID location in the sweep list.
 * If the ID is 0, the entry will go to the end of the list.
 *
 * @param dev - A pointer to the WSA device structure.
 * @param id - ID position where the entry will be saved in the list.
 *
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_sweep_entry_save(struct wsa_device *dev, int32_t id) 
{
	int16_t result;
	char temp_str[50];
	int32_t size = 0;

	// check if id is out of bounds
	result = wsa_get_sweep_entry_size(dev, &size);
	if (result < 0)
		return result;

	if (id < 0 || id > (size + 1))
		return WSA_ERR_SWEEPIDOOB;

	sprintf(temp_str, "SWEEP:ENTRY:SAVE %u\n", id);
	result = wsa_send_command(dev, temp_str);
	doutf(DHIGH, "In wsa_sweep_entry_save: %d - %s.\n", result, wsa_get_error_msg(result));
	if (result < 0)
		return result;
	
	return 0;
}

/**
 * return the settings of the entry with the specified ID
 *
 * @param dev - A pointer to the WSA device structure.
 * @param id - the sweep entry ID in the wsa's list to read
 * @param sweep_list - a pointer structure to store the settings
 *
 * @return 0 on success, or a negative number on error.
 */
int16_t wsa_sweep_entry_read(struct wsa_device *dev, int32_t id, struct wsa_sweep_list* const sweep_list)
{
	char temp_str[50];
	struct wsa_resp query;		// store query results
	double temp;
	char* strtok_result;
	
	if (id < 0)
		return WSA_ERR_INVNUMBER;

	sprintf(temp_str, "SWEEP:ENTRY:READ? %d\n", id);
	wsa_send_query(dev, temp_str, &query);
	if (query.status <= 0)
		return (int16_t) query.status;
	
	// *****
	// Convert the numbers & make sure no error
	// ****

	strtok_result = strtok(query.output, ",");
	if (to_double(strtok_result, &temp) < 0)
		return WSA_ERR_RESPUNKNOWN;
	sweep_list->start_freq = (int64_t) temp;
	
	strtok_result = strtok(NULL, ",");
	if (to_double(strtok_result, &temp) < 0)
		return WSA_ERR_RESPUNKNOWN;
	sweep_list->stop_freq = (int64_t) temp;

	strtok_result = strtok(NULL, ",");
	if (to_double(strtok_result, &temp) < 0)
		return WSA_ERR_RESPUNKNOWN;	
	sweep_list->fstep= (int64_t) temp;

	strtok_result = strtok(NULL, ",");
	if (to_double(strtok_result, &temp) < 0)
		return WSA_ERR_RESPUNKNOWN;
	sweep_list->fshift = (float) temp;

	strtok_result = strtok(NULL, ",");
	if (to_double(strtok_result, &temp) < 0)
		return WSA_ERR_RESPUNKNOWN;	
	sweep_list->decimation_rate = (int32_t) temp;

	strtok_result = strtok(NULL, ",");
	if (to_double(strtok_result, &temp) < 0)
		return WSA_ERR_RESPUNKNOWN;	
	sweep_list->ant_port = (int32_t) temp;
	
	// Convert to wsa_gain type
	strtok_result = strtok(NULL, ",");
	sweep_list->gain_rf = gain_rf_strtonum(strtok_result);

	strtok_result = strtok(NULL, ",");
	if (to_double(strtok_result, &temp) < 0)
		return WSA_ERR_RESPUNKNOWN;
	sweep_list->gain_if = (int32_t) temp;

	strtok_result = strtok(NULL, ",");
	if (to_double(strtok_result, &temp) < 0)
		return WSA_ERR_RESPUNKNOWN;
	sweep_list->samples_per_packet = (int16_t) temp;

	strtok_result = strtok(NULL, ",");
	if (to_double(strtok_result, &temp) < 0)
		return WSA_ERR_RESPUNKNOWN;
	sweep_list->packets_per_block = (int32_t) temp;

	strtok_result = strtok(NULL, ",");
	if (to_double(strtok_result, &temp) < 0)
		return WSA_ERR_RESPUNKNOWN;
	sweep_list->dwell_seconds = (int32_t) temp;

	strtok_result = strtok(NULL, ",");
	if (to_double(strtok_result, &temp) < 0)
		return WSA_ERR_RESPUNKNOWN;	
	sweep_list->dwell_microseconds = (int32_t) temp;

	strtok_result = strtok(NULL, ",");	
	if (strstr(strtok_result, "LEVEL") != NULL)
	{
		sweep_list->trigger_enable = 1;

		strtok_result = strtok(NULL, ",");
		if (to_double(strtok_result, &temp) < 0)
			return WSA_ERR_RESPUNKNOWN;
		sweep_list->trigger_start_freq = (int64_t) temp;
		
		strtok_result = strtok(NULL, ",");
		if (to_double(strtok_result, &temp) < 0)
			return WSA_ERR_RESPUNKNOWN;
		sweep_list->trigger_stop_freq = (int64_t) temp;

		strtok_result = strtok(NULL, ",");
		if (to_double(strtok_result, &temp) < 0)
			return WSA_ERR_RESPUNKNOWN;	
		sweep_list->trigger_amplitude = (int32_t) temp;
	}
	else if (strstr(strtok_result, "NONE") != NULL)
	{
		sweep_list->trigger_enable = 0;
	}

	return 0;
}

