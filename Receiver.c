#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>//for socket operations
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<errno.h> //For errno - the error number
#include<netdb.h> //hostent

typedef struct packet{		//defines the packet to be sent
	char message_type[5];
	int seq_no;
	int ack_no;
	char data[200];
}Packet;
int main(int argc, char **argv)
{
	int sockfd, results, sent_num_bytes, recv_num_bytes;
	struct addrinfo hints, *senddetails, *p;
	struct sockaddr_storage senderAddr;
	char *buf = malloc(200);
	char *server_host;
	char *port;
	socklen_t addr_len;
	int total_size=0;
	int i=0;					
	FILE *fp;
	int m;

	if(argc != 7)						//checks if the command line arguments are in correct number or not
	{
		printf("Error in command line arguments\n");
		exit(1);
	}
	else
	{
		for(i=0;i<argc;i++)
		{
			if(strcmp(argv[i],"-m")==0)
			{
				m=atoi(argv[i+1]);
			}
			else if(strcmp(argv[i],"-h")==0) 
			{
				server_host=argv[i+1];
			}
			else if(strcmp(argv[i],"-p")==0)
			{
				port = argv[i+1];
			}
		}
	}

	memset(&hints,0,sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	if((results=getaddrinfo(NULL,port, &hints, &senddetails))!= 0)
	{
		fprintf(stderr, "getaddrinfo : %s\n", gai_strerror(results) );
		return 1;
	}

	for (p = senddetails ; p !=NULL; p = p->ai_next)
	{
		if((sockfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
				perror("receiver: socket");
				continue;
		}

		if(bind(sockfd,p->ai_addr, p->ai_addrlen)==-1)
		{
			
			perror("receiver:bind");
			close (sockfd);
			continue;
		}
		break;

	}

	if(p==NULL)
	{
		fprintf(stderr, "receiver: failed to bind to socket\n");
		return 2;
	}
	
	printf("Mode is %d - Waiting for data \n",m);

	addr_len = sizeof senderAddr;

	
	Packet init_packet;						//initialises the initial packet
	Packet sent_packet;
	Packet received_packet;
	int packet_id=0;
	int current_ack=0;
	int gbnflag=0;
	int recv_packet_size;

	if ((recv_packet_size = recvfrom(sockfd,&init_packet,sizeof(Packet),0, (struct sockaddr *)&senderAddr, &addr_len)) == 0) 
	         	{
	         		printf("Error in receiving data \n");
	         		exit(1);
	      		}
	if(strcmp(init_packet.message_type, "INIT")!=0)					//if the first message is not INIT, abort
	{
		printf("Error - First message is not an int\n");
		exit(1);
	}

	char filename[50];
	int x=0;
	//strcpy(filename, init_packet.data);
	while(init_packet.data[x]!='.')
	{
		filename[x]=init_packet.data[x];
		x++;
	}							//first packet sends the filename, file size, and mode
	int pid=getpid();
	char spid[10];
	snprintf(spid,10,"%d",pid);
	strcat(filename,spid);
	int x2=strlen(filename);
	x2--;
	filename[x2]='.';
	x2++;
	x++;
	while(init_packet.data[x]!='\0')
	{
		filename[x2]=init_packet.data[x];
		x++;
		x2++;
	}
	int d=0;
	int mode = init_packet.seq_no;
	int file_size = init_packet.ack_no;


	if(mode!=m)													//if the mode is on different machines, abort
	{
		printf("The modes are different on both machines\n");
		exit(1);
	}

	
	if(m==1) 							//checks if the protocol used is stop and wait or go back n
	{
		while(1)
			{
				
				if ((recv_packet_size = recv_num_bytes = recvfrom(sockfd,&received_packet,sizeof(Packet),0, (struct sockaddr *)&senderAddr, &addr_len)) == 0) 
	         	{
	         		printf("Error in receiving data \n");
	         		exit(1);
	      		}
	        	
				if((recv_packet_size>0) && (received_packet.seq_no == (packet_id+1)))
				{
					total_size= total_size + 200;
					fp = fopen(filename, "ab");
					if(total_size>file_size)								//if the data exceeds the file size, it breaks the loop
					{
						d = total_size-file_size;
						fwrite(&received_packet.data, sizeof(char), 200-d, fp);
						printf("Packet number %d received\n",received_packet.seq_no);
						fclose(fp);
						break;
					}
					fwrite(&received_packet.data, sizeof(char), 200, fp);
					fclose(fp);
					printf("Packet number %d received\n",received_packet.seq_no);
					packet_id++;
				}
			
				sent_packet.ack_no = packet_id;								//the receiver sends the acknowledgment for the packet
				sent_packet.seq_no = 0;
				strcpy(sent_packet.message_type, "ACK");
				sendto(sockfd, &sent_packet,sizeof(Packet), 0, (struct sockaddr*)&senderAddr, addr_len);
				printf("Acknowledgment sent for packet %d\n", sent_packet.ack_no);

			}
	}
	else if(m>1)								//code for go back n with window size m
	{
		while(1)
		{
			
			if ((recv_packet_size = recvfrom(sockfd,&received_packet,sizeof(Packet),0, (struct sockaddr *)&senderAddr, &addr_len)) == 0) 
	         	{
	         		printf("Error in receiving data \n");
	         		exit(1);
	      		}

			if((recv_packet_size>0) && (received_packet.seq_no == current_ack+1))
			{
				fp = fopen(filename,"ab");
				total_size=total_size+200;
				if(total_size>file_size)
				{
					int d=total_size-file_size;
					fwrite(&received_packet.data, sizeof(char), 200-d, fp);
					printf("Packet number %d received\n",received_packet.seq_no);
					fclose(fp);
					break;

				}
				
				fwrite(&received_packet.data, sizeof(char), 200, fp);
				fclose(fp);			
				printf("Packet number %d received\n",received_packet.seq_no);
				current_ack++;
			}
			else
			{
				gbnflag++;
			}
			
			if(received_packet.seq_no%m==0 && gbnflag==0)
			{
				sent_packet.ack_no = current_ack;
				strcpy(sent_packet.message_type, "ACK");
				sendto(sockfd, &sent_packet,sizeof(Packet), 0, (struct sockaddr*)&senderAddr, addr_len);;
				printf("Acknowledgment sent for packet %d\n", sent_packet.ack_no);
			}
			else if(gbnflag!=0)
			{
				sent_packet.ack_no=current_ack;
				sendto(sockfd, &sent_packet,sizeof(Packet), 0, (struct sockaddr*)&senderAddr, addr_len);;
				printf("Acknowledgment sent for packet %d\n", sent_packet.ack_no);
				gbnflag=0;
			}
		}
	}	
	
	close(sockfd);

	return 1;
}
