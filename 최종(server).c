#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>
#pragma comment(lib,"ws2_32.lib")
#define BUFSIZE   1024
#define C2S_REQ_LOGIN  1001 // �α��� ��û , ���� �ڵ�
#define S2C_RES_LOGIN  2001 
#define CHAT_ROOMS  6
#define NAMESIZE 20

typedef struct ReqLogin
{
	char achID[20];
	char achPWD[20];
} ReqLogin;

typedef struct ResLogin
{
	int iResult;
} ResLogin;

typedef struct ChatClients
{
	int clntNumber;
	SOCKET clntSocks[10];
} ChatUsers;

DWORD WINAPI ClientConn(void *arg);
void   SendMSG(SOCKET clnt_sock, char *message, int len);
void   initMutex();
void   windllLoad(WSADATA *wsaData);
void   ServInit(SOCKET *serv_sock, SOCKADDR_IN *serv_addr, int port);
void   CheckLogin(SOCKET hSOCKET);
int    recvLoginPacket(SOCKET hSOCKET);
int    trxLogin(SOCKET hSocket, char *buf);
void   chatRoomInfo(SOCKET hSocket);
void   ErrorHandling(char *message);

int clntCnt = 0;
SOCKET clntSocks[10];	
ChatUsers chatUsers[CHAT_ROOMS];
HANDLE hMutex;

int main(int argc, char *argv[])
{
	WSADATA  wsaData;
	SOCKET  serv_sock, clnt_sock;
	SOCKADDR_IN serv_addr, clnt_addr;
	HANDLE  hThread;
	DWORD  dwThreadID;
	int   i, clnt_addr_size, room;
	char  room_num[BUFSIZE];

	if(argc != 2)
	{
		printf("Usage : %s <PORT>\n", argv[0]);
		exit(1);
	} 
	//WSAData ����
	windllLoad(&wsaData);	
	// memset(�迭, �ʱ�ȭ��, ũ��) ���� �ּҰ� �ʱ�ȭ
	memset(&serv_addr, 0, sizeof(serv_addr)); 
	// IPv4 ��Ʈ��ũ ����Ʈ ��ȯ �� ��Ʈ ��ȣ �Ҵ�
	ServInit(&serv_sock, &serv_addr, atoi(argv[1]));
	// ������ ����ȭ ����
	initMutex();

	// Ŭ���̾�Ʈ �� �ο� �� ���� �ʱ�ȭ
	for(i=0; i<CHAT_ROOMS; ++i)
		chatUsers[i].clntNumber = 0;
	// ���ε� ���� üũ
	if(bind(serv_sock, (SOCKADDR *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
		ErrorHandling("bind() Error!");
	// ������ ���� üũ
	if(listen(serv_sock, 5) == SOCKET_ERROR)
		ErrorHandling("listen() Error!");

	while(1)
	{
		clnt_addr_size = sizeof(clnt_addr);
		clnt_sock = accept(serv_sock, (SOCKADDR *)&clnt_addr, &clnt_addr_size);

		if(clnt_sock == INVALID_SOCKET)
		{
			printf("accept() Error!\n");
			continue;
		}

		CheckLogin(clnt_sock);

		while(1)
		{
			if(recv(clnt_sock, room_num, BUFSIZE, 0) == -1)
			{
				printf("�� ��ȣ ����\n");
				break;
			}
			
			room = atoi(room_num) - 1;
			printf("���ȣ : %d\n", room+1);

			if(((room > -1) && (room < 6)) && (chatUsers[room].clntNumber < 10))
			{	// ��Ƽ ������ ����
				WaitForSingleObject(hMutex, INFINITE);
				chatUsers[room].clntSocks[chatUsers[room].clntNumber++] = clnt_sock;
				// ���ؽ� �ݳ�
				ReleaseMutex(hMutex);
				printf("���ο� ����, Ŭ���̾�Ʈ IP : %s\n", inet_ntoa(clnt_addr.sin_addr));
				sprintf(room_num, "%d", room+1);
				send(clnt_sock, room_num, strlen(room_num), 0);
				// ClientConn ������ ����
				/* .cpp������ ��� : ClientConn �� (unsigned int(__stdcall*)(void*))ClientConn */
				hThread = (HANDLE)_beginthreadex(NULL, 0, ClientConn, (void *)clnt_sock, 0, (unsigned *)&dwThreadID);
		     	//Thread �� �����ϴ� �Լ����� 4���� ������ �ִ�.
		 	    //CreateThread : WinAPI �������� Win32�� �������̱� ������ return ���� HANDLE�̴�
		     	 //_beginthread : CreateThread�� ������� ����
		    	 //_beginthreadex : return ���� unsigned int
		        //AfxBeginThread : ���������� VreateThread�� ȣ��
		    	 //CWinThread : UI�� ������
				if(hThread == 0)
					ErrorHandling("������ ���� ����!");

				break;
			}
			else
			{
				// ���� �� á�ٴ� �޽����� �Բ� �� ��ȣ�� �ٽ� �Է��ϰ� �����Ѵ�.
				if(chatUsers[room].clntNumber >= 10)
				{
					printf(" �ش� ���� �� ã���ϴ�\n ������� ���Է��� ��ٸ��ϴ�.\n");
					sprintf(room_num, "%d", -1);
					send(clnt_sock, room_num, strlen(room_num), 0);
				}
			}
		}
	}

	closesocket(serv_sock);
	WSACleanup();

	return 0;
}

DWORD WINAPI ClientConn(void *arg)
{
	SOCKET clnt_sock = (SOCKET)arg;
	int str_len = 0, i, room, flag;
	char message[BUFSIZE];

	while((str_len = recv(clnt_sock, message, BUFSIZE, 0)) != EOF)
		SendMSG(clnt_sock, message, str_len);

	WaitForSingleObject(hMutex, INFINITE);
	for(room=0; room<6; ++room)
	{
		flag = 0;
		for(i=0; i<chatUsers[room].clntNumber; ++i)
		{
			if(clnt_sock == chatUsers[room].clntSocks[i])
			{
				flag = 1;
				for( ; i<chatUsers[room].clntNumber-1; ++i)
					chatUsers[room].clntSocks[i] = chatUsers[room].clntSocks[i+1];

				break;
			}
		}

		if(flag == 1) // ���ῡ �ش��ϴ� Ŭ���̾�Ʈ ������ ã�����Ƿ� ��������
			break;
	}
	chatUsers[room].clntNumber--; 

	ReleaseMutex(hMutex);
	closesocket(clnt_sock);

	return 0;
}

void CheckLogin(SOCKET hSOCKET)
{
	while(1)
	{
		if(recvLoginPacket(hSOCKET) == 0)
			break;
	}
}
void ClientInfo(SOCKET hSOCKET)
{




}
int recvLoginPacket(SOCKET hSOCKET)
{
	char message[BUFSIZE] = {0, };
	short code = 0, size = 0;
	int iret, result;
	char buffer[BUFSIZE] = {0, };
	// �ִ� 2 ����Ʈ�� ũ�⸦ ���� �� �ִ� �޽��� ����
	iret = recv(hSOCKET, message, 2, 0);

	if(iret <= 0)
	{
		if(iret == -1) 
			ErrorHandling(" ��Ŷ ���� ����!\n");

		return -1;
	}
	else
	{
		// ���� �޽����� �޸𸮰� �ڵ忡 ����
		memcpy(&code, &message[0], 2); 
		// �޸� 0���� �ʱ�ȭ
		memset(&message, 0x00, 2);
		// �޽��� ����
		recv(hSOCKET, message, 2, 0);
		// ���� �޽����� �޸𸮰� ����� ����
		memcpy(&size, &message[0], 2);
		// �޸� 0���� �ʱ�ȭ
		memset(&message, 0x00, 2);
	}

	if(size > 0)
		recv(hSOCKET, message, size, 0);

	printf("������ ���� : code = %d, size = %d\n", code, size+4); 
	
	if(code == C2S_REQ_LOGIN)
	{
		printf("�α��� ��û ����\n"); 
		result = trxLogin(hSOCKET, message);  
	
		
		return result;
	}
	else
	{
		printf("��Ŷ �ڵ� ����\n"); 
		return -1;
	}
}

void SendMSG(SOCKET clnt_sock, char *message, int len)
{
	int i, flag = 0, room = 0;

	WaitForSingleObject(hMutex, INFINITE);
	for(room=0; room<6; ++room) // ������ ���Ե� Room Number ã��
	{
		for(i=0; i<chatUsers[room].clntNumber; ++i)
		{
			if(clnt_sock == chatUsers[room].clntSocks[i])
			{
				// room�� ���� ���õǰ� ���� ������.
				flag = 1;
				break; 
			}
		}

		if(flag == 1) // �ش� ������ ���Ե� Room Number�� ã��
			break;
	}

	// ã�� �� �ѹ��� ��� Ŭ���̾�Ʈ�鿡�� �޽��� ����
	for(i=0; i<chatUsers[room].clntNumber; ++i)
		send(chatUsers[room].clntSocks[i], message, len, 0);

	ReleaseMutex(hMutex);
}


int trxLogin(SOCKET hSocket, char *buf)
{
	ReqLogin login;
	ResLogin res;
	FILE *fp1, *fp2;
	char achID[20];
	char achPWD[20];
	char message[BUFSIZE];
	char buffer[20];
	char cnt[20];
	int iRes, size, iret, code = S2C_RES_LOGIN;
	iRes = size = iret = 0;

	if((fp1 = fopen("id.txt","r")) == NULL )
	{
		printf("No File\n");
	}

	if((fp2 = fopen("pwd.txt","r")) == NULL )
	{
		printf("No File\n");
	}
	// �α��� ���� ����ü�� �޸� �Ҵ�
	memcpy(&login, buf, sizeof(login));

	printf("id: %s, password : %s\n", login.achID, login.achPWD);
	// ���� ����� ����
	while (fgets(buffer, sizeof(buffer), fp1))
	{
		if( strstr(buffer,login.achID))
		{
			strcpy(achID,login.achID);
			break;
		}
	}

	while (fgets(buffer, sizeof(buffer), fp2))
	{
		if( strstr(buffer,login.achPWD))
		{
			strcpy(achPWD,login.achPWD);
			break;
		}
	}
	fclose(fp1);
	fclose(fp2);
	// ���̵�� ��й�ȣ�� �´��� üũ
	if(strcmp(achID, login.achID) != 0 || strcmp(achPWD, login.achPWD) != 0)
		iRes = -1; 

	res.iResult = iRes; // 0

	size = sizeof(res); // 0
	// �޸� �ʱ�ȭ
	memset(message, 0x00, BUFSIZE);
	// �ڵ尪�� �޽����� ���� ( �̶� �ڵ尪�� �����û �����ڵ� 2001 )
	memcpy(&message[0], &code, 2);
	// �޽��� �迭 �ε��� 2���� size�� ���� ( �̶� ������� 1)
	memcpy(&message[2], &size, 2);
	// �޽��� �迭 �ε��� 4���� res�� ���� ( �̶� res �� 0 )
	memcpy(&message[4], &res, size);
	// �޽��� ����(�ɼ�x)
	iret = send(hSocket, message, size+4, 0);
	if(iret < 0)
		ErrorHandling("�����͸� ������ ���߽��ϴ�."); 
	// �α��� ����
	if(res.iResult == 0)
		
		
	
		chatRoomInfo(hSocket);

	return res.iResult;
}

void chatRoomInfo(SOCKET hSocket)
{
	char buffer[BUFSIZE];
	char *tableLine = "==========================================================================\n";

	sprintf(buffer, "%s|  ä�ù�1 : %d/2\t |  ä�ù�2 : %d/2\t |  ä�ù�3 : %d/2\t |\n%s", tableLine, chatUsers[0].clntNumber, chatUsers[1].clntNumber, chatUsers[2].clntNumber, tableLine);
	// ���� ����
	send(hSocket, buffer, strlen(buffer), 0);

	sprintf(buffer, "%s|  ä�ù�4 : %d/10\t |  ä�ù�5 : %d/10\t |  ä�ù�6 : %d/10\t |\n%s", tableLine, chatUsers[3].clntNumber, chatUsers[4].clntNumber, chatUsers[5].clntNumber, tableLine);
	// ���� ����
	send(hSocket, buffer, strlen(buffer), 0);

}

void initMutex()
{
	//  ���ؽ� ����
	hMutex = CreateMutex(NULL, FALSE, NULL);
	if(hMutex == NULL)
		ErrorHandling("CreateMutex() Error!");
}

void windllLoad(WSADATA *wsaData)
{
	// ���� ���� ����
	if(WSAStartup(MAKEWORD(2, 2), wsaData) != 0)
		ErrorHandling("WSAStartup() Error!");
}

void ServInit(SOCKET *serv_sock, SOCKADDR_IN *serv_addr, int port)
{
	*serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(*serv_sock == INVALID_SOCKET)
		ErrorHandling("socket() Error!");

	serv_addr->sin_family = AF_INET;							// IPv4 ��������
	serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);		// �ڱ� �ڽ��� IP �ּ��� ���� ����
	serv_addr->sin_port = htons((unsigned short int)port);	// ��Ʈ ��ȣ ���� �� �Ҵ�
}

void ErrorHandling(char *message)
{
	// �޽��� �Ｎ ��� ���� 
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(-1);
}




