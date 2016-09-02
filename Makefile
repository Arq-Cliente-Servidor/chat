# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/utp/zmq/lib
ZMQ=/home/utp/zmq
CC = g++ -std=c++11 -I$(ZMQ)/include -L$(ZMQ)/lib

all: server client sound soundCapture # test

client: client.cc
		$(CC) -o client client.cc -lzmq -lzmqpp -lsfml-system -lsfml-audio -pthread

server: server.cc
		$(CC) -o server server.cc -lzmq -lzmqpp

soundCapture: examples/soundCapture.cc
	$(CC) -o examples/soundCapture examples/soundCapture.cc -lsfml-system -lsfml-audio

sound: examples/sound.cc
		$(CC) -o examples/sound examples/sound.cc -lsfml-system -lsfml-audio

test: test.cc
	$(CC) -o test test.cc -lzmq -lzmqpp

clean:
	rm -rf client server test
