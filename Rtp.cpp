#include "Rtp.h"

using namespace std;

char *FileTemp;
char *RtpHeader;
char *FUIndicator;
char *FUHeader;

void *Rtp(void *fileName)
{
	int sockFD;
	struct sockaddr_in addrClient;
	//RtpHeader相關參數
	int SequenceNumber = 26074;
	unsigned int timestamp = 2785272748;
	unsigned int ssrc = 0xc630ebd7;
	//FileTemp檔相關參數
	char *FrameStartIndex;
	int FrameLength = 0;
	int FileSize;

	printf("I'm at Rtp()!\n");
	
	//建立RtpSocket
	createRtpSocket(&sockFD,&addrClient);
	//開啟H.264影像編碼檔並取得檔案大小
	FileSize = OpenVideoFile((char*)fileName);
	//創造與設定RTP標頭檔
	createRtpHeader();
	setRtpVersion(); 
	setRtpPayloadType();
	setSequenceNumber(SequenceNumber);
	setTimestamp(timestamp);
	setSSRC(ssrc);

	printf("Rtplock=%d\n",lock);
	for(int i=0;i<FileSize && lock;i++){
		//H.264 StartCode 為00 00 00 01或00 00 01
		if(*((int*)(FileTemp+i))==0x01000000){//轉型為4bytes
			//if((*((int*)(FileTemp+i))&0x00FFFFFF)==0x00010000) continue;
			if(FrameLength>0)
				RtpEncoder(sockFD,addrClient,FrameStartIndex,FrameLength,&SequenceNumber,&timestamp);
			FrameStartIndex = FileTemp + i;
			FrameLength = 0;
			i++;//StartCode(0x00010000)
			FrameLength++;
			//printf("FrameStartIndex=%X\n",*((int*)FrameStartIndex));
		}else if((*((int*)(FileTemp+i))&0x00FFFFFF)==0x00010000){
			if(FrameLength>0)
				RtpEncoder(sockFD,addrClient,FrameStartIndex,FrameLength,&SequenceNumber,&timestamp);
			FrameStartIndex = FileTemp + i;
			FrameLength = 0;
			//printf("FrameStartIndex!!!=%X\n",*((int*)FrameStartIndex));
		}
		FrameLength++;
	}
	printf("Rtplock=%d\n",lock);
	printf("End\n");
	close(sockFD);//關閉Socket
	free(FileTemp);
	free(RtpHeader);
	free(FUIndicator);
	free(FUHeader);
}

void createRtpSocket(int *sockFD,sockaddr_in *addrClient)
{
	struct sockaddr_in addrServer;
	int addrClientLen = sizeof(addrClient);

	//建立Socket
	*sockFD = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if(*sockFD == -1){
		perror("Socket failed!!\n");
		exit(0);
	} 
	//Server Socket 位址
	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.s_addr = INADDR_ANY;
	addrServer.sin_port = htons(RtpParameter.rtpServerPort);
	//Client Socket 位址
	addrClient->sin_family = AF_INET;
	addrClient->sin_addr.s_addr = RtpParameter.addrClient.sin_addr.s_addr;
	addrClient->sin_port = htons(RtpParameter.rtpClientPort);

	//printf("IP=%s\n",inet_ntoa(addrClient->sin_addr));
	//printf("Port=%d\n",ntohs(addrClient->sin_port));
	if(bind(*sockFD,(sockaddr*)&addrServer,sizeof(addrServer)) == -1){
		perror("Bind failed!\n");
		close(*sockFD);//關閉Socket
		exit(0);
	}
}
int OpenVideoFile(char *fileName)
{
	FILE *fPtr;
	int FileSize;

	fPtr = fopen(fileName,"rb");
	if(!fPtr){
		perror("Open failed!");
		exit(0);
	}
	//find the fileSize
	fseek(fPtr,0,SEEK_END);
	FileSize = ftell(fPtr);
	fseek(fPtr,0,SEEK_SET);
	//printf("fileSize=%d bytes\n",FileSize);
	
	FileTemp = (char*)malloc(FileSize*sizeof(char));
	//複製檔案內容
	if(fPtr)
		fread(FileTemp,1,FileSize,fPtr);
	else 
		perror("File Open Error!"),exit(0);

	fclose(fPtr);
	return FileSize;
}
//與RtpHeader有關的參數與設定
void createRtpHeader()
{
	int RtpHeaderSize = 12;
	RtpHeader = (char*)malloc(RtpHeaderSize*sizeof(char));
	bzero(RtpHeader,RtpHeaderSize);
}
void setRtpVersion()
{
	RtpHeader[0] |= 0x80;
}
void setRtpPayloadType()
{
	RtpHeader[1] |= PayloadType;
}
void setSequenceNumber(int SequenceNumber)
{
	RtpHeader[2] &=0;
	RtpHeader[3] &=0;
	//取得上半部8bits的位元並將int轉型為char存進RtpHeader裡
	RtpHeader[2] |= (char)((SequenceNumber & 0x0000FF00)>>8);
	//取得下半部8bits的位元並將int轉型為char存進RtpHeader裡
	RtpHeader[3] |= (char)(SequenceNumber & 0x000000FF);
	//printf("%X\n%X\n", RtpHeader[2],RtpHeader[3]);
}
void setTimestamp(unsigned int timestamp)
{
	RtpHeader[4] &= 0;
	RtpHeader[5] &= 0;
	RtpHeader[6] &= 0;
	RtpHeader[7] &= 0;
	RtpHeader[4] |= (char)((timestamp & 0xff000000) >> 24);
	RtpHeader[5] |= (char)((timestamp & 0x00ff0000) >> 16);
	RtpHeader[6] |= (char)((timestamp & 0x0000ff00) >> 8);
	RtpHeader[7] |= (char)(timestamp & 0x000000ff);
	/*printf("setTimestamp=%X\n", RtpHeader[4] );
	printf("%X\n", RtpHeader[5] );
	printf("%X\n", RtpHeader[6] );
	printf("%X\n", RtpHeader[7] );*/
}
void setSSRC(unsigned int ssrc)
{
	RtpHeader[8]  &= 0;
	RtpHeader[9]  &= 0;
	RtpHeader[10] &= 0;
	RtpHeader[11] &= 0;
	RtpHeader[8]  |= (char) ((ssrc & 0xff000000) >> 24);
	RtpHeader[9]  |= (char) ((ssrc & 0x00ff0000) >> 16);
	RtpHeader[10] |= (char) ((ssrc & 0x0000ff00) >> 8);
	RtpHeader[11] |= (char) (ssrc & 0x000000ff);
	/*printf("%X\n", RtpHeader[8] );
	printf("%X\n", RtpHeader[9] );
	printf("%X\n", RtpHeader[10] );
	printf("%X\n", RtpHeader[11] );*/
}
void setMarker(int marker)
{
	if(marker == 0){
		RtpHeader[1] &= 0x7f;
	}else{
		RtpHeader[1] |= 0x80;
	}
}

//生成傳輸Rtp封包所需格式
void RtpEncoder(int sockFD,sockaddr_in addrClient,char *FrameStartIndex,int FrameLength,int *SequenceNumber,unsigned int *timestamp)
{
	//先將長度扣除StartCode部分
	if(*((int*)FrameStartIndex) == 0x01000000) FrameLength -= 4;
	else  FrameLength -= 3;

	//原始封包規格[Start code][NALU][Raw Data]
	if(FrameLength < 1400){ 
		//封包長度小於MTU(減去其他層Header)
		char *sendBuf = (char*)malloc((FrameLength+12)*sizeof(char));
		
		//[RTP Header][NALU][Raw Date]
		if(*((int*)FrameStartIndex) == 0x01000000) 
			memcpy(sendBuf+12,FrameStartIndex+4,FrameLength);
		else
			memcpy(sendBuf+12,FrameStartIndex+3,FrameLength);

		//WiredShark顯示以67 68開頭的資料Mark等於0
		if(sendBuf[12] == 0x67 || sendBuf[12] == 0x68){
			setMarker(0);
			memcpy(sendBuf,RtpHeader,12);
		}else{
			setMarker(1);
			memcpy(sendBuf,RtpHeader,12);
			//設定timestamp以(90000/fps)遞增
			(*timestamp) += (90000/fps);
			setTimestamp(*timestamp);
		}
		
		if(sendto(sockFD,sendBuf,FrameLength+12,0,(sockaddr *)&addrClient,sizeof(addrClient)) == -1){
			printf("Sent failed!!\n");
			close(sockFD);//關閉Socket
			free(sendBuf);
		}
		usleep(sleepTime);
		//封包傳輸序列遞增
		(*SequenceNumber)++;
		setSequenceNumber(*SequenceNumber);
		free(sendBuf);
		//printf("FrameLength=%d\n",FrameLength);
		//printf("FrameStartIndex1=%X\n",*((int*)FrameStartIndex));
	}else{
		//[RTP Header][FU indicator][FU header][Raw Data]
		FUIndicator = (char*)malloc(sizeof(char));
		FUHeader = (char*)malloc(sizeof(char));
		if(FrameLength >= 1400){
			setMarker(0);
			setFUIndicator(FrameStartIndex);
			setFUHeader(FrameStartIndex,1,0);//第一個分段包
			char *sendBuf = (char *)malloc(1500*sizeof(char));
			memcpy(sendBuf,RtpHeader,12);//[RTP Header]
			memcpy(sendBuf+12,FUIndicator,1);//[FU indicator]
			memcpy(sendBuf+13,FUHeader,1);//[FU header]

			if(FrameStartIndex[3] == 0x01){//00 00 00 01
				memcpy(sendBuf+14,FrameStartIndex+5,1386);//[raw data]
				FrameLength -= 1387;//包含NALU(1 Byte)
				FrameStartIndex += 1391;//包含startCode(4 Bytes)和NALU(1 Byte)
				//printf("FrameStartIndex2=%X\n",*((int*)FrameStartIndex));
			}else if(FrameStartIndex[2] == 0x01){//00 00 01
				memcpy(sendBuf+14,FrameStartIndex+4,1386);//[raw data]
				FrameLength -= 1387;
				FrameStartIndex += 1390;//包含startCode(3 Bytes)和NALU(1 Byte)
				//printf("FrameStartIndex3=%X\n",*((int*)FrameStartIndex));
			}
			if(sendto(sockFD,sendBuf,1400,0,(sockaddr *)&addrClient,sizeof(addrClient)) == -1){
				printf("Sent failed!!\n");
				close(sockFD);//關閉Socket
				free(sendBuf);
			}
			usleep(sleepTime);
			//封包傳輸序列遞增
			(*SequenceNumber)++;
			setSequenceNumber(*SequenceNumber);
			
			while(FrameLength >= 1400){
				setFUIndicator(FrameStartIndex);
				setFUHeader(FrameStartIndex,0,0);
				memcpy(sendBuf,RtpHeader,12);//[RTP Header]
				memcpy(sendBuf+12,FUIndicator,1);//[FU indicator]
				memcpy(sendBuf+13,FUHeader,1);//[FU header]
				memcpy(sendBuf+14,FrameStartIndex,1386);
				FrameLength -= 1386;
				FrameStartIndex +=1386;
				//printf("FrameStartIndex4=%X\n",*((int*)FrameStartIndex));

				if(sendto(sockFD,sendBuf,1400,0,(sockaddr *)&addrClient,sizeof(addrClient)) == -1){
					printf("Sent failed!!\n");
					close(sockFD);//關閉Socket
					free(sendBuf);
				}
				usleep(sleepTime);
				//封包傳輸序列遞增
				(*SequenceNumber)++;
				setSequenceNumber(*SequenceNumber);
			}
			if(FrameLength > 0){ //資料分段的最後一個封包
				setFUIndicator(FrameStartIndex);
				setFUHeader(FrameStartIndex,0,1);
				setMarker(1);
				memcpy(sendBuf,RtpHeader,12);//[RTP Header]
				memcpy(sendBuf+12,FUIndicator,1);//[FU indicator]
				memcpy(sendBuf+13,FUHeader,1);//[FU header]
				memcpy(sendBuf+14,FrameStartIndex,FrameLength);
				if(sendto(sockFD,sendBuf,FrameLength+14,0,(sockaddr *)&addrClient,sizeof(addrClient)) == -1){
					printf("Sent failed!!\n");
					close(sockFD);//關閉Socket
					free(sendBuf);
				}
				usleep(sleepTime);
				//printf("FrameStartIndex5=%X\n",*((int*)FrameStartIndex));
				//設定timestamp以(90000/fps)遞增
				(*timestamp) += (90000/fps);
				setTimestamp(*timestamp);
				//封包傳輸序列遞增
				(*SequenceNumber)++;
				setSequenceNumber(*SequenceNumber);
			}
			free(sendBuf);
		}
	}	
}
void setFUIndicator(char *FrameStartIndex)
{
	int FUIndicatorType = 28;
	
	if(FrameStartIndex[2] == 0x01)
		FUIndicator[0] = (FrameStartIndex[3] & 0x60) | FUIndicatorType;
	else if(FrameStartIndex[3] == 0x01)
		FUIndicator[0] = (FrameStartIndex[4] & 0x60) | FUIndicatorType;
}
void setFUHeader(char *FrameStartIndex,bool start,bool end)
{
	if(FrameStartIndex[3] == 0x01)
		FUHeader[0] = FrameStartIndex[4] & 0x1f;
	else if(FrameStartIndex[2] == 0x01)
		FUHeader[0] = FrameStartIndex[3] & 0x1f;
	//設定FU-A Header的封包是第一個分段包或中間包或最後一包
	/*  
	   |0|1|2|3 4 5 6 7|
	   |S|E|R|  TYPE   | 
	*/
	if(start) FUHeader[0] |= 0x80;
	else FUHeader[0] &= 0x7f;
	if(end) FUHeader[0] |= 0x40;
	else FUHeader[0] &= 0xBF;
}
