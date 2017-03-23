/*******************************************************************************************************

	Trabalho Prático 1: Chat Seletivo.
	
	Autores:	Felipe Tadeu Costa Ribeiro - 2013066028
				Tomás Henrique Batista Aguiar - 2013066346
				
	protoIRC2.h
	
*******************************************************************************************************/

#ifndef PROTOIRC2_H
#define PROTOIRC2_H

#define NICK 0
#define POST 2
#define NEW  3
#define MUTE 4
#define UNMUTE 5
#define CLOSE  6

#define EXIT_CLOSE -1
//#defined EXIT_SUCCESS 0	POSIX
//#defined EXIT_FAILURE 1	POSIX
#define EXIT_CMD 2
#define EXIT_NICK 3
#define EXIT_MUTE 4
#define EXIT_UNMUTE 5
#define EXIT_ERR 6

#define MAX_BUF 512
#define MAX_CMD 6
#define MAX_MSG 501
#define MAX_USR 17


#define CLI_MAX 30

typedef struct packet
{
	uint8_t cmd;	//Comando digitado.
	char nickname[MAX_USR];	//Nickname do cliente.
	char message[MAX_MSG];	//Post.
}
Packet;

typedef struct client
{
	uint8_t id;		//Número de identificação do cliente.
	char nickname[MAX_USR];	//Nickname do cliente.
}
Client;

//Enviar pacotes.
int PROTO_SEND(const int socket, const Packet SEND_PAC);

//Receber pacotes.
int PROTO_RECV(Packet *RECV_PAC, const int socket);


int S_NICK_CMD(const Packet *RECV_PAC, Client *clients, const uint8_t cli);
int S_MUTE_CMD(const Packet *RECV_PAC, uint8_t **matrixStatus, const uint8_t cli, const Client *clients);  //Define como zero a relacao de user com mute_user.
int S_UNMUTE_CMD(const Packet *RECV_PAC, uint8_t **matrixStatus, const uint8_t cli, const Client *clients);   //Redefine como 1 a relacao de user com mute_user.  MATRIZ DE RELACAO.
int S_NEW_CMD(Packet *SEND_PAC, uint8_t **matrixStatus, const uint8_t cli, const Client *clients);	//Funcao do comando NEW.

#endif
