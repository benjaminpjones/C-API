#ifndef __WSA_CLIENT_H__
#define __WSA_CLIENT_H__

#include "targetver.h"
#include <fstream>
#include <iostream>
#include <string.h>
//#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <conio.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>	// to be used when passing unknown # or args
#include "stdint.h"
#include "ws-util.h"

#define FALSE	0
#define TRUE	1

#define MAX_STR_LEN 200
#define MAX_BUF_SIZE 20

#define TIMEOUT 1000	/* Timeout for sockets in milliseconds */
#define HISLIP 4880		/* Connection protocol's port to use with TCPIP */


///////////////////////////////////////////////////////////////////////////////
// Global Functions
///////////////////////////////////////////////////////////////////////////////
int32_t wsa_list_ips(char **ip_list);
u_long wsa_verify_addr(const char *sock_addr);
int32_t wsa_get_host_info(char *name);
int32_t wsa_start_client(const char *wsa_addr, SOCKET *cmd_sock, 
						 SOCKET *data_sock);
int32_t wsa_close_client(SOCKET cmd_sock, SOCKET data_sock);

#endif
