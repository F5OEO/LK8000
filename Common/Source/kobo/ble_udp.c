/*
 *
 *  GattLib - GATT Library
 *
 *  Copyright (C) 2016-2021  Olivier Martin <olivier@labapart.org>
 * F5OEO -> udp output
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include<time.h>
#define SERVER "127.0.0.1"

#define PORT 8880	//The port on which to send data

#ifdef GATTLIB_LOG_BACKEND_SYSLOG
#include <syslog.h>
#endif

#include "gattlib.h"

#define MIN(a,b)	((a)<(b)?(a):(b))

#define NUS_CHARACTERISTIC_TX_UUID	"6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define NUS_CHARACTERISTIC_RX_UUID	"6e400003-b5a3-f393-e0a9-e50e24dcca9e"

struct sockaddr_in si_other;
	int s, i, slen=sizeof(si_other);
	

gatt_connection_t* m_connection=NULL;

char buffer_temp[1500]; //UDP MTU
unsigned int buffer_temp_size=0;

void notification_cb(const uuid_t* uuid, const uint8_t* data, size_t data_length, void* user_data) {
	int i;
    static time_t start=0;
    if(start==0)     time(&start);
    
    time_t current;
    time(&current);
	for(i = 0; i < data_length; i++) {
		fprintf(stderr,"%c", data[i]);
	}
    
    if(buffer_temp_size+data_length<1500)
    {
        memcpy(buffer_temp+buffer_temp_size,data,data_length);
        buffer_temp_size+=data_length;              
    }
    
    if(difftime(current,start)>1) //send only every 1 second 
    {
        
        if(buffer_temp_size>0)
            sendto(s, buffer_temp, buffer_temp_size , 0 , (struct sockaddr *) &si_other, slen);
        buffer_temp_size=0;
        time(&start);
    }
    
    fflush(stderr);
}

static void usage(char *argv[]) {
	printf("%s <device_address>\n", argv[0]);
}

void int_handler(int dummy) {
	gattlib_disconnect(m_connection);
	exit(0);
}

void die(char *s)
{
	perror(s);
	exit(1);
}

int main(int argc, char *argv[]) {
		
	int i, ret  = 0;
	uuid_t nus_characteristic_tx_uuid;
	uuid_t nus_characteristic_rx_uuid;

	#define GATTLIB_LOG_LEVEL 3
    

	if (argc != 2) {
		usage(argv);
		return 1;
	}
    


    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		die("socket");
	}

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);
	
	if (inet_aton(SERVER , &si_other.sin_addr) == 0) 
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

#ifdef GATTLIB_LOG_BACKEND_SYSLOG
	openlog("gattlib_ble_udp", LOG_CONS | LOG_NDELAY | LOG_PERROR, LOG_USER);
	setlogmask(LOG_UPTO(LOG_INFO));
#endif

    while(m_connection==NULL)
    {   
	m_connection = gattlib_connect(NULL, argv[1],
				       GATTLIB_CONNECTION_OPTIONS_LEGACY_BDADDR_LE_RANDOM |
				       GATTLIB_CONNECTION_OPTIONS_LEGACY_BT_SEC_LOW,NULL,NULL);
	if (m_connection == NULL) {
		GATTLIB_LOG(GATTLIB_ERROR, "Fail to connect to the bluetooth device.");
	sleep(5);
	}
    }
	// Convert characteristics to their respective UUIDs
	ret = gattlib_string_to_uuid(NUS_CHARACTERISTIC_TX_UUID, strlen(NUS_CHARACTERISTIC_TX_UUID) + 1, &nus_characteristic_tx_uuid);
	if (ret) {
		GATTLIB_LOG(GATTLIB_ERROR, "Fail to convert characteristic TX to UUID.");
		return 1;
	}
	ret = gattlib_string_to_uuid(NUS_CHARACTERISTIC_RX_UUID, strlen(NUS_CHARACTERISTIC_RX_UUID) + 1, &nus_characteristic_rx_uuid);
	if (ret) {
		GATTLIB_LOG(GATTLIB_ERROR, "Fail to convert characteristic RX to UUID.");
		return 1;
	}

	// Look for handle for NUS_CHARACTERISTIC_TX_UUID
	gattlib_characteristic_t* characteristics;
	int characteristic_count;
	ret = gattlib_discover_char(m_connection, &characteristics, &characteristic_count);
	if (ret) {
		GATTLIB_LOG(GATTLIB_ERROR, "Fail to discover characteristic.");
		return 1;
	}

	uint16_t tx_handle = 0, rx_handle = 0;
	for (i = 0; i < characteristic_count; i++) {
		if (gattlib_uuid_cmp(&characteristics[i].uuid, &nus_characteristic_tx_uuid) == 0) {
			tx_handle = characteristics[i].value_handle;
		} else if (gattlib_uuid_cmp(&characteristics[i].uuid, &nus_characteristic_rx_uuid) == 0) {
			rx_handle = characteristics[i].value_handle;
		}
	}
	if (tx_handle == 0) {
		GATTLIB_LOG(GATTLIB_ERROR, "Fail to find NUS TX characteristic.");
		return 1;
	} else if (rx_handle == 0) {
		GATTLIB_LOG(GATTLIB_ERROR, "Fail to find NUS RX characteristic.");
		return 1;
	}
	free(characteristics);

	// Register notification handler
	gattlib_register_notification(m_connection, notification_cb, NULL);

	ret = gattlib_notification_start(m_connection, &nus_characteristic_rx_uuid);
	if (ret) {
		GATTLIB_LOG(GATTLIB_ERROR, "Fail to start notification.");
		return 2;
	}

	// Register handler to catch Ctrl+C
	signal(SIGINT, int_handler);

	while(1) {
		sleep(1);
	}

	return 0;
}
