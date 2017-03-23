/*******************************************************************************************************

	Trabalho Prático 1: Chat Seletivo.
	
	Autores:	Felipe Tadeu Costa Ribeiro - 2013066028
				Tomás Henrique Batista Aguiar - 2013066346
				
	protoIRC2.c
	
*******************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "protoIRC2.h"

//Implementacao de envio de pacotes para o protocolo IRC2.
int PROTO_SEND(const int socket, const Packet SEND_PAC)
{
	return (send(socket, (Packet *) &SEND_PAC, sizeof(Packet), 0));
}

//Implementação de recebimento de pacotes para o protocolo IRC2.
int PROTO_RECV(Packet *RECV_PAC, const int socket)
{
	return (recv(socket, RECV_PAC, sizeof(Packet), 0));
}


//Implementação do comando NICK do servidor.
int S_NICK_CMD(const Packet *RECV_PAC, Client *clients, const uint8_t cli)
{
    	if(clients[cli].nickname[0] == '\0')
    	{
        	Packet ON_PAC;		//Pacote para enviar aos clientes que um novo usuário entrou.
        	ON_PAC.cmd = POST;	//NEW servidor->cliente.
        	strcpy(ON_PAC.nickname, RECV_PAC->nickname);	//nickname = argv3 do cliente.
        	strcpy(ON_PAC.message, "Está online.\n");	//mensagem = [nickname]: Está online.

        	for(uint8_t i = 0; i < CLI_MAX; i++)
        	{
        		if((clients[i].id != 0) && (i != cli))
                	PROTO_SEND(clients[i].id, ON_PAC);	//Envia a todos os clientes conectados.
        	}
    	}
    	else
    	{
        	Packet CHA_PAC;		//Pacote para informar aos clientes que um usuário trocou seu nickname.
        	CHA_PAC.cmd = POST;	//NEW servidor->cliente.
        	strcpy(CHA_PAC.nickname, clients[cli].nickname);	//nickname(antigo).

        	char change[] = "Mudou seu nickname para ";
        	strcpy(CHA_PAC.message, change); 		//[antigo]: Mudou seu nickname para <novoNick>.
        	strcat(CHA_PAC.message, RECV_PAC->nickname);
        	strcat(CHA_PAC.message, ".\n\0");		

        	for(uint8_t i = 0; i < CLI_MAX; i++)
        	{
        		if((clients[i].id != 0) && (i != cli))
                	PROTO_SEND(clients[i].id, CHA_PAC);	//Envia a todos os clientes conectados.
        	}
    	}

	//Coloca nickname do usuário no conjunto. 
	strcpy(clients[cli].nickname, RECV_PAC->nickname);	
	if(strcmp(clients[cli].nickname, RECV_PAC->nickname) != 0)
		return EXIT_NICK;	//3.

	fprintf(stdout, "New user = %s, fd = %d.\n", clients[cli].nickname, clients[cli].id); 

	return EXIT_SUCCESS;
}

//Implementação do comando NEW do servidor (POST do cliente + checkMUTE).
int S_NEW_CMD(Packet *SEND_PAC, uint8_t **matrixStatus, const uint8_t cli, const Client *clients)
{
	fprintf(stdout, "NEW [%s] %s", clients[cli].nickname, SEND_PAC->message);

	strcpy(SEND_PAC->nickname, clients[cli].nickname);	//nickname = cli.nickname.

	for(uint8_t i = 0; i < CLI_MAX; i++)
	{
		if((matrixStatus[i][cli] == 1) && (clients[i].id != 0) && (i != cli))
			PROTO_SEND(clients[i].id, *SEND_PAC);	//Envia a todos com unmuted status.
	}

	return EXIT_SUCCESS;
}

//Implementação do comando MUTE do servidor.
int S_MUTE_CMD(const Packet *RECV_PAC, uint8_t **matrixStatus, const uint8_t cli, const Client *clients)
{
	for(uint8_t i = 0; i < CLI_MAX; i++)
	{
		//No servidor não guarda o nome do cliente e \n, portanto exclui-se da comparação.
		if(strcmp(RECV_PAC->nickname, clients[i].nickname) == 0)
		{
			fprintf(stdout, "%s MUTEd %s.\n", clients[cli].nickname, clients[i].nickname);
			
			matrixStatus[cli][i] = 0;	//cli mute i.
			return EXIT_SUCCESS;
		}
	}

	return EXIT_MUTE;	//4
}

//Implementação do comando UNMUTE no servidor.
int S_UNMUTE_CMD(const Packet *RECV_PAC, uint8_t **matrixStatus, const uint8_t cli, const Client *clients)
{
	for(uint8_t i = 0; i < CLI_MAX; i++)
	{
		//No servidor não guarda o nome do cliente e \n, portanto exclui-se da comparação.
		if(strcmp(RECV_PAC->nickname, clients[i].nickname) == 0)
		{
            if(matrixStatus[cli][i] == 0)
            {
				matrixStatus[cli][i] = 1;	//cli unmute i.

			   	fprintf(stdout, "%s UNMUTEd %s.\n", clients[cli].nickname, clients[i].nickname);

    			return EXIT_SUCCESS;
            }
		}
	}

	return EXIT_UNMUTE;	//5.
}
