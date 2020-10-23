# Client-Server

Assigment
Client - Server application for understanding UDP/TCP protocols and multiplexing.
We have a UDP client who sends a lot of messages to different topics, a server which
sends the messages to TCP clients subscribed to those topics and a client who can
subscribe to topics and quit.

The main difficulty was to convert UPD packet to TCP and manage subscribers.
I used 3 structures for UDP packets, for TCP packets and for subscribers.
I also use a map to link topics with subscribers.

