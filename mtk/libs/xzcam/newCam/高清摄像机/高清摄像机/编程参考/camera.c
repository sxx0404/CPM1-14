struct FindPacket{
	UINT cmd;
	UINT id;
	UINT flag;
	UINT ip;
	WORD port;
	WORD rev1;
	BYTE rev2[24];
};

/************************************************************
ͨ������㲥�����ھ������з���������豸
����豸�յ�����㲥����Է��͹㲥��������Ӧ������ݰ�
���ݰ�����������������IP��ַ�Ͷ˿ں�
recvfrom(sock,(char*)&findpk,sizeof(findpk),0,(SOCKADDR*)&addr,&addlens);	
����ip��ֱַ��ȡrecvfrom ����ĵ�ַ���˿ں�ȡ�������port
************************************************************/
void BroadCast()
{
	SOCKADDR_IN addr;  
    memset(&addr, 0, sizeof(addr));  
    addr.sin_family = AF_INET;  
    addr.sin_addr.S_un.S_addr = htonl(INADDR_BROADCAST);  
    addr.sin_port = htons(9800);  
    BOOL bBoardcast = TRUE;  

	FindPacket packet;
	packet.cmd = 0x55AA0801;
	packet.ip  = ip;
	packet.port = port;

	if( SOCKET_ERROR == sendto(bsock,(char*)&packet, sizeof(packet), 0, (LPSOCKADDR)&addr, sizeof(addr)) ){  
		TRACE("sendto failed with error: %d/n", WSAGetLastError());  
    }else{
		TRACE("ok\n");
	}  
}

void RecvFined()
{
	/*����ط���UDP socket*/
	int lens = recvfrom(sock,(char*)&findpk,sizeof(findpk),0,(SOCKADDR*)&addr,&addlens);	
	if( lens == sizeof(findpk) )
	{
		if( findpk.cmd == 0x55AA0801 ){			
			//�����IP�Ͷ˿ں�
			//addr.sin_addr.s_addr
			//findpk.port �˿ں�			
		}
	}
}

/*****************************************************/
/*������tcp����,ȫ����һ������ sock*/

#pragma pack(push)  
#pragma pack(1)  
//�����ֽڶ���
struct COMMAND{
	u32 sync;
	u32 cmd;	
	u16 video_w;
	u16 video_h;	
	u32 bitrate;
	u32 seconds;
	u32 date;
	u32 rec_type;
	u16 fps;
	u16 gop;
	u16 max_qp;
	u16 min_qp;//6*5 30
	char filename[128];
	u32 rev[10];		
};
#pragma pack(pop) 
enum{
	CMD_STOP_RECORD = 0x0200,
	CMD_START_RECORD = 0x0201,
	
	CMD_STOP_REALVIDEO = 0x0300,
	CMD_START_REALVIDEO = 0x0301,
};

/*���������*/
int ConnectServer(UINT srvip, WORD srvport)
{
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if( sock < 0 )return -1;

	serverAddr.sin_addr.S_un.S_addr = srvip;//inet_addr("127.0.0.1");
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(srvport);
	return connect(sock, (SOCKADDR *)&serverAddr, sizeof(serverAddr));	

}

/*����ʵʱ��Ƶ*/
int StartRealVideo()
{

	struct COMMAND cmd;
	cmd.sync = 0x55AA0801;
	cmd.cmd = CMD_START_REALVIDEO;

	cmd.fps			= 25;
	cmd.bitrate		= 1000;
	cmd.gop			= 60;
	cmd.max_qp		= 38;
	cmd.min_qp		= 26;
	cmd.rec_type	= 2;
	cmd.video_w		= 1280;
	cmd.video_h		= 720;//������16��������
	
	return send(sock,(char*)&cmd,sizeof(cmd),0);	
}

/*¼������洢Ϊ�ļ�*/
int StartRecord()
{
	struct COMMAND cmd;
	CTime time1		= CTime::GetCurrentTime(); 
	int nTSeconds	= time1.GetTime();
	//nTSeconds +=  8*
	cmd.sync		= 0x55AA0801;
	cmd.cmd			= CMD_START_RECORD;	
	cmd.seconds		= 10;/*¼��ʱ�䳤��*/
	cmd.date		= nTSeconds;/*��ǰʱ�䣬������ 1970����*/

	cmd.fps			= 25;
	cmd.bitrate		= 1000;
	cmd.gop			= 60;
	cmd.max_qp		= 36;
	cmd.min_qp		= 26;
	cmd.rec_type	= 2;
	cmd.video_w		= 1280;
	cmd.video_h		= 720;//������16��������
	
	strcpy(cmd.filename,"test.h264");/*¼���ļ���*/

	return send(sock,(char*)&cmd,sizeof(cmd),0);	
}


int StopRealVideo()
{

	struct COMMAND cmd;
	cmd.sync = 0x55AA0801;
	cmd.cmd = CMD_STOP_REALVIDEO;
	return send(sock,(char*)&cmd,sizeof(cmd),0);	
}

int StopRecord()
{

	struct COMMAND cmd;
	cmd.sync = 0x55AA0801;
	cmd.cmd = CMD_STOP_RECORD;
	return send(sock,(char*)&cmd,sizeof(cmd),0);	
}




/*��ȡʵʱ����
����StartRealVideo��Ȼ����������ȡ������
*/

int GetVideoStream(){
	char buffer[512];
	char stream[1024*1024];
	int sync55_cnt = 0;

	while(1){
		/*ͬ������*/
		recvlens = recv(sock,buffer,512,0);
		if( recvlens <= 0 )continue;
		for(int i=0;i<recvlens;i++){
			if( buffer[i] != 0x55 ){				
				sync55_cnt = 0;
			}else{
				sync55_cnt++;
				if( sync55_cnt == 128 ){
					i++;
					goto end;
				}					
			}
		}
	}

end:
	
	if( sync55_cnt == 128 ){
		if( i < recvlens ){			
			/*����ط���һ����stream,����洢*/
			memcpy(stream,buffer+i,recvlens-i);
		}
	}

	while(1)
	{		
		/*��ȡh264��*/
		recvlens = recv(sock,stream,1024*1024,0);		
	}
}