#/*******************************************************************************************************
#
#	Trabalho Prático 1: Chat Seletivo.
#	
#	Autores:	Felipe Tadeu Costa Ribeiro - 2013066028
#				Tomás Henrique Batista Aguiar - 2013066346
#				
#	Makefile
#
#*******************************************************************************************************/

all: proto funct cliente servidor clean

proto: protoIRC2.h
	gcc -Wall -Werror -std=c11 -pedantic -c protoIRC2.c

funct: funct.h
	gcc -Wall -Werror -std=c11 -pedantic -c funct.c

cliente: funct.o protoIRC2.o cliente.c
	gcc -Wall -Werror -std=c11 -pedantic -o cliente funct.o protoIRC2.o cliente.c

servidor: funct.o protoIRC2.o servidor.c
	gcc -Wall -Werror -std=c11 -pedantic -o servidor funct.o protoIRC2.o servidor.c
	
clean: funct.o protoIRC2.o
	rm *.o
