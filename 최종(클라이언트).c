#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <process.h>
#include <conio.h>
#include <windows.h>
#pragma comment(lib,"ws2_32.lib")

//--------------------------------
// Packet CODE
//--------------------------------
#define C2S_REQ_LOGIN   1001   //클라이언트가 서버에게 로그인을 요청한다.
#define S2C_RES_LOGIN   2001   //서버가 클라이언트에게 로그인 요청 응답을 전송한다.
//--------------------------------
//로그인 요청 데이터 구조체
typedef struct ReqLogin
{  
	char  achID[20];  //아이디
	char  achPW[20];  //비번
}ReqLogin;

typedef struct ResLogin
{ 
	int  iResult;   // 오류 여부 ( 0 이면 로그인 성공)
}ResLogin;

#define BUFSIZE 1024
#define NAMESIZE 20
#define ROOMNUM  6 
#define BACKSPACE 8
#define ENTER     13

//--------------------------------
//함수
DWORD WINAPI SendMSG(void *arg);
DWORD WINAPI RecvMSG(void *arg);
void sendPacket( SOCKET hSocket, int code, int size, void *pData);
int recvPacket(SOCKET hSocket);
int reqLogin(SOCKET hSocket);
int checkLogin(SOCKET hSocket, char *buf);
void main_print();
void roomInfo(SOCKET hSocket);
void Enter_room(SOCKET sock);
void gotoxy(int x, int y);
void Coord(void);
void ErrorHandling(char *message);
void FriendList();

//--------------------------------
//전역 변수
char name[NAMESIZE]="[Default]";
char message[BUFSIZE];
char room[ROOMNUM];
char Allchat[8][BUFSIZE+BUFSIZE];

//메인 함수
int main(int argc, char **argv)
{
	char cursor;
	int inp;
	WSADATA wsaData;
	SOCKET sock;

	SOCKADDR_IN servAddr;

	HANDLE hThread1,hThread2;
	DWORD dwThreadID1,dwThreadID2;

	if(argc!=4){
		printf("Usage : %s <IP> <port> <name> \n", argv[0]);

	}

	if(WSAStartup(MAKEWORD(2,2), &wsaData)!=0)
		ErrorHandling("WSAStartup() error!");
	sprintf(name, "[%s]", argv[3]);

	sock=socket(PF_INET, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET)
		ErrorHandling("socket() error!");

	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family=AF_INET;
	servAddr.sin_addr.s_addr=inet_addr(argv[1]);
	servAddr.sin_port=htons((unsigned short int)atoi(argv[2]));

	if(connect(sock, (SOCKADDR*)&servAddr,sizeof(servAddr))==SOCKET_ERROR)
		ErrorHandling("connect() error!");

	while(1)
	{
		//로그인 처리
		if(reqLogin(sock)==0)
			break;



	}
start:
	main_print();
	gotoxy(55,18); // 메뉴 1번 선택

	while(1){
		printf("\b");
reset3:
		cursor=getch(); 
		if(cursor>='1' && cursor<='3') 
			printf("%c", cursor); 
		else 
			goto reset3;
		inp=getch();
		if(inp==ENTER){
			switch(cursor){ 
			case '1' : 
				roomInfo(sock);
				Enter_room(sock);
				Coord();
				break;
			case '2' :
				FriendList();
				Sleep(3000);
				system("cls");
				goto start;
			case '3':
				Sleep(1000);
				system("cls");
				printf("정상적으로 종료되었습니다.");
				Sleep(1000);
				exit(0); 
				break;
			}
			break;

		}

	}


	hThread1 = (HANDLE)_beginthreadex(NULL, 0, (unsigned int(__stdcall*)(void*))SendMSG, (void*)sock, 0, (unsigned*)&dwThreadID1);
	hThread2 = (HANDLE)_beginthreadex(NULL, 0, (unsigned int(__stdcall*)(void*))RecvMSG, (void*)sock, 0, (unsigned*)&dwThreadID2);

	if(hThread1==0 || hThread2==0){
		ErrorHandling("쓰레드 생성 오류");
	}

	WaitForSingleObject(hThread1, INFINITE);
	WaitForSingleObject(hThread2, INFINITE);

	closesocket(sock);
	return 0;
}
//로그인 성공 시 메인 화면 출력
void main_print(){
   printf("\n\n");
   printf("       ■■■  ■    ■     ■■    ■■■■ ■■■  ■■   ■   ■■■■   \n");
   printf("     ■■     ■      ■  ■■■■     ■      ■   ■■■  ■  ■      ■  \n");
   printf("     ■       ■      ■ ■      ■    ■      ■   ■  ■  ■ ■           \n");
   printf("    ■        ■■■■■ ■■■■■    ■      ■   ■  ■  ■ ■           \n");
   printf("     ■       ■      ■ ■      ■    ■      ■   ■  ■  ■ ■   ■■■  \n");
   printf("     ■■     ■      ■ ■      ■    ■      ■   ■  ■■■  ■      ■  \n");
   printf("       ■■■  ■    ■   ■    ■     ■    ■■■ ■   ■■    ■■■■   \n");
   printf("\n\n");
   printf("   ■□■□■□■□■□■□■□■□■□■□■□■□■□■□■□■□■□■□ \n");
   printf("   □                                                                    ■ \n");
   printf("   ■                                                                    □ \n");
   printf("   □          1. 채팅하기                                               ■ \n");
   printf("   ■                                        ☞ SELECT  NUMBER ☜        □ \n");
   printf("   □                                                                    ■ \n");
   printf("   ■          2. 친구목록                      ┌─────┐           □ \n");
   printf("   □                                           │          │           ■ \n");
   printf("   ■                                           └─────┘           □ \n");
   printf("   □          3. 채팅종료                                               ■ \n");
   printf("   ■                                                                    □ \n");
   printf("   □                                                                    ■ \n");
   printf("   ■□■□■□■□■□■□■□■□■□■□■□■□■□■□■□■□■□■□ \n");

}
//메세지 전송 함수 
DWORD WINAPI SendMSG(void *arg)
{
	SOCKET sock = (SOCKET)arg;
	char nameMessage[NAMESIZE+BUFSIZE]={0,};
	char log[5]={0,};
	char ch=0;
	char Log[BUFSIZE]={0,};
	int i=0,j=2;

	memset(message,0,BUFSIZE);
	sprintf(Log, "%s 님이 입장하셨습니다. \n", name);
	send(sock, Log, strlen(Log), 0);
	memset(log,0,BUFSIZE);


	while(1){


		memset(message,0,BUFSIZE);
		gotoxy(2,20);
		printf("%s",message);
		gotoxy(2,20);

		for(;;)
		{



			ch = getche();


			message[j-2]=ch;

			//BACKSPACE 기능 구현부

			if(ch==BACKSPACE)
			{

				if(j<=2){
					gotoxy(j,20);

				}

				else{

					gotoxy( j-1,20);
					printf(" ");
					j--;

					gotoxy(j,20);

				}

				continue;

			}

			//문자열 입력의 제한

			if(j==64){

				gotoxy(j,20);
				printf(" ");
				j--;
				gotoxy(j,20);

			}

			//엔터 문자 처리부

			if(ch==ENTER)
			{
				gotoxy(2,20);
				j=2;
				break;
			}


			j++;




		}


		//채팅방 퇴장

		if(message[0]=='/' && message[1] == 'q')
		{


			gotoxy(2,20);
			printf(" 로그 아웃 하시겠습니까? (y/n)");
			Sleep(500);

			scanf("%s",log);

			Sleep(500);


			if(!strcmp(log, "y"))
			{
				gotoxy(2,20);
				sprintf(log, "%s 님이 퇴장 하셨습니다. \n\n\n\n", name);
				send(sock, log, strlen(log), 0);
				Sleep(1000);
				system("cls");
				closesocket(sock);
				exit(0);

			}

			else if(!strcmp(log, "n"))
			{
				gotoxy(2,20);
				printf(" 로그아웃 취소, 메세지를 입력하세요. \n");
				memset(message,0,BUFSIZE);
				Sleep(800);

				gotoxy(2,20);
				printf("                                         %s" ,message);
				gotoxy(2,20);
				memset(nameMessage,0,BUFSIZE);
				continue;
			}

		}


		sprintf(nameMessage, "%s %s\n", name, message); 
		send(sock, nameMessage, strlen(nameMessage), 0);

	}
}


//메세지 수신 함수
DWORD WINAPI RecvMSG(void *arg)
{
	SOCKET sock = (SOCKET)arg;
	char nameMessage[NAMESIZE+BUFSIZE];

	int strLen,j,i=0;


	Coord();


	while(1){



		gotoxy(2,20);

		memset(nameMessage,0,sizeof(nameMessage));
		printf("%s",nameMessage);
		strLen = recv(sock, nameMessage, NAMESIZE+BUFSIZE-1, 0);

		//서버와의 연결 종료
		if(strLen == -1)
			ErrorHandling("서버와 연결이 종료되었습니다. \n\n\n\n");
		gotoxy(2,26);


		nameMessage[strLen]=0;


		//메세지 출력부 
		if(i==0)
		{
			Coord();
			gotoxy(3,3);

			fputs(nameMessage, stdout);
			strncpy(Allchat[i-1],nameMessage,sizeof(nameMessage));

		}


		else if (i==1)
		{
			Coord();
			gotoxy(3,3);
			sprintf(Allchat[i-1],"%s\n   %s\n",Allchat[i-2],nameMessage);
			fputs(Allchat[i-1], stdout);
			for(j=1;j<=23;j++)
			{
				gotoxy(0,j);
				printf(".");
			}
		}


		else
		{
			Coord();
			gotoxy(3,3);
			sprintf(Allchat[i-1],"%s   %s\n",Allchat[i-2],nameMessage);
			fputs(Allchat[i-1], stdout);
			for(j=1;j<=23;j++)
			{
				gotoxy(0,j);
				printf(".");
			}
		}



		i++;

		if(i==8)
		{
			i=0;
			continue;
		}

	}
}

//패킷 전송 함수 
void sendPacket( SOCKET sock, int code, int size, void *pData)
{ 
	int iret=0;
	char sendBuf[BUFSIZE]; 

	//패킷 데이터 설정
	memset(sendBuf,0x00,BUFSIZE); 
	memcpy(&sendBuf[0],&code, 2);      
	memcpy(&sendBuf[2],&size, 2);   
	memcpy(&sendBuf[4], pData, size);  

	//패킷 전송
	iret = send(sock, sendBuf, size+4, 0);
	if(iret<0)
		ErrorHandling(" 데이터를 보내지 못했습니다.");

}


//패킷 수신 함수
int recvPacket(SOCKET sock)
{
	char buf[BUFSIZE] = {0, };
	short code=0;
	short size=0;
	int iret;


	//1. 패킷 종류 읽기
	iret = recv(sock, buf, 2,0);

	//읽어온 데이터 길이가 0보다 같거나 작으면, 소켓 연결이 클라이언트의 사정으로 불안정하게 끊어진 것이기때문에 클라이언트와의 연결을 해제한다.
	if (iret <= 0)  
	{  
		if( iret == -1 )   
			ErrorHandling("패킷 수신에러");
	}
	else
	{  
		//1. 패킷 종류 수신이 성공인 경우  설정
		memcpy(&code,&buf[0], 2);  
		memset(&buf,0x00,2);

		//2. 데이터 길이 읽기  
		recv(sock, buf, 2,0);      
		memcpy(&size,&buf[0],2);
		memset(&buf,0x00,2);

		//3. 데이터 읽기
		if( size > 0 )   
			recv(sock, buf, size,0);


		switch(code)
		{
		case S2C_RES_LOGIN: 
			checkLogin(sock, buf);
			break;


		default :
			printf("패킷 코드 오류\n");
			return -1;
		}
	}
	return 0;
}

//로그인 요청 함수 
int reqLogin(SOCKET sock)
{
	int randcolor = 0;

	//패킷 관련 선언  
	ReqLogin login;  

	Sleep(800);

	system("cls");
printf("\n");
   printf("         ■■        ■■   ■■■ ■     ■■  ■■   ■  ■  ■■■ \n");
   printf("         ■■        ■■   ■ ■  ■    ■    ■  ■ ■■■■ ■ ■  \n");
   printf("         ■■        ■■   ■■■ ■■■ ■■  ■■  ■ ■ ■ ■■■ \n");
   printf("         ■■   ■   ■■                                             \n");
   printf("         ■■  ■■  ■■              ■■■ ■■                    \n");
   printf("          ■■ ■■ ■■                 ■  ■  ■                   \n");
   printf("            ■■  ■■                   ■   ■■                    \n");
   printf("\n");
   printf("      ■■ ■  ■  ■■ ■■■ ■■■ ■■■ ■■■ ■  ■ ■■■ ■■■ \n");
   printf("     ■    ■  ■ ■  ■  ■   ■     ■     ■  ■ ■  ■ ■     ■  ■ \n");
   printf("     ■    ■■■ ■■■  ■   ■■■ ■■■ ■■■ ■  ■ ■■■ ■■■ \n");
   printf("     ■    ■  ■ ■  ■  ■       ■ ■     ■ ■   ■■  ■     ■ ■  \n");
   printf("      ■■ ■  ■ ■  ■  ■   ■■■ ■■■ ■  ■   ■   ■■■ ■  ■ \n");
   printf("\n\n");
   printf("      ○○○○○              ○○○○○ \n");
   printf("      ○●●●○              ○●●●○ \n");
   printf("      ○●●●○              ○●●●○            ");   
   printf("ID : \n");
   printf("      ○○○○○              ○○○○○ \n");
   printf("         ■■     *** ♥ ***     ■■    \n");
   printf("        ■■■                  ■■■              ");
   printf("PW : \n");
   printf("     □□□□□□            □□□□□□ \n");
   printf("      □□□□□□          □□□□□□  \n");
   gotoxy(57,18);
   scanf("%s",login.achID);
   gotoxy(57,21);
   scanf("%s",login.achPW);
   printf("\n");
   
   system("cls");

	//2.로그인 요청 패킷 전송
	sendPacket(sock, C2S_REQ_LOGIN, sizeof(login), &login);

	//3. 로그인 요청 응답 수신
	recvPacket(sock);


	return 0;
}

//로그인 체크 함수 
int checkLogin(SOCKET sock, char *buf)
{
	ResLogin res;
	memcpy(&res, buf, sizeof(res));

	if( res.iResult != 0)
	{
		printf("아이디와 비밀번호가 일치 하지 않습니다.\n\n");

		reqLogin(sock); //로그인 재시도
	}
	else
	{
		printf("정상적으로 로그인 되었습니다.");

		Sleep(1000);

		system("cls");
	} 

	return 0;
}


// 방정보 수신 함수 
void roomInfo(SOCKET sock)
{

	Sleep(1000);

	system("cls");


	recv(sock, message, BUFSIZE, 0);



	printf("%s",message);




}


//채팅방 선택 함수 
void Enter_room(SOCKET sock)
{
	while(1){
		printf("    Tip : \n1 ~ 3 : 1:1 채팅\n4 ~ 6 : 다중 채팅\n");
		printf("\n");
		printf(" 입장할 채팅 방을 선택해 주세요 : ");
		scanf("%s", room); 

		//개설된 채팅방의 범위를 벗어나게 선택하는 경우
		if(atoi(room) < 1 || atoi(room) > 6 )
		{
			printf("\n");
			printf(" 1 ~ 6 사이의 방을 선택해 주세요! \n");
			continue;
		}

		send(sock, room, sizeof(room), 0);
		recv(sock,room,sizeof(room),0);


		//방에 인원이 가득 찼을 경우 
		if(atoi(room)==-1)
		{
			printf("\n");
			printf(" 해당방의 인원이 꽉 찼습니다. 다른방을 선택 해 주세요. \n");
			continue;
		}
		else
			break;
	}


	printf("\n");
	printf(" %s 번방으로 입장합니다. \n",room);
	Sleep(1000);




}

//에러 처리 함수 
void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


//커서이동 함수 
void gotoxy(int x, int y)
{

	COORD pos ={x, y};
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),pos);
}


//채팅방 외형 처리 함수 
void Coord(void)
{
	int i;


	system("cls");

	for(i=0;i<80;i++)
	{
		gotoxy(i,0);
		printf(".");
	}

	for(i=1;i<=23;i++)
	{
		gotoxy(0,i);
		printf(".");
	}


	for(i=0;i<80;i++)
	{
		gotoxy(i,18);
		printf(".");

	}
	for(i=0;i<80;i++)
	{
		gotoxy(i,23);
		printf(".");
	}

	for(i=0;i<=23;i++)
	{
		gotoxy(79,i);
		printf(".");
	}

	gotoxy(1,1);
	printf(" [%s] 번방 ",room);
	gotoxy(2,20);

}
void FriendList()
{
   FILE *fp;
   int count = 0; 
   char word[80];  

      fp = fopen("id.txt", "r"); 

            system("cls");
            Sleep(800);

            if (fp != NULL)   
            {      
               while (1)    
               {        
                  if (fgets(word, 80, fp) == NULL)         //파일에서 한줄읽어오기        
                     break;   
                  printf("%s",word);                     //못읽어오면 루프 종료        
                                        
               }      

            }   
            fclose(fp);
}