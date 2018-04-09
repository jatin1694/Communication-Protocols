#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/time.h>
#include<errno.h> //For errno - the error number
#include<netdb.h> //hostent

typedef struct packet{
	char message_type[5];
	int seq_no;
	int ack_no;
	char data[200];
}Packet;
int main(int argc, char **argv)
{
	int sockfd;
	struct addrinfo hints, *clientdetail, *p;
	char *buf = malloc(200); //allocates memory for the packet (200 is the packet size)

	char *client_host;		//variables to store the command line arguments
	char *port;
	char filename[100];
	int status;
	struct sockaddr_storage recvAddr;
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];

	int m;
	FILE *fp;	
	struct timeval timeout;      
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

	Packet sent_packet;
	Packet received_packet;
	int packet_id=1;
	int i=0,flag=0,j=0,flag2=0;
	sent_packet.seq_no=0;
	int total_size=0;
	
	if(argc != 9)												//checks if the command line arguments are in correct number
	{
		printf("Error in command line arguments\n");
		exit(1);
	}
	else														//takes in the command line arguments
	{
		for(i=0;i<argc;i++)
		{
			if(strcmp(argv[i],"-m")==0)
			{
				m=atoi(argv[i+1]);
			}
			else if(strcmp(argv[i],"-h")==0) 
			{
				client_host=argv[i+1];
			}
			else if(strcmp(argv[i],"-p")==0)
			{
				port=(argv[i+1]);
			}
			else if(strcmp(argv[i],"-f")==0)
			{
				strcpy(filename,argv[i+1]);
			}
		}
	}
	
	//code to initialize the socket
	memset(&hints,0,sizeof hints);
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;


	if((status=getaddrinfo(client_host,port,&hints,&clientdetail))!=0)
	{
		fprintf(stderr, "getaddrinfo: %s\n",gai_strerror(status));
		return 1;
	}

	for(p=clientdetail; p!=NULL;p=p->ai_next)
	{
		if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1)
		{
			perror("sender: socket\n");
			continue;
		}
		break;
	}

	if(p==NULL)
		{
			fprintf(stderr, "sender: failed to create a socket\n" );
			return 2;
		}	

	addr_len = sizeof(recvAddr);
	
	fp=fopen(filename, "rb");
	if (setsockopt (sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
                sizeof(timeout)) < 0)
        printf("setsockopt failed\n");


    sent_packet.seq_no=0;
    received_packet.ack_no=0;
    int file_size;
    int numbytes;
    fseek(fp, 0L, SEEK_END);
	file_size = ftell(fp);
	rewind(fp);
	int read_bytes;
	Packet init_packet;
	strcpy(init_packet.message_type, "INIT");
	init_packet.seq_no = m;
	init_packet.ack_no = file_size;
	strcpy(init_packet.data, filename);
	int init_sent_bytes = sendto(sockfd,&init_packet,sizeof(Packet),0,p->ai_addr, p->ai_addrlen);
	if(init_sent_bytes<0)
	{
		printf("ERROR");
		exit(1);
	}

	if(m==1)														//code for STOP and WAIT
	{
	
		while(1)
		{
			if(sent_packet.seq_no==packet_id-1)
			{
				read_bytes=fread(&sent_packet.data, sizeof(char),200,fp);	
				sent_packet.ack_no=0;
				sent_packet.seq_no=packet_id;
				strcpy(sent_packet.message_type, "DATA");
				if ((numbytes = sendto(sockfd,&sent_packet,sizeof(Packet),0,p->ai_addr, p->ai_addrlen)) == -1)			//sends packet to the receiver 
				{

      						printf("sendto error");
      						exit(1);
    			}
    			printf("Packet of sequence number %d is sent\n",sent_packet.seq_no);
						
			}
			
			
			int recv_packet_size;
			recv_packet_size = recvfrom(sockfd,&received_packet,sizeof(Packet),0,(struct sockaddr *)&recvAddr,&addr_len);		//waits to receive the acknowledgement


			if(recv_packet_size<=0)																								//checks if the ack was received within the timeout limit
				{
					printf("Timeout reached for packet number %d\n", sent_packet.seq_no);
					sendto(sockfd,&sent_packet,sizeof(Packet),0,p->ai_addr, p->ai_addrlen);
					printf("Retransmitted packet number %d\n",sent_packet.seq_no);
				}
			else if((recv_packet_size>0) && (received_packet.ack_no == sent_packet.seq_no))										//if the packet received is not in order, it resends the previous packets
				{
					printf("Acknowledgment of packet number %d is received\n\n", received_packet.ack_no);
					packet_id++;
					total_size+=sizeof(sent_packet);
				}
			if(feof(fp))
					break;

		}
	}
	else if (m>1)											//code for Go Back N where N is m
	{
		while(1)
		{
			
			if((sent_packet.seq_no == (packet_id-1)) && ((received_packet.ack_no%m) == 0))
			{
				
				read_bytes=fread(&sent_packet.data, sizeof(char),200,fp);
				sent_packet.ack_no=0;
				sent_packet.seq_no=packet_id;
				strcpy(sent_packet.message_type, "DATA");
				if ((numbytes = sendto(sockfd,&sent_packet,sizeof(Packet),0,p->ai_addr, p->ai_addrlen)) == -1) 
				{

      				printf("sendto error");
      				exit(1);
    			}
    			printf("Packet of sequence number %d is sent\n",sent_packet.seq_no);

				packet_id++;

			}
			
			else
			{
				int bytes_resend = 200 * received_packet.ack_no;								//if the acknowledgement received is not a multiple of window size, 
				fseek(fp, bytes_resend, SEEK_SET);												//send the last acknowledgment received till the nearest multiple of window size
				while((sent_packet.seq_no%m)!=1)
				{
					if(flag==0)
					{
						packet_id = received_packet.ack_no + 1;
						flag++;
					}
					read_bytes=fread(&sent_packet.data, sizeof(char),100,fp);
					sent_packet.seq_no=packet_id;
					sent_packet.ack_no=0;
					strcpy(sent_packet.message_type, "DATA");
					sendto(sockfd,&sent_packet,sizeof(Packet),0,p->ai_addr, p->ai_addrlen);
					printf("Packet of sequence number %d is sent\n", sent_packet.seq_no);
					packet_id++;
				}
				sent_packet.seq_no--;
			}

			if((sent_packet.seq_no%m)==0 && (sent_packet.seq_no>1))
			{
				
				int recv_packet_size = recvfrom(sockfd,&received_packet,sizeof(Packet),0,(struct sockaddr *)&recvAddr,&addr_len);

				if(recv_packet_size<=0)
				{
					printf("Timeout reached for packet number %d\n", sent_packet.seq_no);
					sendto(sockfd,&sent_packet,sizeof(Packet),0,p->ai_addr, p->ai_addrlen);
					printf("Retransmitted packet number %d\n",sent_packet.seq_no);
				}

				if((recv_packet_size>0) && received_packet.ack_no == sent_packet.seq_no)
				{
					printf("Acknowledgement of packet number %d is received\n\n", received_packet.ack_no);
					
				}
			}
			if(feof(fp))
					break;
			
		}
	}
					
close(sockfd);
fclose(fp);
}

