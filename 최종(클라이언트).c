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
#define C2S_REQ_LOGIN   1001   //Ŭ���̾�Ʈ�� �������� �α����� ��û�Ѵ�.
#define S2C_RES_LOGIN   2001   //������ Ŭ���̾�Ʈ���� �α��� ��û ������ �����Ѵ�.
//--------------------------------
//�α��� ��û ������ ����ü
typedef struct ReqLogin
{  
	char  achID[20];  //���̵�
	char  achPW[20];  //���
}ReqLogin;

typedef struct ResLogin
{ 
	int  iResult;   // ���� ���� ( 0 �̸� �α��� ����)
}ResLogin;

#define BUFSIZE 1024
#define NAMESIZE 20
#define ROOMNUM  6 
#define BACKSPACE 8
#define ENTER     13

//--------------------------------
//�Լ�
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
//���� ����
char name[NAMESIZE]="[Default]";
char message[BUFSIZE];
char room[ROOMNUM];
char Allchat[8][BUFSIZE+BUFSIZE];

//���� �Լ�
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
		//�α��� ó��
		if(reqLogin(sock)==0)
			break;



	}
start:
	main_print();
	gotoxy(55,18); // �޴� 1�� ����

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
				printf("���������� ����Ǿ����ϴ�.");
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
		ErrorHandling("������ ���� ����");
	}

	WaitForSingleObject(hThread1, INFINITE);
	WaitForSingleObject(hThread2, INFINITE);

	closesocket(sock);
	return 0;
}
//�α��� ���� �� ���� ȭ�� ���
void main_print(){
   printf("\n\n");
   printf("       ����  ��    ��     ���    ����� ����  ���   ��   �����   \n");
   printf("     ���     ��      ��  �����     ��      ��   ����  ��  ��      ��  \n");
   printf("     ��       ��      �� ��      ��    ��      ��   ��  ��  �� ��           \n");
   printf("    ��        ������ ������    ��      ��   ��  ��  �� ��           \n");
   printf("     ��       ��      �� ��      ��    ��      ��   ��  ��  �� ��   ����  \n");
   printf("     ���     ��      �� ��      ��    ��      ��   ��  ����  ��      ��  \n");
   printf("       ����  ��    ��   ��    ��     ��    ���� ��   ���    �����   \n");
   printf("\n\n");
   printf("   ������������������������������������� \n");
   printf("   ��                                                                    �� \n");
   printf("   ��                                                                    �� \n");
   printf("   ��          1. ä���ϱ�                                               �� \n");
   printf("   ��                                        �� SELECT  NUMBER ��        �� \n");
   printf("   ��                                                                    �� \n");
   printf("   ��          2. ģ�����                      ��������������           �� \n");
   printf("   ��                                           ��          ��           �� \n");
   printf("   ��                                           ��������������           �� \n");
   printf("   ��          3. ä������                                               �� \n");
   printf("   ��                                                                    �� \n");
   printf("   ��                                                                    �� \n");
   printf("   ������������������������������������� \n");

}
//�޼��� ���� �Լ� 
DWORD WINAPI SendMSG(void *arg)
{
	SOCKET sock = (SOCKET)arg;
	char nameMessage[NAMESIZE+BUFSIZE]={0,};
	char log[5]={0,};
	char ch=0;
	char Log[BUFSIZE]={0,};
	int i=0,j=2;

	memset(message,0,BUFSIZE);
	sprintf(Log, "%s ���� �����ϼ̽��ϴ�. \n", name);
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

			//BACKSPACE ��� ������

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

			//���ڿ� �Է��� ����

			if(j==64){

				gotoxy(j,20);
				printf(" ");
				j--;
				gotoxy(j,20);

			}

			//���� ���� ó����

			if(ch==ENTER)
			{
				gotoxy(2,20);
				j=2;
				break;
			}


			j++;




		}


		//ä�ù� ����

		if(message[0]=='/' && message[1] == 'q')
		{


			gotoxy(2,20);
			printf(" �α� �ƿ� �Ͻðڽ��ϱ�? (y/n)");
			Sleep(500);

			scanf("%s",log);

			Sleep(500);


			if(!strcmp(log, "y"))
			{
				gotoxy(2,20);
				sprintf(log, "%s ���� ���� �ϼ̽��ϴ�. \n\n\n\n", name);
				send(sock, log, strlen(log), 0);
				Sleep(1000);
				system("cls");
				closesocket(sock);
				exit(0);

			}

			else if(!strcmp(log, "n"))
			{
				gotoxy(2,20);
				printf(" �α׾ƿ� ���, �޼����� �Է��ϼ���. \n");
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


//�޼��� ���� �Լ�
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

		//�������� ���� ����
		if(strLen == -1)
			ErrorHandling("������ ������ ����Ǿ����ϴ�. \n\n\n\n");
		gotoxy(2,26);


		nameMessage[strLen]=0;


		//�޼��� ��º� 
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

//��Ŷ ���� �Լ� 
void sendPacket( SOCKET sock, int code, int size, void *pData)
{ 
	int iret=0;
	char sendBuf[BUFSIZE]; 

	//��Ŷ ������ ����
	memset(sendBuf,0x00,BUFSIZE); 
	memcpy(&sendBuf[0],&code, 2);      
	memcpy(&sendBuf[2],&size, 2);   
	memcpy(&sendBuf[4], pData, size);  

	//��Ŷ ����
	iret = send(sock, sendBuf, size+4, 0);
	if(iret<0)
		ErrorHandling(" �����͸� ������ ���߽��ϴ�.");

}


//��Ŷ ���� �Լ�
int recvPacket(SOCKET sock)
{
	char buf[BUFSIZE] = {0, };
	short code=0;
	short size=0;
	int iret;


	//1. ��Ŷ ���� �б�
	iret = recv(sock, buf, 2,0);

	//�о�� ������ ���̰� 0���� ���ų� ������, ���� ������ Ŭ���̾�Ʈ�� �������� �Ҿ����ϰ� ������ ���̱⶧���� Ŭ���̾�Ʈ���� ������ �����Ѵ�.
	if (iret <= 0)  
	{  
		if( iret == -1 )   
			ErrorHandling("��Ŷ ���ſ���");
	}
	else
	{  
		//1. ��Ŷ ���� ������ ������ ���  ����
		memcpy(&code,&buf[0], 2);  
		memset(&buf,0x00,2);

		//2. ������ ���� �б�  
		recv(sock, buf, 2,0);      
		memcpy(&size,&buf[0],2);
		memset(&buf,0x00,2);

		//3. ������ �б�
		if( size > 0 )   
			recv(sock, buf, size,0);


		switch(code)
		{
		case S2C_RES_LOGIN: 
			checkLogin(sock, buf);
			break;


		default :
			printf("��Ŷ �ڵ� ����\n");
			return -1;
		}
	}
	return 0;
}

//�α��� ��û �Լ� 
int reqLogin(SOCKET sock)
{
	int randcolor = 0;

	//��Ŷ ���� ����  
	ReqLogin login;  

	Sleep(800);

	system("cls");
printf("\n");
   printf("         ���        ���   ���� ��     ���  ���   ��  ��  ���� \n");
   printf("         ���        ���   �� ��  ��    ��    ��  �� ����� �� ��  \n");
   printf("         ���        ���   ���� ���� ���  ���  �� �� �� ���� \n");
   printf("         ���   ��   ���                                             \n");
   printf("         ���  ���  ���              ���� ���                    \n");
   printf("          ��� ��� ���                 ��  ��  ��                   \n");
   printf("            ���  ���                   ��   ���                    \n");
   printf("\n");
   printf("      ��� ��  ��  ��� ���� ���� ���� ���� ��  �� ���� ���� \n");
   printf("     ��    ��  �� ��  ��  ��   ��     ��     ��  �� ��  �� ��     ��  �� \n");
   printf("     ��    ���� ����  ��   ���� ���� ���� ��  �� ���� ���� \n");
   printf("     ��    ��  �� ��  ��  ��       �� ��     �� ��   ���  ��     �� ��  \n");
   printf("      ��� ��  �� ��  ��  ��   ���� ���� ��  ��   ��   ���� ��  �� \n");
   printf("\n\n");
   printf("      �ۡۡۡۡ�              �ۡۡۡۡ� \n");
   printf("      �ۡܡܡܡ�              �ۡܡܡܡ� \n");
   printf("      �ۡܡܡܡ�              �ۡܡܡܡ�            ");   
   printf("ID : \n");
   printf("      �ۡۡۡۡ�              �ۡۡۡۡ� \n");
   printf("         ���     *** �� ***     ���    \n");
   printf("        ����                  ����              ");
   printf("PW : \n");
   printf("     �������            ������� \n");
   printf("      �������          �������  \n");
   gotoxy(57,18);
   scanf("%s",login.achID);
   gotoxy(57,21);
   scanf("%s",login.achPW);
   printf("\n");
   
   system("cls");

	//2.�α��� ��û ��Ŷ ����
	sendPacket(sock, C2S_REQ_LOGIN, sizeof(login), &login);

	//3. �α��� ��û ���� ����
	recvPacket(sock);


	return 0;
}

//�α��� üũ �Լ� 
int checkLogin(SOCKET sock, char *buf)
{
	ResLogin res;
	memcpy(&res, buf, sizeof(res));

	if( res.iResult != 0)
	{
		printf("���̵�� ��й�ȣ�� ��ġ ���� �ʽ��ϴ�.\n\n");

		reqLogin(sock); //�α��� ��õ�
	}
	else
	{
		printf("���������� �α��� �Ǿ����ϴ�.");

		Sleep(1000);

		system("cls");
	} 

	return 0;
}


// ������ ���� �Լ� 
void roomInfo(SOCKET sock)
{

	Sleep(1000);

	system("cls");


	recv(sock, message, BUFSIZE, 0);



	printf("%s",message);




}


//ä�ù� ���� �Լ� 
void Enter_room(SOCKET sock)
{
	while(1){
		printf("    Tip : \n1 ~ 3 : 1:1 ä��\n4 ~ 6 : ���� ä��\n");
		printf("\n");
		printf(" ������ ä�� ���� ������ �ּ��� : ");
		scanf("%s", room); 

		//������ ä�ù��� ������ ����� �����ϴ� ���
		if(atoi(room) < 1 || atoi(room) > 6 )
		{
			printf("\n");
			printf(" 1 ~ 6 ������ ���� ������ �ּ���! \n");
			continue;
		}

		send(sock, room, sizeof(room), 0);
		recv(sock,room,sizeof(room),0);


		//�濡 �ο��� ���� á�� ��� 
		if(atoi(room)==-1)
		{
			printf("\n");
			printf(" �ش���� �ο��� �� á���ϴ�. �ٸ����� ���� �� �ּ���. \n");
			continue;
		}
		else
			break;
	}


	printf("\n");
	printf(" %s �������� �����մϴ�. \n",room);
	Sleep(1000);




}

//���� ó�� �Լ� 
void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


//Ŀ���̵� �Լ� 
void gotoxy(int x, int y)
{

	COORD pos ={x, y};
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),pos);
}


//ä�ù� ���� ó�� �Լ� 
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
	printf(" [%s] ���� ",room);
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
                  if (fgets(word, 80, fp) == NULL)         //���Ͽ��� �����о����        
                     break;   
                  printf("%s",word);                     //���о���� ���� ����        
                                        
               }      

            }   
            fclose(fp);
}