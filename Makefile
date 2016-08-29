# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/utp/zmq/lib
ZMQ=/home/utp/zmq
CC = g++ -std=c++11 -I$(ZMQ)/include -L$(ZMQ)/lib

all: client server soundCapture # test

client: client.cc
		$(CC) -o client client.cc -lzmq -lzmqpp

server: server.cc
		$(CC) -o server server.cc -lzmq -lzmqpp

soundCapture: examples/soundCapture.cc
	$(CC) -o examples/soundCapture examples/soundCapture.cc -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio

# test: test.cc
# 	$(CC) -o test test.cc -lzmq -lzmqpp
