#Compile Vehicle Tracking System
CC = gcc
CXX = g++
LTB = -lpthread
TARGET = VTS_Server LocGen CLI_Client
OBJ = VTS_Server.o LocGen.o CLI_Client.o
all: $(OBJ)
	$(CXX) -o VTS_Server VTS_Server.o $(LTB)
	$(CC) -o LocGen LocGen.o $(LTB)
	$(CC) -o CLI_Client CLI_Client.o
VTS_Server.o: VTS_Server.c
	$(CXX) -c $<
LocGen.o: LocGen.c
	$(CC) -c $<
CLI_Client.o: CLI_Client.c
	$(CC) -c $<
clean:
	$(RM) *.o $(TARGET)
.PHONY: run clean
