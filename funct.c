/*******************************************************************************************************

	Trabalho Prático 1: Chat Seletivo.
	
	Autores:	Felipe Tadeu Costa Ribeiro - 2013066028
				Tomás Henrique Batista Aguiar - 2013066346
				
	funct.c
	
*******************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include "funct.h"
#include "protoIRC2.h"

void commandLine_C(char *buffer, const char *nick)
{
	buffer[0] = '\0';
	char c; //Recebe caracteres digitados.
	int i = 0; //Contador de caracteres.

	uint8_t in = 0; //Booleano para testar se jah comecou a receber os caracteres.

   	while((c = fgetc(stdin)) && (i < MAX_BUF))
    {
    	if((i == 0) && (c != '\n') && (c != 0x007F) && (!in))     //Se b não contém nada e 1º char.
			fprintf(stdout, "   %s: ", nick);
                    
		in = 1; //Jah comecou a receber os caracteres.

    	if((c == 0x7F) && (i > 0) ) //Se c = backspace (apagar).
        {
           	fprintf(stdout, "\b");      //Volta um caracter.
            --i;    //Decrementa contagem.
        }
		else if((c == 0x7F) && (i == 0))  //Se c = backspace mas i eh igual a 0.
		   	i = 0;
        else        //Buffer recebe caracter lido.
        {
	       	buffer[i] = c;     //Coloca c em b.
        	fprintf(stdout, "%c", c);       //Imprime c em STDOUT_FILENO.
            i++;
        }
                
		if(c == '\n')   //Se c == [ENTER] (termina b).
        {
           	buffer[++i] = '\0';    //Fim de b.
            break;       //Sai do while.
        }
	}   
}

int handle_input_C(const char *buffer, const int socket, char *nick)
{
	char *command = (char *) malloc(MAX_CMD * sizeof(char));	//Recebe o comando digitado. (Máx.: 6[UNMUTE]).
	command[0] = '\0';	//Inicia vazio.

	uint8_t i = 0;
	uint16_t k = 0;

	while(buffer[i++] == ' ');	//Tira os espaços até onde começa comando.

	for(uint8_t j = (i - 1); (j < MAX_CMD) && (buffer[j] != ' '); j++)
		command[k++] = buffer[j];	//Recebe o comando.
	command[k] = '\0';	//Termina string do comando.

	command = (char *) realloc(command, k * sizeof(char));      //Aloca espaco para retirar o comando do buffer.

	uint8_t ok = 1;	//Booleano para verificar a validade do comando.

	Packet SEND_PAC;        //Pacote a ser enviado.
	memset(&SEND_PAC, 0, sizeof(Packet));   //Zera pacote.

	switch(command[0])
	{
		case 'N':	//Comando é inciado pela letra N.
			if(strcmp(command, "NICK\0") != 0)	//Compara command com NICK.
				ok = 0;	//Error: comando não reconhecido.
			SEND_PAC.cmd = NICK; //NICK;	//cmd = 0.	
			break;
		case 'P':	//Comando é inciado pela letra P.
			if(strcmp(command, "POST\0") != 0)	//Compara command com POST.
				ok = 0;	//Error: comando não reconhecido.
			SEND_PAC.cmd = POST;	//cmd = 1.	
			break;
		case 'M':	//Comando é inciado pela letra M.
			if(strcmp(command, "MUTE\0") != 0)	//Compara command com MUTE.
				ok = 0;	//Error: comando não reconhecido.
			SEND_PAC.cmd = MUTE;	//cmd = 2.	
			break;
		case 'U':	//Comando é inciado pela letra U.
			if(strcmp(command, "UNMUTE\0") != 0)	//Compara command com UNMUTE.
				ok = 0;	//Error: comando não reconhecido.
			SEND_PAC.cmd = UNMUTE;	//cmd = 3.	
			break;
		case 'C':	//Comando é inciado pela letra C.
			if(strcmp(command, "CLOSE\0") != 0)	//Compara command com CLOSE.
				return EXIT_CLOSE;	//EXIT_CLOSE = -1.
		default:
			ok = 0;	//Erro: comando não reconhecido.
	}

	free(command);	//Libera o espaço alocado para testar validade do comando.

	if(!ok)	//Se erro.
		return EXIT_CMD;	//Erro de comando.

	while(buffer[++k] == ' ');	//Até encontrar novo campo (nick ou post).

	if((SEND_PAC.cmd == NICK) || (SEND_PAC.cmd == MUTE) || (SEND_PAC.cmd == UNMUTE))
	{
		//Aloca espaço na memória para definir o usuário.
		char *user = (char *) malloc(MAX_USR * sizeof(char));   //Aloca espaco para retirar o nickname do buffer.

		uint8_t j = 0;
		while(j < MAX_USR)
		{
			if(buffer[k] == ' ')	//Se deu espaço ao digitar o nick, nao considera 
				break;		// depois do espaço.
			else if((buffer[k] == '\0') || (buffer[k] == '\n'))	//Fim de nick.
				break;
			
			user[j++] = buffer[k++];
		}

        if(j == MAX_USR)    //Se user atingiu o limite de 16 caracteres.
            	user[MAX_USR - 1] = '\0';   //Limita aos 16 + '\0'.
        else
		    user[j] = '\0';

		user = (char *) realloc(user, j * sizeof(char));    //Acerta o tamanho do espaco de user.
		
		strcpy(SEND_PAC.nickname, user);	//nickname = user.
		strcpy(SEND_PAC.message, "\0");		//message = NULL.
	
		free(user);	//Libera espaço alocado para definir nickname.
	}
	else if(SEND_PAC.cmd == POST)
	{
		//Aloca espaço na memória para definir a mensagem.
		char *msg = (char *) malloc(MAX_MSG * sizeof(char));

		uint16_t j = 0;
		while(j < strlen(buffer) && (j < MAX_MSG))
		{
			if(buffer[k] == '\0')	//Fim de mensagem.
				break;
			
			msg[j++] = buffer[k++];
		}		
		
		if(j == MAX_MSG)	//Se mensagem atingiu o limite de 500 caracteres.
			msg[MAX_MSG - 1] = '\0';	//Limita a 500 + '\0'.
		else
			msg[j] = '\0';	//Fim da mensagem.

        strcpy(SEND_PAC.nickname, "\0");	//nickname = NULL.
		strcpy(SEND_PAC.message, msg);		//message = msg.

		free(msg);	//Libera espaço para definir mensagem.
	}

	int ret = PROTO_SEND(socket, SEND_PAC);     //Envia pacote para o servidor.
	if(ret < 0)
	{
		perror("send:");
		return EXIT_FAILURE;	//Erro de envio.
	}
	else if(SEND_PAC.cmd == NICK)
		strcpy(nick, SEND_PAC.nickname);	//Atualiza nick, nick = user.
	
	return EXIT_SUCCESS;
}

int handle_output_C(const Packet *RECV_PAC)
{
	switch(RECV_PAC->cmd)
	{
		case POST:     
			if(RECV_PAC->message[0] == '\0')	//Se não existe mensagem.
				return EXIT_FAILURE;
			break;	
		default:
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void printMessage_C(const Packet *RECV_PAC)
{
	time_t currtime;		//Variável que recebe o tempo atual.
	struct tm *timeinfo;	//Estrutura para guradar valores de tempo.
	
	time(&currtime);		//Recebe o tempo atual.
	timeinfo = localtime(&currtime);	//Coloca na estrutura.
	
	//HH:MM [RECV_PAC->nickname]: RECV_PAC->message
	fprintf(stdout, "%02d:%02d ", timeinfo->tm_hour, timeinfo->tm_min);
	fprintf(stdout, "[%s]: ", RECV_PAC->nickname);
    fprintf(stdout, "%s", RECV_PAC->message);
}

int handle_input_S(Packet *RECV_PAC, const int cli, uint8_t **matrixStatus, Client *clients)
{
	switch(RECV_PAC->cmd)
	{
		case POST:
			return (S_NEW_CMD(RECV_PAC, matrixStatus, cli, clients));
		case MUTE:
			return (S_MUTE_CMD(RECV_PAC, matrixStatus, cli, clients));
		case UNMUTE:
			return (S_UNMUTE_CMD(RECV_PAC, matrixStatus, cli, clients));
		case NICK:
			return (S_NICK_CMD(RECV_PAC, clients, cli));
		default:
			return EXIT_ERR;	//Erro do cliente.
	}
	
	return EXIT_FAILURE;
}
