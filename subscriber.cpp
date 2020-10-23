#include "utils.h"

int main(int argc, char **argv) {

	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[PACKET_LEN];
	packet_tcp *receive;
	server_tcp send_pkt;

	// bad arguments
	if (argc < 4) {
		cout << "Usage: ./subscriber ID IP_SERVER PORT\n";
		exit(0);
	}

	// bad ID
	if (strlen(argv[1]) > 10) {
		cout << "ID too long, max 10\n";
		exit(0);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	// socket err
	DIE(sockfd < 0, "socket error\n");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	// inet_aton err
	DIE(ret == 0, "inet_aton error\n");

	ret = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	// connection err
	DIE(ret < 0, "connect\n");

	// file descriptors for server and stdin
	fd_set read_fds, tmp_fds;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	FD_SET(sockfd, &read_fds);
	FD_SET(0, &read_fds);

	int fdmax = sockfd;
	char read[BUFLEN];

	ret = 1;
	// deactivate Neagle alg
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &ret, sizeof(int));

	// send subscriber's id
	ret = send(sockfd, argv[1], strlen(argv[1]) + 1, 0);
	DIE(ret < 0, "no ID\n");

	while (1) {
		tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select\n");

		// stdin
		if (FD_ISSET(0, &tmp_fds)) {
			memset(buffer, 0, BUFLEN);
			cin >> read;
			if (!strncmp(read, "exit", 4)) {
				break;
			} else if (!strncmp(read, "subscribe", 9)) {
				send_pkt.type = 1;
			} else if (!strncmp(read, "unsubscribe", 11)) {
				send_pkt.type = 2;
			} else {
				// debug
				cout << "d";
				continue;
			}

			cin >> read;
			if (strlen(read) > 50) {
				cout << "topic too long\n";
				continue;
			}

			strcpy(send_pkt.topic, read);

			if (send_pkt.type == 1) {
				cin >> read;
				if (read[0] != '0' && read[0] != '1') {
					cout << "sf=1 || sf=0\n";
					continue;
				}
				send_pkt.sf = atoi(&read[0]);
			}

			n = send(sockfd, (char*) &send_pkt, sizeof(send_pkt), 0);
			DIE(n < 0, "send_pkt error\n");

			if (send_pkt.type == 1) {
				cout << "Te-ai abonat la: " << send_pkt.topic << 
				" cu optiunea sf = " << send_pkt.sf << "\n";
			} else if (send_pkt.type == 2) {
				cout << "Dezabonare de la: " << send_pkt.topic << "\n";
			}
		}

		// packet from server
		if (FD_ISSET(sockfd, &tmp_fds)) {
			memset(buffer, 0, BUFLEN);
			n = recv(sockfd, buffer, BUFLEN, 0);
			DIE(n < 0, "recv\n");
			if (n == 0) {
				break;
			}

			receive = (packet_tcp *)buffer;
			cout << receive->ip << ":" << receive->udp_port <<
					" - " << receive->topic << " - " <<
					receive->type << " - " << receive->payload << "\n";
		}
	}

	close(sockfd);
	return 0;
}