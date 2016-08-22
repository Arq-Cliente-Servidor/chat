# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/utp/zmq/lib
ZMQ=/home/utp/zmq
CC = g++ -std=c++11 -I$(ZMQ)/include -L$(ZMQ)/lib

all: client server

client: client.cc
		$(CC) -o client client.cc -lzmq -lzmqpp

server: server.cc
		$(CC) -o server server.cc -lzmq -lzmqpp
