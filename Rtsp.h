#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>

#ifndef _RTSP_H_
 #define _RTSP_H

	#define BUF_SIZE 1024
	#define RtspServerPort 8554
	#define RtpServerPort 50000
	void Rtsp(char *fileName);
	void OPTIONS_Reply(int clientFD);
	void DESCRIBE_Reply(int clientFD,char *RtspContentBase);
	void SETUP_Reply(int clientFD);
	void PLAY_Reply(int clientFD,sockaddr_in addrClient,char *RtspUrl,char *fileName);
	void GET_PARAMETER_Reply(int clientFD);
	void TEARDOWN_Reply(int clientFD);
	void createRtspSocket(int *serverFD,int *clientFD,sockaddr_in *addrClient);
#endif
