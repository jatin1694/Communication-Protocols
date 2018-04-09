CC=gcc
OPT=-o
CMP=$(CC) $(OPT)
SENDER_FILE=Sender.c
RECEIVER_FILE=Receiver.c
SENDER_OUT=Sender
RECEIVER_OUT=Receiver

build-client: $(SENDER_FILE)
				$(CMP) $(SENDER_OUT) $(SENDER_FILE)
build-server: $(RECEIVER_FILE)
				$(CMP) $(RECEIVER_OUT) $(RECEIVER_FILE)
build: build-client build-server


