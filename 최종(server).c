#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <process.h>
#pragma comment(lib,"ws2_32.lib")
#define BUFSIZE   1024
#define C2S_REQ_LOGIN  1001 // 로그인 요청 , 수신 코드
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
	//WSAData 생성
	windllLoad(&wsaData);	
	// memset(배열, 초기화값, 크기) 서버 주소값 초기화
	memset(&serv_addr, 0, sizeof(serv_addr)); 
	// IPv4 네트워크 바이트 변환 후 포트 번호 할당
	ServInit(&serv_sock, &serv_addr, atoi(argv[1]));
	// 쓰레드 동기화 시작
	initMutex();

	// 클라이언트 방 인원 수 정보 초기화
	for(i=0; i<CHAT_ROOMS; ++i)
		chatUsers[i].clntNumber = 0;
	// 바인딩 에러 체크
	if(bind(serv_sock, (SOCKADDR *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
		ErrorHandling("bind() Error!");
	// 리스닝 에러 체크
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
				printf("방 번호 에러\n");
				break;
			}
			
			room = atoi(room_num) - 1;
			printf("방번호 : %d\n", room+1);

			if(((room > -1) && (room < 6)) && (chatUsers[room].clntNumber < 10))
			{	// 멀티 쓰레드 생성
				WaitForSingleObject(hMutex, INFINITE);
				chatUsers[room].clntSocks[chatUsers[room].clntNumber++] = clnt_sock;
				// 뮤텍스 반납
				ReleaseMutex(hMutex);
				printf("새로운 연결, 클라이언트 IP : %s\n", inet_ntoa(clnt_addr.sin_addr));
				sprintf(room_num, "%d", room+1);
				send(clnt_sock, room_num, strlen(room_num), 0);
				// ClientConn 쓰레드 동작
				/* .cpp파일일 경우 : ClientConn → (unsigned int(__stdcall*)(void*))ClientConn */
				hThread = (HANDLE)_beginthreadex(NULL, 0, ClientConn, (void *)clnt_sock, 0, (unsigned *)&dwThreadID);
		     	//Thread 를 생성하는 함수에는 4가지 정도가 있다.
		 	    //CreateThread : WinAPI 전용으로 Win32에 종속적이기 때문에 return 값이 HANDLE이다
		     	 //_beginthread : CreateThread의 취약점을 보안
		    	 //_beginthreadex : return 값이 unsigned int
		        //AfxBeginThread : 내부적으로 VreateThread를 호출
		    	 //CWinThread : UI에 종속적
				if(hThread == 0)
					ErrorHandling("쓰레드 생성 오류!");

				break;
			}
			else
			{
				// 방이 꽉 찼다는 메시지와 함께 방 번호를 다시 입력하게 지시한다.
				if(chatUsers[room].clntNumber >= 10)
				{
					printf(" 해당 방이 꽉 찾습니다\n 사용자의 재입력을 기다립니다.\n");
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

		if(flag == 1) // 종료에 해당하는 클라이언트 소켓을 찾았으므로 빠져나감
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
	// 최대 2 바이트의 크기를 받을 수 있는 메시지 수신
	iret = recv(hSOCKET, message, 2, 0);

	if(iret <= 0)
	{
		if(iret == -1) 
			ErrorHandling(" 패킷 수신 에러!\n");

		return -1;
	}
	else
	{
		// 받은 메시지의 메모리값 코드에 복사
		memcpy(&code, &message[0], 2); 
		// 메모리 0으로 초기화
		memset(&message, 0x00, 2);
		// 메시지 수신
		recv(hSOCKET, message, 2, 0);
		// 받은 메시지의 메모리값 사이즈에 복사
		memcpy(&size, &message[0], 2);
		// 메모리 0으로 초기화
		memset(&message, 0x00, 2);
	}

	if(size > 0)
		recv(hSOCKET, message, size, 0);

	printf("데이터 수신 : code = %d, size = %d\n", code, size+4); 
	
	if(code == C2S_REQ_LOGIN)
	{
		printf("로그인 요청 수신\n"); 
		result = trxLogin(hSOCKET, message);  
	
		
		return result;
	}
	else
	{
		printf("패킷 코드 오류\n"); 
		return -1;
	}
}

void SendMSG(SOCKET clnt_sock, char *message, int len)
{
	int i, flag = 0, room = 0;

	WaitForSingleObject(hMutex, INFINITE);
	for(room=0; room<6; ++room) // 소켓이 포함된 Room Number 찾기
	{
		for(i=0; i<chatUsers[room].clntNumber; ++i)
		{
			if(clnt_sock == chatUsers[room].clntSocks[i])
			{
				// room의 값이 세팅되고 빠져 나간다.
				flag = 1;
				break; 
			}
		}

		if(flag == 1) // 해당 소켓이 포함된 Room Number를 찾음
			break;
	}

	// 찾은 룸 넘버의 모든 클라이언트들에게 메시지 전송
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
	// 로그인 정보 구조체에 메모리 할당
	memcpy(&login, buf, sizeof(login));

	printf("id: %s, password : %s\n", login.achID, login.achPWD);
	// 파일 입출력 복사
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
	// 아이디와 비밀번호가 맞는지 체크
	if(strcmp(achID, login.achID) != 0 || strcmp(achPWD, login.achPWD) != 0)
		iRes = -1; 

	res.iResult = iRes; // 0

	size = sizeof(res); // 0
	// 메모리 초기화
	memset(message, 0x00, BUFSIZE);
	// 코드값을 메시지에 복사 ( 이때 코드값은 응답요청 전송코드 2001 )
	memcpy(&message[0], &code, 2);
	// 메시지 배열 인덱스 2번에 size값 복사 ( 이때 사이즈는 1)
	memcpy(&message[2], &size, 2);
	// 메시지 배열 인덱스 4번에 res값 복사 ( 이때 res 는 0 )
	memcpy(&message[4], &res, size);
	// 메시지 전송(옵션x)
	iret = send(hSocket, message, size+4, 0);
	if(iret < 0)
		ErrorHandling("데이터를 보내지 못했습니다."); 
	// 로그인 성공
	if(res.iResult == 0)
		
		
	
		chatRoomInfo(hSocket);

	return res.iResult;
}

void chatRoomInfo(SOCKET hSocket)
{
	char buffer[BUFSIZE];
	char *tableLine = "==========================================================================\n";

	sprintf(buffer, "%s|  채팅방1 : %d/2\t |  채팅방2 : %d/2\t |  채팅방3 : %d/2\t |\n%s", tableLine, chatUsers[0].clntNumber, chatUsers[1].clntNumber, chatUsers[2].clntNumber, tableLine);
	// 버퍼 전송
	send(hSocket, buffer, strlen(buffer), 0);

	sprintf(buffer, "%s|  채팅방4 : %d/10\t |  채팅방5 : %d/10\t |  채팅방6 : %d/10\t |\n%s", tableLine, chatUsers[3].clntNumber, chatUsers[4].clntNumber, chatUsers[5].clntNumber, tableLine);
	// 버퍼 전송
	send(hSocket, buffer, strlen(buffer), 0);

}

void initMutex()
{
	//  뮤텍스 생성
	hMutex = CreateMutex(NULL, FALSE, NULL);
	if(hMutex == NULL)
		ErrorHandling("CreateMutex() Error!");
}

void windllLoad(WSADATA *wsaData)
{
	// 소켓 생성 시작
	if(WSAStartup(MAKEWORD(2, 2), wsaData) != 0)
		ErrorHandling("WSAStartup() Error!");
}

void ServInit(SOCKET *serv_sock, SOCKADDR_IN *serv_addr, int port)
{
	*serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(*serv_sock == INVALID_SOCKET)
		ErrorHandling("socket() Error!");

	serv_addr->sin_family = AF_INET;							// IPv4 프로토콜
	serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);		// 자기 자신의 IP 주소의 순서 정렬
	serv_addr->sin_port = htons((unsigned short int)port);	// 포트 번호 정렬 후 할당
}

void ErrorHandling(char *message)
{
	// 메시지 즉석 출력 가능 
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(-1);
}




