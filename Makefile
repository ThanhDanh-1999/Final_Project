#Compile Vehicle Tracking System
CC = gcc
CXX = g++
LTB = -lpthread

main: VTS_Server.o LocGen.o CLI_Client.o 
	$(CXX) -o VTS_Server VTS_Server.o $(LTB)
	$(CC) -o LocGen LocGen.o $(LTB)
	$(CC) -o CLI_Client CLI_Client.o
VTS_Server.o: VTS_Server.c
	$(CXX) -c VTS_Server.c
LocGen.o: LocGen.c
	$(CC) -c LocGen.c
CLI_Client.o: CLI_Client.c
	$(CC) -c CLI_Client.c
clean:
	$(RM) *.o VTS_Server LocGen CLI_Client 
.PHONY: run clean
