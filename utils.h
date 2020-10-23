#ifndef _UTILS_H_
#define _UTILS_H_

#include <iostream>
#include <utility>
#include <vector>
#include <unordered_map>
#include <math.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <unistd.h>

using namespace std;

#define PACKET_LEN 1501
#define TOPIC_LEN 50
#define TYPE_ID_MAX 11
#define ID_MAX 11
#define BUFLEN sizeof(packet_tcp)

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

// structura pentru client UDP
struct packet_udp {
	char topic[TOPIC_LEN];
	uint8_t type;
	char payload[PACKET_LEN];
};

// structura packet trimis la un client TCP
struct packet_tcp {
	char ip[16];
	uint16_t udp_port;
	char topic[TOPIC_LEN];
	char type[TYPE_ID_MAX];
	char payload[PACKET_LEN];
};

// structura packet trimis la server de un client TCP
struct server_tcp {
	char topic[TOPIC_LEN];
	unsigned char type;
	int sf;
};

#endif








