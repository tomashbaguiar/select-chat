/*******************************************************************************************************

	Trabalho Prático 1: Chat Seletivo.
	
	Autores:	Felipe Tadeu Costa Ribeiro - 2013066028
				Tomás Henrique Batista Aguiar - 2013066346
				
	servidor.c
	
*******************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <errno.h>

#include <sys/time.h>
#include <unistd.h>

#include "funct.h"
#include "protoIRC2.h"

int main(int argc, char *argv[])
{
	if(argc != 2)	//	./servidor port_num
	{
		fprintf(stderr, "\nNúmero de argumentos errado. Correto: 2.\n");
		exit(EXIT_FAILURE);
	}

	int master;	//Recebe o descritor do socket master.
	if((master = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
	{	
		perror("socket:");
		exit(EXIT_FAILURE);
	}

	int opt = 1;	//TRUE.
	//Opções do socket master. Receber múltiplas conexões.
	if((setsockopt(master, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt))) < 0)
	{
		perror("setsockopt:");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in address; //Guarda as informacoes dos clientes.
	address.sin_family = AF_INET;	//IPv4.
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(atoi(argv[1]));	//Porta.

	//Bind à porta de localhost: argv[1].
	if((bind(master, (struct sockaddr *) &address, sizeof(address))) < 0)
	{
		perror("bind:");
		exit(EXIT_FAILURE);
	}

	//Conexões pendentes. Listen.
	if((listen(master, 3)) < 0)
	{
		perror("listen:");
		exit(EXIT_SUCCESS);
	}

	Client clients[CLI_MAX];
	//uint8_t matrixStatus[CLI_MAX][CLI_MAX];
	uint8_t **matrixStatus = (uint8_t **) malloc(CLI_MAX * sizeof(uint8_t));

	//Inicializa todos os clientes com 0 (não-checado).
	for(uint8_t i = 0; i < CLI_MAX; i++)
	{
		clients[i].id = 0;              //Inicializa todos os clientes com 0
		clients[i].nickname[0] = '\0';  // e todos os nicknames como vazio.

		for(uint8_t j = 0; j < CLI_MAX; j++)
			matrixStatus[i] = (uint8_t *) malloc(CLI_MAX * sizeof(uint8_t));    //Aloca espaco para matriz.
	}

	for(uint8_t i = 0; i < CLI_MAX; i++)
	{
		for(uint8_t j = 0; j < CLI_MAX; j++)
			matrixStatus[i][j] = 1;	//Todos recebem de todos.
	}

	fd_set rfd;	//Sockets em select.

	uint8_t max_cli = CLI_MAX;  
	int vrd, s, new_socket, activity;

	//Aceita conexões.
	int addrlen = sizeof(address);
	fprintf(stdout, "Waiting for connections...\n");

	Packet RECV_PAC;    //Cria espacao para pacote a ser recebido.

	while(1)    //Loop infinito.
	{
		FD_ZERO(&rfd);	//Limpa o conjunto de select.
		FD_SET(master, &rfd);	//Coloca o socket master no conjunto.

		//Coloca os sockets dos clientes no conjunto.
		for(uint8_t i = 0; i < CLI_MAX; i++)
		{
			s = clients[i].id;
			if(s > 0)
				FD_SET(s, &rfd);
		}

		//Espera acontecer algo no conjunto (indefinidamente).
		activity = select(max_cli + 3, &rfd, NULL, NULL, NULL);
		
		if((activity < 0) && (errno != EINTR))      //Se select retornou um erro e nenhum sinal foi recebido
			fprintf(stderr, "select.");             // informa a STDERR_FILENO.

		//Se tem atividade no master socket, é uma nova conexão.
		if(FD_ISSET(master, &rfd))
		{
			if((new_socket = accept(master, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0)
			{
				perror("accept:");
				exit(EXIT_FAILURE);
			}

			//Informar que novo cliente entrou.
			fprintf(stdout, "New user: fd %d, ip: %s, port: %d.\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
			
			//Coloca novo cliente no conjunto de clientes.
			for(uint8_t i = 0; i < CLI_MAX; i++)
			{
				s = clients[i].id;      //s recebe o id do cliente a ser testado em if.
				if(s == 0)          //Se esse id nao contem nenhum socket
				{                   // coloca o novo socket nesse id.
					clients[i].id = new_socket;
					i = CLI_MAX;    //Sai do loop.
				}
			}
		}

		//Atividade de entrada e saída.
		for(uint8_t i = 0; i < CLI_MAX; i++)
		{
			s = clients[i].id;      //s recebe o socket do client i.
			
			if(FD_ISSET(s, &rfd))       //Testa se houve sinal de s.
			{
                memset((Packet *) &RECV_PAC, 0, sizeof(Packet));

				vrd = PROTO_RECV(&RECV_PAC, s);
				if(vrd == 0)	//Se s fechou a conexão.
				{
					//Imprime informações de s.
					getpeername(s, (struct sockaddr *) &address, (socklen_t *) &addrlen);       //address recebe as informacoes de s.
					fprintf(stdout, "%s disconnected, ip: %s, fd = %d.\n", clients[i].nickname, inet_ntoa(address.sin_addr), clients[i].id);    //Imprime-as.
					Packet EXIT_PAC;    //Cria um pacote para informar que s estah offline.
					EXIT_PAC.cmd = 2;	//POST.
					strcpy(EXIT_PAC.nickname, clients[i].nickname);
					strcpy(EXIT_PAC.message, "Está offline.\n");

					S_NEW_CMD(&EXIT_PAC, matrixStatus, i, clients); //Envia esse pacote com o comando NEW.

					//Fecha a conexão com s.
					close(s);   
					clients[i].id = 0;          //Coloca 0 no cliente i, para reaproveita-lo.
					clients[i].nickname[0] = '\0';  //Retira o nickname de s.

					for(uint8_t j = 0; j < CLI_MAX; j++)
						matrixStatus[i][j] = 1;		//Todos os clientes mutados pelo que logged off voltam ao estado normal (1 - unmuted).
				}
				else if(vrd < 0)    //Se s fechou a conexao de forma abrupta (devido a erro).
				{
					perror("recv:");
                    fprintf(stderr, "Connection reseted by peer %s.\n", clients[i].nickname);
					//exit(EXIT_FAILURE);   //Algum cliente foi fechado por erro (Connection reset by peer).
                    						//  Servidor nao pode fechar por isto.
				}
				else        //Se nao houve erros no cliente e ele nao fechou a conexao.
				{
					int status = handle_input_S(&RECV_PAC, i, matrixStatus, clients);       //Trata o arquivo recebido.

					switch(status)      //Retorno de handle_input_S.
					{
						case EXIT_SUCCESS:
							break;
						case EXIT_FAILURE:      //Nao foi possivel tratar o pacote.
							fprintf(stderr, "handle:");
							exit(EXIT_FAILURE);
						case EXIT_ERR:          //Se houve erro do cliente com o tratamento do pacote.
							fprintf(stderr, "Client's error.\n");
							break;
						case EXIT_NICK:         //Se nao foi possivel tratar o comando NICK do cliente.
							fprintf(stderr, "Handle NICK error.\n");
							break;
						case EXIT_MUTE:         //Se nao foi possivel tratar o comando MUTE do cliente.
							fprintf(stderr, "Handle MUTE error.\n");
							break;
						case EXIT_UNMUTE:       //Se nao foi possivel tratar o comando UNMUTE do cliente.
							fprintf(stderr, "Handle UNMUTE error.\n");
							break;
						default:
							fprintf(stderr, "Unknown error.\n");
					}
				}
			}
		}
	}

	for(uint8_t i = 0; i < CLI_MAX; i++)
		free(matrixStatus[i]);      //Desaloca espaco para matriz.
    free(*matrixStatus);   

	return EXIT_SUCCESS;
}
