/* client.cpp

    This program transfers a file to a remote server, possibly through an
    intermediary relay.  The server will respond with an acknowledgement.
    
    This program then calculates the round-trip time from the start of
    transmission to receipt of the acknowledgement, and reports that along
    with the average overall transmission rate.
    
    Written by _______________ for CS 450 HW1 January 2014
*/

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "CS450Header6.h"

using namespace std;

uint16_t calcCheckSum(void* addr, int nBytes)
{
uint16_t *addr16 = (uint16_t*)addr;
uint8_t *addr8 = (uint8_t*)addr;

uint32_t sum = 0, max16 = 0xffff;

int n;
for(n = 0; n < nBytes/2; n++)
{
        sum += addr16[n];
        if(sum > max16)
        sum -= max16;
}

if((nBytes % 2) == 1)
{
        sum += addr8[(nBytes - n)];
        if(sum > max16)
        sum -= max16;
}

return (~sum);
}


int main(int argc, char *argv[])
{
	printf("Zihang Feng.\n");
        printf("CS account jfeng9.\n");
        printf("665726671.\n");
        printf("This is the Client Application.\n");

    // User Input
    
    /* Check for the following from command-line args, or ask the user:
        
        Destination ( server ) name and port number
        Relay name and port number.  If none, communicate directly with server
        File to transfer.  Use OPEN(2) to verify that the file can be opened
        for reading, and ask again if necessary.
    */
	struct hostent *hostIP, *clntIP, *relayIP;
	int serPort, relayPort;
	struct sockaddr_in clntAddr, serAddr;
	if(argc < 2)
	{
		hostIP = gethostbyname("127.0.0.1");
                if(hostIP == NULL)
                {
                        printf("Can't convert the host IP\n");
                        exit(-1);
                }

		serPort = 54323;
	}
	else if(argc < 3)
	{
		hostIP = gethostbyname(argv[1]);
                if(hostIP == NULL)
                {
                        printf("Can't convert the host IP\n");
                        exit(-1);
                }
		serPort = 54323;
	}
	else if(argc < 4)
	{
		hostIP = gethostbyname(argv[1]);
		if(hostIP == NULL)
		{
			printf("Can't convert the host IP\n");
			exit(-1);
		}
		serPort = atoi(argv[2]);
	}
        else if(argc < 5)
        {
                hostIP = gethostbyname(argv[1]);
                if(hostIP == NULL)
                {
                        printf("Can't convert the host IP\n");
                        exit(-1);
                }
                serPort = atoi(argv[2]);
		relayIP = gethostbyname(argv[3]);
                relayPort = 54322;
        }
	else
	{
		hostIP = gethostbyname(argv[1]);
                if(hostIP == NULL)
                {
                        printf("Can't convert the host IP\n");
                        exit(-1);
                }
                serPort = atoi(argv[2]);
		relayIP = gethostbyname(argv[3]);
		relayPort = atoi(argv[4]);
	}

    // Use FSTAT and MMAP to map the file to a memory buffer.  That will let the
    // virtual memory system page it in as needed instead of reading it byte
    // by byte.


	char file_name[80];
	printf("Give a file name: ");
	scanf("%s",file_name);
	printf("\n");

	int filefd;
        if((filefd = open(file_name, O_RDONLY)) < 0)
        {
        	printf( "method open has problem.\n");
               	exit(-1);
        }

	struct stat filestatus;
	if(fstat(filefd, &filestatus) < 0)
	{
		printf("Can't find the size of the file.\n");
		exit(-1);
	}
	int size_of_file = filestatus.st_size;

	// missing a file size as the second argument of the mmap
	void *addr;
	if((addr = mmap(NULL, size_of_file, PROT_READ, MAP_PRIVATE, filefd, 0)) == (void *)-1)
        {
        	printf("method mmap has problem.\n");
		close(filefd);
               	exit(-1);
        }


	char clntHostName[255];
	if(gethostname(clntHostName,255) < 0)
	{
		printf("Can't get the client host name.\n");
		exit(-1);
	}

	if((clntIP = gethostbyname(clntHostName)) == NULL)
	{
		printf("Can't convert it the client host name.\n");
		exit(-1);
	}

	if(argc > 3) // If relay exists
	{
                serAddr.sin_family = relayIP->h_addrtype;
                memcpy((char*)&serAddr.sin_addr.s_addr, relayIP->h_addr_list[0],relayIP->h_length);
                serAddr.sin_port = htons(relayPort);
	}
	else
	{
	        serAddr.sin_family = hostIP->h_addrtype;
        	memcpy((char*)&serAddr.sin_addr.s_addr, hostIP->h_addr_list[0],hostIP->h_length);
		serAddr.sin_port = htons(serPort);
	}


    // Open a Connection to the server ( or relay )  TCP for the first HW
    // call SOCKET and CONNECT and verify that the connection opened.

	int UDPsockfd = socket(AF_INET,SOCK_DGRAM, 0);
	if(UDPsockfd < 0)
	{
		printf("Socket Erorr.\n");
		close(filefd);
		exit(-1);
	}

	clntAddr.sin_family = clntIP->h_addrtype;
        memcpy((char*)&clntAddr.sin_addr.s_addr, clntIP->h_addr_list[0],clntIP->h_length);
        clntAddr.sin_port = htons(INADDR_ANY);


	socklen_t clntLen = sizeof(clntAddr);
        if((bind(UDPsockfd, (struct sockaddr *) &clntAddr, clntLen)) < 0)
        {
                printf("Can't bind the sockfd in the client side\n.");
                close(UDPsockfd);
                exit(-1);
        }

	if(getsockname(UDPsockfd, (struct sockaddr *) &clntAddr, &clntLen) < 0)
	{
		printf("Can't use the getsockname.\n");
		close(UDPsockfd);
		exit(-1);
	}
    // Note the time before beginning transmission
	clock_t start = clock();
    // Create a CS450Header object, fill in the fields, and use SEND to write
    // it to the socket.

	Packet pack;
	pack.header.version = 4;
	pack.header.UIN = 665726671;
	pack.header.HW_number = 1;
	strcpy(pack.header.ACCC,"jfeng9");
	strcpy(pack.header.filename, file_name);
	pack.header.from_IP = htonl(clntAddr.sin_addr.s_addr);

	memcpy((char*)&pack.header.to_IP, hostIP->h_addr_list[0],hostIP->h_length);
        pack.header.to_Port = htons(serPort);

	pack.header.from_Port = clntAddr.sin_port;
	pack.header.nTotalBytes = size_of_file;
	pack.header.relayCommand = 0;
	pack.header.saveFile = 1;;


//	clntAddr.sin_family = AF_INET;
//	clntAddr.sin_addr.s_addr = htons(INADDR_ANY);
//	clntAddr.sin_port = htons(serPort);
    // Use SEND to send the data file.  If it is too big to send in one gulp
    // Then multiple SEND commands may be necessary.

	socklen_t serLen = sizeof(serAddr);
        if((sendto(UDPsockfd, &pack, sizeof(pack), 0,(struct sockaddr *) &serAddr, serLen)) < 0)
	{
		printf("Can't send\n");
		exit(-1);
	}

	char ackValue[80];
	ssize_t length_recieved = recvfrom(UDPsockfd,ackValue, 80, 0,(struct sockaddr *) &serAddr, &serLen);
	if(length_recieved < 0)
	{
		printf("Recv Error.\n");
		close(UDPsockfd);
		close(filefd);
		exit(-1);
	}
	else
	{
		int n = 0;
		while(n < size_of_file)
		{
			if((size_of_file - n) > BLOCKSIZE)
			{
				pack.header.nbytes = BLOCKSIZE;
			}
			else
			{
				pack.header.nbytes = size_of_file - n;
			}
			memcpy(pack.data, (char *)addr + n, pack.header.nbytes);
			pack.header.checksum = calcCheckSum(pack.data, pack.header.nbytes);
			if((sendto(UDPsockfd, &pack, sizeof(pack), 0,(struct sockaddr *) &serAddr, serLen)) < 0)
        		{
                		printf("Can't send\n");
                		exit(-1);
        		}

        		length_recieved = recvfrom(UDPsockfd,ackValue, 80, 0,(struct sockaddr *) &serAddr, &serLen);
        		if(length_recieved < 0)
        		{
                		printf("Recv Error.\n");
                		close(UDPsockfd);
                		close(filefd);
                		exit(-1);
        		}
			else
			{
				if(atoi(ackValue))// the piece has been sent accurately
				{
					n += pack.header.nbytes;
				}
			}

		}
	}

    // Calculate the round-trip time and
    // bandwidth, and report them to the screen along with the size of the file
    // and output echo of all user input data.

	clock_t end = clock();
        float seconds = (float)(end - start) / CLOCKS_PER_SEC;
	
	printf("Time it takes to the destination and back is %f\n", seconds);

	CS450Header headerValue;

        length_recieved = recvfrom(UDPsockfd, &headerValue, sizeof(headerValue), 0,(struct sockaddr *) &serAddr, &serLen);
        if(length_recieved < 0)
        {
                printf("Recv Error.\n");
                close(UDPsockfd);
                close(filefd);
                exit(-1);
        }

        if(headerValue.packetType == 1)
        {
                printf("File has been accurately transfered.\n");
        }
        else if(headerValue.packetType == 2)
	{
		printf("It is acknowledgement from the server.\n");
	}
	else if(headerValue.packetType == 3)
	{
		printf("It is both file transfer and acknowledgement.\n");
	}
	else
        {
                printf("Something wrong with the packetType.\n");
                exit(-1);
        }

        close(filefd);
        close(UDPsockfd);
    // When the job is done, either the client or the server must transmit a
    // CS450Header containing the command code to close the connection, and 
    // having transmitted it, close their end of the socket.  If a relay is 
    // involved, it will transmit the close command to the other side before
    // closing its connections.  Whichever process receives the close command
    // should close their end and also all open data files at that time.
    
    // If it is desired to transmit another file, possibly using a different
    // destination, protocol, relay, and/or other parameters, get the
    // necessary input from the user and try again.
    
    // When done, report overall statistics and exit.

    return 0;
}

