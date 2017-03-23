/*******************************************************************************************************

	Trabalho Prático 1: Chat Seletivo.
	
	Autores:	Felipe Tadeu Costa Ribeiro - 2013066028
				Tomás Henrique Batista Aguiar - 2013066346
				
	cliente.c
	
*******************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>

#include <stdint.h>

#include <sys/select.h>

#include <termios.h>

#include "funct.h"
#include "protoIRC2.h" 

int main(int argc, char *argv[])
{
	if(argc != 4)	//  ./cliente ip_name port_number user_name
	{
		fprintf(stderr, "Número incorreto de argumentos para este programa.\n");
		exit(EXIT_FAILURE);
	}

	int sock;	//Descritor de arquivo para socket.
	/* Cria um endpoint para comunicação com o servidor */
	if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)	//Testa erro.
	{
		perror("Socket.");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server;		//tad para receber informações do servidor.
	memset(&server, 0, sizeof(server));	//Zera a struct com infos do servidor.
	server.sin_family = AF_INET;		//Servidor da familia de protocolo IPv4.
	server.sin_port = htons(atoi(argv[2]));	//Recebe a porta do servidor.

	struct hostent *hp;			//Ponteiro para receber o ip/dominio.
	char *hostname = (char *) malloc(strlen(argv[1])*sizeof(char));
	strcpy(hostname, argv[1]);		//Recebe por parametro o ip/dominio

	if(!(hp = gethostbyname(hostname)))	//Recebe o endereço de argv[1].
	{
		printf("%s was not resolved. gethostbyname:\n", hostname);
		exit(EXIT_FAILURE);
	}
    
    server.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr_list[0]))->s_addr;	//Atualiza endereço do servidor (ip).

	/* Conecta o cliente com o servidor */
	if((connect(sock, (struct sockaddr *)&server, sizeof(struct sockaddr))) == -1)
	{	
		perror("Connect.");
		exit(EXIT_FAILURE);
	}

	fd_set rfd;	//Leitor para select.
	FD_ZERO(&rfd);	//Zera rfd.

	struct termios old_t, new_t;	//Struct para atribuições do terminal.
	tcgetattr(STDIN_FILENO, &old_t);	//Recebe parâmetros atuais do terminal.
	new_t = old_t;	//Copia novos parâmetros do antigo.

	//Muda modos locais para o terminal novo.
	new_t.c_lflag &= ~(ICANON | ECHO);	//Define noncanonical e sem echo.
	tcsetattr(STDIN_FILENO, TCSANOW, &new_t);	//Define novo terminal imediatamente.

	Packet begin;		//Pacote inicial para enviar o nickname ao servidor.
	begin.cmd = NICK;
	strcpy(begin.nickname, argv[3]);	//nickname = argumento 4.
	begin.message[0] = '\0';
	if((send(sock, (Packet *) &begin, sizeof(Packet), 0)) < 0)
	{
		perror("send-begin:");
		tcsetattr(STDIN_FILENO, TCSANOW, &old_t);
		exit(EXIT_FAILURE);
	}
    
    char *buffer = (char *) malloc(MAX_BUF * sizeof(char));	//Aloca espaço para o buffer dos comandos.

	char *nick = (char *) malloc(MAX_USR * sizeof(char));	//Aloca espaço para o nickname do cliente.
	strcpy(nick, argv[3]);	//Recebe o nick inicial.

	while(1)	//Roda indefinidamente.
	{
		FD_SET(0, &rfd);	//Coloca stdin em rfd.
		FD_SET(sock, &rfd);		//Coloca socket em rfd.

		int maxfd = sock + 1;		//Delimita máximo fd.
		select(maxfd, &rfd, NULL, NULL, NULL);	//Verifica ação em fds.
		
		if(FD_ISSET(0, &rfd))	//Se ação em stdin.
		{
            commandLine_C(buffer, nick);	//Recebe o comando do cliente.

			int status = handle_input_C(buffer, sock, nick);	//Trata o buffer e cria pacote a ser enviado
			if(status != EXIT_SUCCESS)			// e envia.
			{
				if(status == EXIT_FAILURE)	//Pacote não enviado.
				{
					tcsetattr(STDIN_FILENO, TCSANOW, &old_t);	//Define configurações normais
					exit(EXIT_FAILURE);				// do terminal.
				}
				else if(status == EXIT_CLOSE)	//Comando CLOSE do cliente.
				{	
					fprintf(stdout, "Saindo...\n");
					break;	//while.
				}
				
                fprintf(stdout, "Comando não reconhecido.\n");	//Comando não foi reconhecido. (EXIT_CMD).
            }
	
            memset(buffer, 0, MAX_BUF * sizeof(char));
			buffer[0] = '\0';
		}

        else if(FD_ISSET(sock, &rfd))	//Se a ação ocorreu no socket.
		{
			Packet RECV_PAC;	//Pacote para receber.

			int recve;
			if((recve = PROTO_RECV(&RECV_PAC, sock)) <= 0)	//Não recebeu nada ou erro.
			{
				perror("recv-server disconnected:");
				tcsetattr(STDIN_FILENO, TCSANOW, &old_t);	//Define terminal como antes.	
				exit(EXIT_FAILURE);
			}

			int hand = handle_output_C(&RECV_PAC);	//Trata pacote recebido.
			if(hand != EXIT_SUCCESS)	//Comando recebido incorreto, ou servidor fechou.
			{
				if(hand == EXIT_CLOSE)	//Servidor fechou.
				{
					write(STDOUT_FILENO, "\nServidor foi fechado.\n", 23);
					close(sock);	//Fecha o socket.
					tcsetattr(STDIN_FILENO, TCSANOW, &old_t);	//Define terminal como antes.	
					exit(EXIT_FAILURE);
				}

                fprintf(stdout, "\nErro do servidor.\n");	//Não recebeu um pacote válido.
			}
			else
				printMessage_C(&RECV_PAC);	//Imprime o post recebido.
			
			memset((Packet *) &RECV_PAC, 0, sizeof(Packet));
		}
	}

    free(buffer);	//Libera espaço destinado ao buffer de entrada de dados.

	close(sock);	//Fecha o socket.

	tcsetattr(STDIN_FILENO, TCSANOW, &old_t);	//Define terminal com configurações normais.	

	return EXIT_SUCCESS;
}
