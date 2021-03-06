#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gnb_platform.h"

#ifdef __UNIX_LIKE_OS__
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

#ifdef _WIN32

#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600

#include <winsock2.h>
#include <ws2tcpip.h>

#define _POSIX
#define __USE_MINGW_ALARM
#endif


#include "gnb_address.h"


unsigned long long gnb_htonll(unsigned long long val){

#if __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
	  uint64_t rv= 0;
	  uint8_t x= 0;
	  for(x= 0; x < 8; x++){
	    rv= (rv << 8) | (val & 0xff);
	    val >>= 8;
	  }
	  return rv;
#endif

#if __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
		return val;
#endif

}


unsigned long long gnb_ntohll(unsigned long long val){

#if __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
	  uint64_t rv= 0;
	  uint8_t x= 0;
	  for(x= 0; x < 8; x++){
	    rv= (rv << 8) | (val & 0xff);
	    val >>= 8;
	  }
	  return rv;
#endif

#if __BYTE_ORDER__==__ORDER_BIG_ENDIAN__
	return val;
#endif

}


char get_netmask_class(uint32_t addr4){

	if (  !(~(IN_CLASSA_NET) &  ntohl(addr4))  ){
		return 'a';
	}

	if (  !(~(IN_CLASSB_NET) &  ntohl(addr4))  ){
		return 'b';
	}

	if (  !(~(IN_CLASSC_NET) &  ntohl(addr4))  ){
		return 'c';
	}

	return 'n';

}


gnb_address_list_t* gnb_create_address_list(size_t size){

	gnb_address_list_t *address_list;

	address_list = (gnb_address_list_t *)malloc( sizeof(gnb_address_list_t) + sizeof(gnb_address_t)*size );

	memset(address_list, 0, sizeof(gnb_address_list_t) + sizeof(gnb_address_t)*size);

	address_list->size = size;
	address_list->num = 0;

	return address_list;

}

void gnb_address_list_release(gnb_address_list_t *address_list){
	free(address_list);
}


int gnb_address_list_find(gnb_address_list_t *address_list, gnb_address_t *address) {

	int i;

	for ( i=0; i < address_list->num; i++ ) {

		if ( address->type != address_list->array[i].type ){
			continue;
		}

		if ( AF_INET == address->type && 0 == memcmp(address_list->array[i].m_address4, address->m_address4, 4) ){
			return i;
		}

		if ( AF_INET6 == address->type && 0 == memcmp(address_list->array[i].m_address6, address->m_address6, 16)){
			return i;
		}

	}

	return -1;
}


int gnb_address_list_get_free_idx(gnb_address_list_t *address_list) {

	int free_idx = 0;

	uint64_t min_ts_sec = address_list->array[free_idx].ts_sec;

	int i;

	for ( i=0; i < address_list->size; i++ ) {

		if ( 0 == address_list->array[i].port ){
			free_idx = i;
			break;
		}

		if ( address_list->array[i].ts_sec <= min_ts_sec ) {
			min_ts_sec = address_list->array[i].ts_sec;
			free_idx = i;
		}

	}

	return free_idx;

}


void gnb_address_list_update(gnb_address_list_t *address_list, gnb_address_t *address){

	int idx = 0;

	idx = gnb_address_list_find(address_list, address);

	if ( -1 != idx ){
		goto finish;
	}

	idx = gnb_address_list_get_free_idx(address_list);

finish:

if ( 0 == address_list->array[idx].port ){
		address_list->num += 1;
		if( address_list->num > address_list->size ){
			address_list->num = address_list->size;
		}
	}

	memcpy( &address_list->array[idx], address, sizeof(gnb_address_t) );

}


void gnb_set_address4(gnb_address_t *address, struct sockaddr_in *in){

	address->type = AF_INET;

	address->port = in->sin_port;

	memcpy(&address->address.addr4, &in->sin_addr.s_addr ,sizeof(struct in_addr));

}


void gnb_set_address6(gnb_address_t *address, struct sockaddr_in6 *in6){

	address->type = AF_INET6;
	address->port = in6->sin6_port;
	memcpy(&address->address.addr6, &in6->sin6_addr ,sizeof(struct in6_addr));

}


char * gnb_get_address4string(void *byte4, char *dest){
	inet_ntop(AF_INET, byte4, dest, INET_ADDRSTRLEN);
	return dest;
}


char * gnb_get_address6string(void *byte16, char *dest){
	inet_ntop(AF_INET6, byte16, dest, INET6_ADDRSTRLEN);
	return dest;
}


char * gnb_get_socket4string(struct sockaddr_in *in, char *dest){
	char buf[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &in->sin_addr, buf, INET_ADDRSTRLEN);
	snprintf(dest, GNB_IP_PORT_STRING_SIZE,"%s:%d",buf,ntohs(in->sin_port));
	return dest;
}


char * gnb_get_socket6string(struct sockaddr_in6 *in6, char *dest){
	char buf[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, &in6->sin6_addr, buf, INET6_ADDRSTRLEN);
	snprintf(dest, GNB_IP_PORT_STRING_SIZE,"[%s:%d]",buf,ntohs(in6->sin6_port));
	return dest;
}


char * gnb_get_sockaddress_string(gnb_sockaddress_t *sockaddress, char *dest){

	if ( AF_INET6 == sockaddress->addr_type ){
		dest = gnb_get_socket6string(&sockaddress->addr.in6,dest);
	}else if ( AF_INET == sockaddress->addr_type ){
		dest = gnb_get_socket4string(&sockaddress->addr.in,dest);
	}

	return dest;

}


char * gnb_get_ip_port_string(gnb_address_t *address, char *dest){

	char buf[INET6_ADDRSTRLEN];

	if ( AF_INET6 == address->type ){
		inet_ntop(AF_INET6, &address->address.addr6, buf, INET6_ADDRSTRLEN);
		snprintf(dest, GNB_IP_PORT_STRING_SIZE,"[%s:%d]",buf,ntohs(address->port));
	}else if( AF_INET == address->type ){
		inet_ntop(AF_INET, &address->address.addr4, buf, INET_ADDRSTRLEN);
		snprintf(dest, GNB_IP_PORT_STRING_SIZE,"%s:%d",buf,ntohs(address->port));
	}else{
		snprintf(dest, GNB_IP_PORT_STRING_SIZE,"NONE_ADDRESS");
	}

	return dest;
}


int gnb_cmp_sockaddr_in6(struct sockaddr_in6 *in1, struct sockaddr_in6 *in2){

	if ( in1->sin6_port != in2->sin6_port ){
		return 1;
	}

	if ( 0 != memcmp( &in1->sin6_addr, &in2->sin6_addr, 16) ){
		return 2;
	}

	return 0;

}


int gnb_cmp_sockaddr_in(struct sockaddr_in *in1, struct sockaddr_in *in2){

	if ( in1->sin_port != in2->sin_port ){
		return 1;
	}

	if ( in1->sin_addr.s_addr != in2->sin_addr.s_addr  ){
		return 2;
	}

	return 0;

}


void gnb_set_sockaddress4(gnb_sockaddress_t *sockaddress, int protocol, const char *host, int port){

    sockaddress->addr_type = AF_INET;

    if (GNB_PROTOCOL_TCP == protocol){
    	sockaddress->protocol = SOCK_STREAM;
    }else if (GNB_PROTOCOL_UDP == protocol){
    	sockaddress->protocol = SOCK_DGRAM;
    }else{
    	sockaddress->protocol = SOCK_DGRAM;
    }

    memset(&sockaddress->m_in4,0, sizeof(struct sockaddr_in));

    sockaddress->m_in4.sin_family = AF_INET;

    if ( NULL != host ){
    	inet_pton(AF_INET, host, (struct in_addr *)&sockaddress->m_in4.sin_addr.s_addr);
    }else{
    	sockaddress->m_in4.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    sockaddress->m_in4.sin_port = htons(port);

    sockaddress->socklen = sizeof(struct sockaddr_in);

}


void gnb_set_sockaddress6(gnb_sockaddress_t *sockaddress, int protocol, const char *host, int port){

	sockaddress->addr_type = AF_INET6;

    if (GNB_PROTOCOL_TCP == protocol){
    	sockaddress->protocol = SOCK_STREAM;
    }else if (GNB_PROTOCOL_UDP == protocol){
    	sockaddress->protocol = SOCK_DGRAM;
    }else{
    	sockaddress->protocol = SOCK_DGRAM;
    }

	memset(&sockaddress->m_in6,0, sizeof(struct sockaddr_in6));

	sockaddress->m_in6.sin6_family = AF_INET6;

    if ( NULL != host ){
    	inet_pton( AF_INET6, host, &sockaddress->m_in6.sin6_addr);
    }else{
    	sockaddress->m_in6.sin6_addr = in6addr_any;
    }

    sockaddress->m_in6.sin6_port = htons(port);

    sockaddress->socklen = sizeof(struct sockaddr_in6);

}


