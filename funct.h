/*******************************************************************************************************

	Trabalho Prático 1: Chat Seletivo.
	
	Autores:	Felipe Tadeu Costa Ribeiro - 2013066028
				Tomás Henrique Batista Aguiar - 2013066346
				
	funct.h
	
*******************************************************************************************************/

#ifndef FUNCT_H
#define FUNCT_H

#include <stdint.h>

#include "protoIRC2.h"

//Recebe no buffer o comando.
void commandLine_C(char *buffer, const char *nick);

//Trata a entrada de stdin para comandos.
int handle_input_C(const char *buffer, const int socket, char *nick);

//Trata a entrada do socket.
int handle_output_C(const Packet *RECV_PAC);

//Imprime em stdout a mensagem recebida.
void printMessage_C(const Packet *RECV_PAC);

//Trata o pacote recebido pelo servidor.
int handle_input_S(Packet *RECV_PAC, const int cli, uint8_t **matrixStatus, Client *clients);

#endif	/* FUNCT_H */
