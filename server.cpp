#include "utils.h"

int main(int argc, char **argv) {

	int tcp_sockfd, udp_sockfd, newsockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr, udp_addr;
	int n, i, ret;
	socklen_t clilen, udp_len;

	/**
	 * I used a map to store as key - name of the topic and
	 * a vector of subscribers to make an easier acces to users.
	 * subscriber vector is a list of pairs (subscriber's socket,
	 * subscriber id)
	*/
	unordered_map<string, vector<pair<int, string>>> topics;

	/**
	* connected TCP clients
	*/
	unordered_map<int, string> subs;

	fd_set read_fds;	// read data for select()
	fd_set tmp_fds;		// temporary file descriptor
	int fdmax;			// maximum fd from read_fds

	if (argc < 2) {
		cout << "Usage: ./server port\n";
		exit(0);
	}

	// empty file descriptor set
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(tcp_sockfd < 0, "socket");
	udp_sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(udp_sockfd < 0, "socket");

	portno = atoi(argv[1]);
	DIE(portno < 1024, "atoi");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	udp_addr.sin_family = AF_INET;
	udp_addr.sin_port = htons(portno);
	udp_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(tcp_sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");
	ret = bind(udp_sockfd, (struct sockaddr *) &udp_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = listen(tcp_sockfd, 1000);
	DIE(ret < 0, "listen");

	// listening socket
	FD_SET(tcp_sockfd, &read_fds);
	FD_SET(udp_sockfd, &read_fds);
	fdmax = tcp_sockfd;
	FD_SET(0, &read_fds);

	char read[BUFLEN];
	int ok = 4;
	// from TCP client to server
	server_tcp *server_pkt;
	// from server to TCP client
	packet_tcp packet;
	// from UDP client
	packet_udp *cli_pkt;

	while (ok != 10) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == tcp_sockfd) {
					// connection from TCP client
					clilen = sizeof(cli_addr);
					newsockfd = accept(i, (sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					// add new socket
					FD_SET(newsockfd, &read_fds);
					// update max
					if (newsockfd > fdmax) {
						fdmax = newsockfd;
					}

					ret = 1;
					// deactivate Neagle algoirthm
					setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &ret, sizeof(int));

					n = recv(newsockfd, buffer, sizeof(buffer), 0);
					DIE(n < 0, "ID");
					cout << buffer << " " << newsockfd << "\n";
					
					cout << "Noua conexiune de la " << inet_ntoa(cli_addr.sin_addr) <<
						", port " << ntohs(cli_addr.sin_port) << ", socket client "
						<< newsockfd << "\n";
					
					// new TCP client
					subs.insert ({newsockfd, buffer});

				} else if (i == udp_sockfd) {
					// UDP client message
					udp_len = sizeof(udp_addr);
					n = recvfrom(udp_sockfd, buffer, 1552, 0,(sockaddr *) &udp_addr, &udp_len);
					DIE(n < 0, "nada\n");
					
					// received packet
					cli_pkt = (packet_udp *)buffer;
					// make TCP packet
					strcpy(packet.ip, inet_ntoa(udp_addr.sin_addr));
					packet.udp_port = ntohs(udp_addr.sin_port);

					/**
					* we need to make a TCP packet to send
					* to all subscribers of this topic
					*/
					if (cli_pkt->type > 3) {
						cout << "not good\n";
					} else {
						strcpy(packet.topic, cli_pkt->topic);
						packet.topic[50] = '\0';
						if (cli_pkt->type == 0) {
							
							long long int numero = ntohl(*(uint32_t *)(cli_pkt->payload + 1));
							// sign bit
							if (cli_pkt->payload[0]) {
								numero *= -1;
							} else {
								if (cli_pkt->payload[0] > 1) {
									cout << "nu e bun semnul\n";
								}
							}
							strcpy(packet.type, "INT");
							sprintf(packet.payload, "%lld", numero);
						} else if (cli_pkt->type == 1) {
							
							double no = ntohs(*(uint16_t *)(cli_pkt->payload));
							no /= 100;
							strcpy(packet.type, "SHORT_REAL");
							sprintf(packet.payload, "%.2f", no);
						} else if (cli_pkt->type == 2) {
							
							float no2 = ntohl(*(uint32_t *) (cli_pkt->payload + 1));
							no2 /= pow(10, cli_pkt->payload[5]);
							if (cli_pkt->payload[0]) {
								no2 *= -1;
							} else {
								if (cli_pkt->payload[0] > 1) {
									cout << "nu e bun semnul\n";
								}
							}
							strcpy(packet.type, "FLOAT");
							sprintf(packet.payload, "%lf", no2);

						} else {
							
							strcpy(packet.type, "STRING");
							strcpy(packet.payload, cli_pkt->payload);
							packet.payload[strlen(packet.payload) + 1] = '\0';
						}
					
						// send packet to subscribers
						if (topics.find(packet.topic) != topics.end()) {
							for (auto i : topics[packet.topic]) {
								n = send (i.first, (char *) &packet, sizeof(packet), 0);
								DIE(n < 0, "client");
							}
						}
					}
				} else if (i == 0) {
					
					cin >> read;
					if (!strncmp(read, "exit", 4)) {
						ok = 10;
						break;
					} else {
						cout << "not valide\n";
					}
				} else {
					
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "nada");

					if (n == 0) {
						cout << "Socket-ul client " << i << " cu id-ul "
							<< subs[i] << " a inchis conexiunea\n";
						
						subs.erase(i);
						
						for (auto& j : topics) {
							for (auto& k : j.second) {
								if (k.first == i) {
									j.second.erase(j.second.begin() + i);
								}
							}
						}
						FD_CLR(i, &read_fds);
						close(i);
					} else {
						// subscribe/unsubscribe
						server_pkt = (server_tcp *)buffer;
						if (server_pkt->type == 1) {
							// subscribe
							if (topics.size() == 0) {
								// initialize map
								vector<pair<int, string>> v;
								v.push_back(make_pair(i, subs[i]));
								topics.insert(make_pair(server_pkt->topic, v));
							} else {
								// not empty map
								if (topics.find(server_pkt->topic) == topics.end()) {
									// insert user
									vector<pair<int, string>> v;
									v.push_back(make_pair(i, subs[i]));
									topics.insert(make_pair(server_pkt->topic, v));
								} else {
									// insert subscriber
									topics.at(server_pkt->topic).push_back(make_pair(i, subs[i]));	
								}
							}
						} else if (server_pkt->type == 2) {
							//unsubscribe
							if (topics.find(server_pkt->topic) == topics.end()) {
								// topic not found
								cout << "error\n";
							} else {
								topics.at(server_pkt->topic).erase(topics.at(server_pkt->topic).begin() + i);
							}
						} else {
							// debug
							cout << "d\n";
						}
					}
				}
			}
		}
	}

	close(tcp_sockfd);
	close(udp_sockfd);
	return 0;
}