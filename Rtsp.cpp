#include "Rtsp.h"
#include "Rtp.h"

using namespace std;

//RTSP傳輸所需檔案
char *RtspResponse = "RTSP/1.0 200 OK\r\n";
char *RtspServer = "Server: VLC\r\n";
char *RtspCachControl = "Cache-Control: no-cache\r\n";
char *RtspSession = "Session: ee62ba70a1ddca;timeout=60\r\n";
string RtspContentLength = "Content-Length: \r\n";
string RtspCseq = "Cseq: \r\n";
string RtpClientPort;
int RtspCseqNumber = 2;
//RTP Thread use
bool lock;
struct RtpData RtpParameter;
//RTSP的傳收BUFFER
char recvBuf[BUF_SIZE];
char sendBuf[BUF_SIZE];


void Rtsp(char *fileName)
{
	//存放OPTIONS DESCRIBE SETUP PLAY TEARDOWN等
	char *RequestType;
	char *RtspUrl;
	//TCP連線所需
	int serverFD,clientFD;//Server and Client SocketID
	struct sockaddr_in addrClient;
	socklen_t addrClientLen = sizeof(addrClient);
	int retVal;

	printf("fileName=%s\n",fileName);
	//RtspSocket
	createRtspSocket(&serverFD,&clientFD,&addrClient);

	while(1)
	{
		retVal = recv(clientFD,recvBuf,BUF_SIZE,0);
		if(retVal == -1){
			printf("Received failed!!\n");
			close(serverFD);//關閉Socket
			close(clientFD);//關閉Socket
			exit(0);
		}
		RequestType = (char*) malloc(strlen(recvBuf)+1);
		strcpy(RequestType,recvBuf);
		strtok(RequestType," ");
		RtspUrl = strtok(NULL," ");
		//printf("RequestType=%s,RtspUrl=%s\n",RequestType,RtspUrl);
		//printf("RecvBuf :\n%s",recvBuf);
		if(!strcmp(RequestType,"OPTIONS"))
			OPTIONS_Reply(clientFD);
		else if(!strcmp(RequestType,"DESCRIBE"))
			DESCRIBE_Reply(clientFD,RtspUrl);
		else if(!strcmp(RequestType,"SETUP"))
			SETUP_Reply(clientFD);
		else if(!strcmp(RequestType,"PLAY"))
			PLAY_Reply(clientFD,addrClient,RtspUrl,fileName);
		else if(!strcmp(RequestType,"GET_PARAMETER"))
			GET_PARAMETER_Reply(clientFD);
		else if(!strcmp(RequestType,"TEARDOWN"))
			TEARDOWN_Reply(clientFD);
		else
		{
			close(clientFD);//關閉Socket
			printf("Server is listening.....\n");
			clientFD = accept(serverFD,(struct sockaddr*)&addrClient,&addrClientLen);
			if(clientFD == -1){
				perror("Accept failed!!\n");
				close(serverFD);//關閉Socket
				exit(0);
			}else printf("succeed!!\n");
			RtspCseqNumber = 1;
		}
		RtspCseqNumber++;
		free(RequestType);	
	}
	free(fileName);
	system("pause");
	exit(1);

}

void createRtspSocket(int *serverFD,int *clientFD,sockaddr_in *addrClient)
{
	struct sockaddr_in addrServer;
	socklen_t addrClientLen = sizeof(*addrClient);

	//建立Socket
	*serverFD = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	//Server Socket IP
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.s_addr = INADDR_ANY;
	addrServer.sin_port = htons(RtspServerPort);

	//綁定Socket
	if(bind(*serverFD,(sockaddr*)&addrServer,sizeof(addrServer)) == -1){
		perror("Bind failed!\n");
		close(*serverFD);//關閉Socket
		exit(0);
	}
	//建立監聽
	if(listen(*serverFD,10) == -1){
		perror("Listen failed!!\n");
		close(*serverFD);//關閉Socket
		exit(0);
	}
	//接收客戶端請求
	printf("Server is listening.....");
	*clientFD = accept(*serverFD,(sockaddr*)addrClient,&addrClientLen);
	if(*clientFD == -1){
		perror("Accept failed!!\n");
		close(*serverFD);//關閉Socket
		exit(0);
	}
	else printf("succeed!!\n");
}
char *int2str(int i)
{
	char *s;
	s = (char *)malloc(i);
	sprintf(s,"%d",i);
	return s;
}
int str2int(string temp)
{
	return atoi(temp.c_str());
}
string getRtpClientPort()
{
	string temp;
	string retVal;
	int begin,end;

	printf("\n******getRtpClientPort******\n");
	temp.assign(recvBuf);
	begin = temp.find("client_port=")+12;
	end = begin + 11;

	retVal.append(temp.begin()+begin,temp.begin()+end);
	printf("retVal=%s\n",retVal.c_str());

	return retVal;
}
void createRtpThread(char* fileName)
{
	int res;
	pthread_t tid;
	void *thread_result;

	res = pthread_create(&tid,NULL,Rtp,(void*)fileName);
	if(res!=0){
		perror("Thread creation failed");
		exit(EXIT_FAILURE);
	}
}
void OPTIONS_Reply(int clientFD)
{
	char *RtspPublic = "Public: DESCRIBE,SETUP,TEARDOWN,PLAY,PAUSE,GET_PARAMETER\r\n\r\n";
	string temp;

	printf("\n*****OPTIONS*****\n");
	temp.insert(0,RtspResponse);
	temp.insert(strlen(temp.c_str()),RtspServer);
	temp.insert(strlen(temp.c_str()),RtspContentLength);
	temp.insert(strlen(temp.c_str())-2,"0");
	temp.insert(strlen(temp.c_str()),RtspCseq);
	temp.insert(strlen(temp.c_str())-2,int2str(RtspCseqNumber));
	temp.insert(strlen(temp.c_str()),RtspPublic);
	free(int2str(RtspCseqNumber));

	//printf("Send Temp:\n%s",temp.c_str());

	strcpy(sendBuf,temp.c_str());
	if(send(clientFD,sendBuf,strlen(sendBuf),0) == -1){
		printf("Sent failed1!!\n");
		close(clientFD);//關閉Socket
		exit(0);
	}
	bzero(sendBuf,BUF_SIZE);

}
void DESCRIBE_Reply(int clientFD,char *RtspContentBase)
{
	char *RtspContentType = "Content-type: application/sdp\r\n";
	char *SDPFile = "v=0\r\no=- 15409869162442327530 15409869162442327530 IN IP4 ESLab-PC\r\n"
					"s=Unnamed\r\ni=N/A\r\nc=IN IP4 0.0.0.0\r\nt=0 0\r\na=tool:vlc 2.0.7\r\n"
					"a=recvonly\r\na=type:broadcast\r\na=charset:UTF-8\r\n"
					"a=control:rtsp://192.168.0.186:8554/trackID=0\r\nm=video 0 RTP/AVP 96\r\n"
					"b=RR:0\r\na=rtpmap:96 H264/90000\r\na=fmtp:96 packetization-mode=1;profile-level-id=64001f;sprop-parameter-sets=Z2QAH6zZQFAFuwEQACi7EAmJaAjxgjlg,aOvjyyLA;\r\n"
					"a=control:rtsp://163.13.133.191:8554/trackID=1\r\n";
				    "Date: Wed, 15 May 2013 12:10:17 GMT\r\nContent-type: application/sdpContent-Base: rtsp://192.168.0.186:8554/\r\n"
					"Content-length: 362Cache-Control: no-cache\r\nCseq: 3\r\n\r\n"
					"v=0\r\no=- 15365712008849713956 15365712008849713956 IN IP4 User-PC\r\ns=Unnamed\r\ni=N/A\r\n"
					"c=IN IP4 0.0.0.0\r\nt=0 0\r\na=tool:vlc 2.0.3\r\na=recvonly\r\na=type:broadcast\r\na=charset:UTF-8\r\n"
					"a=control:rtsp://192.168.0.186:8554/\r\na=framerate:100\r\nm=video 0 RTP/AVP 96\r\nb=RR:0\r\n"
					"a=rtpmap:96 H264/90000\r\na=fmtp:96 packetization-mode=1\r\na=control:rtsp://192.168.0.186:8554/trackID=1\r\n";

	string temp;

	printf("\n*****DESCRIBE*****\n");
	temp.insert(0,RtspResponse);
	temp.insert(strlen(temp.c_str()),RtspServer);
	temp.insert(strlen(temp.c_str()),RtspContentType);
	temp.insert(strlen(temp.c_str()),"Content-Base: ");
	temp.insert(strlen(temp.c_str()),RtspContentBase);
	temp.insert(strlen(temp.c_str()),"\r\n");
	temp.insert(strlen(temp.c_str()),RtspContentLength);
	temp.insert(strlen(temp.c_str())-2,int2str(strlen(SDPFile)));
	temp.insert(strlen(temp.c_str()),RtspCachControl);
	temp.insert(strlen(temp.c_str()),RtspCseq);
	temp.insert(strlen(temp.c_str())-2,int2str(RtspCseqNumber));
	temp.insert(strlen(temp.c_str()),"\r\n");
	temp.insert(strlen(temp.c_str()),SDPFile);
	free(int2str(strlen(SDPFile)));
	free(int2str(RtspCseqNumber));

	//printf("Send temp:\n%s",temp.c_str());
	strcpy(sendBuf,temp.c_str());
	if(send(clientFD,sendBuf,strlen(sendBuf),0) == -1){
		printf("Sent failed2!!\n");
		close(clientFD);//關閉Socket
		exit(0);
	}
	bzero(sendBuf,BUF_SIZE);
}
void SETUP_Reply(int clientFD)
{
	char *RtspTransport = "Transport: RTP/AVP/UDP;unicast;";
	char *ssrc = ";ssrc=15F6B7CF;";
	char *mode = "mode=play\r\n";
	string RTPServerPort = ";server_port=50000-50001";
	string temp;

	RtpClientPort = getRtpClientPort();
	printf("\n*****SETUP*****\n");
	temp.insert(0,RtspResponse);
	temp.insert(strlen(temp.c_str()),RtspServer);
	temp.insert(strlen(temp.c_str()),RtspTransport);
	temp.insert(strlen(temp.c_str()),"client_port=");
	temp.insert(strlen(temp.c_str()),RtpClientPort);
	temp.insert(strlen(temp.c_str()),RTPServerPort);
	temp.insert(strlen(temp.c_str()),ssrc);
	temp.insert(strlen(temp.c_str()),mode);
	temp.insert(strlen(temp.c_str()),RtspSession);
	temp.insert(strlen(temp.c_str()),RtspContentLength);
	temp.insert(strlen(temp.c_str())-2,"0");
	temp.insert(strlen(temp.c_str()),RtspCachControl);
	temp.insert(strlen(temp.c_str()),RtspCseq);
	temp.insert(strlen(temp.c_str())-2,"4");
	temp.insert(strlen(temp.c_str()),"\r\n");

	//printf("Send temp:\n%s",temp.c_str());

	strcpy(sendBuf,temp.c_str());
	if(send(clientFD,sendBuf,strlen(sendBuf),0) == -1){
		printf("Sent failed3!!\n");
		close(clientFD);//關閉Socket
		exit(0);
	}
	//ZeroMemory(sendBuf,BUF_SIZE);
	
}
void PLAY_Reply(int clientFD,sockaddr_in addrClient,char *RtspUrl,char *fileName)
{
	char *RTPInfo = "RTP-Info: url=";
	char *seq = ";seq=5873;";
	char *rtptime = "rtptime=2217555422\r\n";
	char *Range = "Range: npt=10-\r\n";
	string temp;
	//RTP所需參數
	//struct RtpData *RtpParameter;
	//RtpParameter = (RtpData*)malloc(sizeof(RtpData));

	printf("\n*****Play*****\n");
	temp.insert(0,RtspResponse);
	temp.insert(strlen(temp.c_str()),RtspServer);
	temp.insert(strlen(temp.c_str()),RTPInfo);
	temp.insert(strlen(temp.c_str()),RtspUrl);
	temp.insert(strlen(temp.c_str()),seq);
	temp.insert(strlen(temp.c_str()),rtptime);
	temp.insert(strlen(temp.c_str()),Range);
	temp.insert(strlen(temp.c_str()),RtspSession);
	temp.insert(strlen(temp.c_str()),RtspContentLength);
	temp.insert(strlen(temp.c_str())-2,"0");
	temp.insert(strlen(temp.c_str()),RtspCachControl);
	temp.insert(strlen(temp.c_str()),RtspCseq);
	temp.insert(strlen(temp.c_str())-2,int2str(RtspCseqNumber));
	temp.insert(strlen(temp.c_str()),"\r\n");
	free(int2str(RtspCseqNumber));

	//printf("Send temp:\n%s",temp.c_str());
	strcpy(sendBuf,temp.c_str());
	if(send(clientFD,sendBuf,strlen(sendBuf),0) == -1){
		printf("Sent failed4!!\n");
		close(clientFD);//關閉Socket
		exit(0);
	}
	lock = 1;
	bzero(sendBuf,BUF_SIZE);
	//送完PLAY_Reply後開始進行RTP傳輸
	RtpParameter.addrClient = addrClient;
	RtpParameter.rtpServerPort = RtpServerPort;
	RtpParameter.rtpClientPort = str2int(RtpClientPort);
	createRtpThread(fileName);
}
void GET_PARAMETER_Reply(int clientFD)
{
	string temp;

	printf("\n*****GET_PARAMETER*****\n");

	temp.insert(0,RtspResponse);
	temp.insert(strlen(temp.c_str()),RtspServer);
	temp.insert(strlen(temp.c_str()),RtspSession);
	temp.insert(strlen(temp.c_str()),RtspContentLength);
	temp.insert(strlen(temp.c_str())-2,"0");
	temp.insert(strlen(temp.c_str()),RtspCachControl);
	temp.insert(strlen(temp.c_str()),RtspCseq);
	temp.insert(strlen(temp.c_str())-2,int2str(RtspCseqNumber));
	temp.insert(strlen(temp.c_str()),"\r\n");
	free(int2str(RtspCseqNumber));

	//printf("Send temp:\n%s",temp.c_str());
	strcpy(sendBuf,temp.c_str());
	if(send(clientFD,sendBuf,strlen(sendBuf),0) == -1){
		printf("Sent failed0000!!\n");
		close(clientFD);//關閉Socket
		exit(0);
	}
	bzero(sendBuf,BUF_SIZE);
}
void TEARDOWN_Reply(int clientFD)
{
	string temp;

	printf("\n*****TEARDOWN*****\n");
	temp.insert(0,RtspResponse);
	temp.insert(strlen(temp.c_str()),RtspServer);
	temp.insert(strlen(temp.c_str()),RtspSession);
	temp.insert(strlen(temp.c_str()),RtspContentLength);
	temp.insert(strlen(temp.c_str())-2,"0");
	temp.insert(strlen(temp.c_str()),RtspCachControl);
	temp.insert(strlen(temp.c_str()),RtspCseq);
	temp.insert(strlen(temp.c_str())-2,int2str(RtspCseqNumber));
	temp.insert(strlen(temp.c_str()),"\r\n");
	free(int2str(RtspCseqNumber));

	//printf("Send temp:\n%s",temp.c_str());
	strcpy(sendBuf,temp.c_str());
	if(send(clientFD,sendBuf,strlen(sendBuf),0) == -1){
		printf("Sent failed5!!\n");
		close(clientFD);//關閉Socket
		exit(0);
	}
	lock = 0;
	bzero(sendBuf,BUF_SIZE);
	bzero(recvBuf,BUF_SIZE);
}
