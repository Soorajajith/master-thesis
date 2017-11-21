#include<stdio.h>
#include<winsock2.h>
#include<time.h>
#include"networkcomm.h"

#define ANALOG_DEADZONE 75

#pragma comment(lib,"ws2_32.lib") //Winsock Library
#pragma warning(disable: 4996) //4996 for _CRT_SECURE_NO_WARNINGS equivalent

struct sockaddr_in udp_si_other;
int udp_socket, slen;
WSADATA wsa;
clock_t timer_client;
clock_t timer_robot;
clock_t timer_contact;
clock_t timer_analog;

bool connected = false;

char old_move[] = {createHeader(NETINSTR_DIR), 0, 0};

void tryForContact();

int clientSoftTimeoutExceeded(){
	return timeoutExceeded(timer_client, NET_SOFT_TIMEOUT);
}

int clientHardTimeoutExceeded(){
	return timeoutExceeded(timer_client, NET_HARD_TIMEOUT);
}

int robotHardTimeoutExceeded(){
	return timeoutExceeded(timer_robot, NET_HARD_TIMEOUT);
}

int timeoutExceeded(clock_t timer, int timeout){
	return (clock() - timer) * 1000 / CLOCKS_PER_SEC > timeout;
}

void resetTimer(clock_t* timer){
	*timer = clock();
}

void resetClientTimer(){
	resetTimer(&timer_client);
}

void resetRobotTimer(){
	resetTimer(&timer_robot);
}

void sendPing(){
	printf("Sending ping\n");
	char message[2];
	message[0] = createHeader(NETINSTR_PING);
	sendMessage(message, 1);
}

void tryForContact(){
	char message[BUFLEN];
	char answer[BUFLEN];

	if (VERBOSE)
		printf("Trying for contact... ");
	//Wait until a certain time has been passed so the network isn't cluttered.
	if ((clock() - timer_contact) * 1000 / CLOCKS_PER_SEC >= NET_CONTACT_WAIT){
		message[0] = createHeader(NETINSTR_CONTACT); //Create message
		resetTimer(&timer_contact);
		if (sendMessage(message, 1) == SOCKET_ERROR){ //Try to send message
			printf("tryForContact: sendto failed with error code : %d\n", WSAGetLastError());
			return;
		}
	}
	if (getMessage(answer) == SOCKET_ERROR){ //Try to receive a message
		int error = WSAGetLastError();
		if (error != WSAEWOULDBLOCK){ //No data received, normal if there's no contact
			if (error != WSAEMSGSIZE){ //Too much data received, this is assumed to be an image and is ignored.
				printf("tryForContact: recvfrom failed with error code : %d\n", error); //Print other errors
				return;
			}
		}
	}
	else if (answer[0] = NETINSTR_ACK){ //Answer is received
		if (VERBOSE)
			printf("Contact received.\n");
		resetRobotTimer();
		connected = true;
		return;
	}
}

void establishContact(){
	char message[BUFLEN];
	char answer[BUFLEN];
	message[0] = createHeader(NETINSTR_CONTACT);
	if (VERBOSE)
		printf("Trying to establish contact... ");
	while (1){
		//Try to send message
		if (sendMessage(message, 1) == SOCKET_ERROR){
			printf("establishContact: sendto failed with error code : %d\n", WSAGetLastError());
			return;
		}
		//Check for answers for a while
		for (int i = 0; i < 10; i++){
			if (getMessage(answer) == SOCKET_ERROR){ //Try to reveive a message
				int error = WSAGetLastError();
				if (error != WSAEWOULDBLOCK){ //No data, this is expected so keep reading
					if (error != WSAEMSGSIZE){ //Too much data received, assuming this is an image. Ignore and wait for ack.
						printf("establishContact: recvfrom failed with error code : %d\n", error);
						return;
					}
				}
				Sleep(100); //Wait until next read
			}
			else if (answer[0]=NETINSTR_ACK){ //Answer is received
				if (VERBOSE)
					printf("Established.\n");
				resetRobotTimer();
				connected = true;
				return;
			}
		}
	}
}

bool isConnected(){
	if (robotHardTimeoutExceeded())
		//No answer received for too long, assume connection is lost
		connected = false;
	else
		connected = true;
	return connected;
}

void closeNetwork(){
	closesocket(udp_socket);
	WSACleanup();
}

int initNetwork(char* server){
	slen = sizeof(struct sockaddr_in);

	//Initialise winsock
	if (VERBOSE)
		printf("\nInitialising Winsock... ");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		if (VERBOSE)
			printf("Failed. Error Code : %d\n", WSAGetLastError());
		closeNetwork();
		return -1;
	}
	if (VERBOSE)
		printf("Initialised.\n");

	//create UDP socket
	if (VERBOSE)
		printf("Initializing UDP socket... ");
	if ((udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		printf("socket() failed with error code : %d\n", WSAGetLastError());
		closeNetwork();
		return -1;
	}
	if (VERBOSE)
		printf("Initialized.\n");

	//setup UDP address structure
	memset((char *)&udp_si_other, 0, sizeof(udp_si_other));
	udp_si_other.sin_family = AF_INET;
	udp_si_other.sin_port = htons(UDP_PORT);
	udp_si_other.sin_addr.S_un.S_addr = inet_addr(server);

	//Set unblocking reads
	setUnblocking();

	//Start timers
	resetTimer(&timer_contact);
	resetTimer(&timer_analog);
	resetClientTimer();

	timer_robot = clock() -3 * CLOCKS_PER_SEC;
	return 0;
}

void setUnblocking(){
	u_long iMode = 1;
	ioctlsocket(udp_socket, FIONBIO, &iMode);
}

void setBlocking(){
	u_long iMode = 0;
	ioctlsocket(udp_socket, FIONBIO, &iMode);
}

int getMessage(char* buf){
	int recv_size = recvfrom(udp_socket, buf, BUFLEN, 0, (struct sockaddr *) &udp_si_other, &slen);
	if (recv_size != -1){
		resetRobotTimer();
	}
	return recv_size;
}

int getImage(char*buf){
	int recv =  recvfrom(udp_socket, buf, BUFLEN_IMG + 1, 0, (struct sockaddr *) &udp_si_other, &slen);
	
	if (recv != -1){
		resetRobotTimer();
	}	
	return recv;
}

int sendMessage(char* buf, int len){
	int sendval = sendto(udp_socket, buf, len, 0, (struct sockaddr *) &udp_si_other, slen);
	if (sendval != -1)
		resetClientTimer();
	return sendval;
}


void printBinary(char* buf, int len){
	for (int i = 0; i < len; i++){
		char byte = buf[i];
		for (int j = 7; j >= 0; j--){
			int bit = (byte >> j)&1;
			if (bit == 1)
				printf("1");
			else
				printf("0");
		}
		printf(" ");
	}
	printf("\n");
}

void sendMovementData(char* move){
	//Only send data if it differs from the last data sent
	if (move[1] != old_move[1] || move[2] != old_move[2]){
		sendMessage(move, 3);
		old_move[1] = move[1];
		old_move[2] = move[2];
	}
}

void sendAnalogMovementToRobot(int x, int y){
	//Since analog data can change often a timer is used to
	//prevent flooding the network
	if (timeoutExceeded(timer_analog, 100)){
		if (x > -ANALOG_DEADZONE && x < ANALOG_DEADZONE)
			x = 0;
		if (y > -ANALOG_DEADZONE && y < ANALOG_DEADZONE)
			y = 0;
		char move[] = { createHeader(NETINSTR_DIR), y, (3 * x) / 4 };
		sendMovementData(move);
		resetTimer(&timer_analog);
	}
}

void sendDigitalMovementToRobot(bool* keys){
	if (isConnected()){
		char move[] = {createHeader(NETINSTR_DIR), 0, 0};
		if (keys[KEY_UP])
			move[1] = 127;
		else if (keys[KEY_DOWN])
			move[1] = -128;
		else
			move[1] = 0;

		if (keys[KEY_RIGHT]){
			if (move[1] != 0)
				move[2] = 96;
			else
				move[2] = 127;
		}
		else if (keys[KEY_LEFT]){
			if (move[1] != 0)
				move[2] = -96;
			else
				move[2] = -128;
		}
		else
			move[2] = 0;
		sendMovementData(move);
		
	}
}

void sendButton(int btn){
	char buf[] = { createHeader(NETINSTR_BTNS), btn };
	sendMessage(buf, 2);
}

void sendShutdown(){
	char buf[] = { createHeader(NETINSTR_SHUTDOWN) };
	sendMessage(buf, 1);
}

byte createHeader(byte instr_type){
	/*The header is a single byte that contains the instruction type
	and number of data bytes. The first four bytes contains the type
	and the last four the number of data types. The header is built 
	by shifting the instruction type four times to the left, and then
	adding the number of data types to that byte. If an unknown
	instruction is received the stop instruction is returned.*/
	switch (instr_type){
	//Instructions with 0 data bytes
	case NETINSTR_CONTACT:
	case NETINSTR_PING:
	case NETINSTR_SHUTDOWN:
		return (instr_type << 4) + 0;

	//Instructions with 1 data byte
	case NETINSTR_STOP:
	case NETINSTR_BTNS:
	case NETINSTR_IMGMODE:
		return (instr_type << 4) + 1;

	//Instructions with 2 data bytes
	case NETINSTR_DIR:
	case NETINSTR_MOUSE:
		return (instr_type << 4) + 2;

	//Unknown instruction, return stop
	default:
		return (NETINSTR_STOP << 4) + 0;
	}
}