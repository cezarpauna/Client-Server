CC=g++ -std=c++11 -Wall

all: server subscriber

server: server.cpp
	$(CC) server.cpp -o server

subscriber: subscriber.cpp
	$(CC) subscriber.cpp -o subscriber

clean:
	rm server subscriber
