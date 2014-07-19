# Which compiler
CC = g++
#Where the library
LIBS    = -lpthread

RtspServer:Main.cpp Rtsp.cpp Rtp.cpp Rtsp.h Rtp.h
	$(CC) -o $@ $? $(LIBS) 
