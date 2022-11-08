#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>

#define MAXLINE 1024    
#define BUFFSIZE 100

#define ERROR 0    	// Represents error
#define REQ 1		// Represents REQUEST for connection
#define ENDOFFILE 2	// Represents file is ended
#define ACK 3		// Represents acknowledgement of a particular sequence packet
#define DATA 4		// Represents DATA of the song requested

int PORT1, PORT2;

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
	perror(msg);
	exit(1);
}
/*
*	Datagram : sends song packets along with seqno, name of the file, and timestamps for latency calculation
*/
struct Datagram
{
	int type;
	int Seq_no;
	char filename[25];
	char buffer[MAXLINE];
};

struct LinkedList
{
    char buffer[BUFFSIZE];
    struct LinkedList *next;
};

char **listFiles(int *len)
{   
    int length=0;
    struct LinkedList *head = NULL;
    head = (struct LinkedList*)malloc(sizeof(struct LinkedList)); 
    
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d)
    {
        while((dir = readdir(d)) != NULL)
        {
            if(dir->d_type==8)
            {
                if(strstr(dir->d_name, ".mp4") || strstr(dir->d_name, ".mp3") || strstr(dir->d_name, ".wav"))
                {
                    struct LinkedList *current_node = head;
                    struct LinkedList *new_node;
                    while ( current_node != NULL && current_node->next != NULL) {
                        current_node = current_node->next;
                    }
                    new_node = (struct LinkedList *) malloc(sizeof(struct LinkedList));
                    strcpy(new_node->buffer, dir->d_name);
                    new_node->next= NULL;
                    if (current_node != NULL)
                        current_node->next = new_node;
                    else
                        head = new_node;
                }
            }
        }
        closedir(d);
    }
    struct LinkedList *tmp = head;
    while(tmp->next != NULL)
    {
        tmp = tmp->next;
        length++;
    }
    
    char **arr;
    int i=0;

    arr = malloc(sizeof(int*)*length);
    for(int j=0; j<length; j++)
    {
        arr[j] = malloc(sizeof(char*) * BUFFSIZE);
    }

    while(head->next != NULL)
    {
        head = head->next;
        strcpy(arr[i++], head->buffer);
    }
    *len = length;
    return arr;
}

/*
*	udp_send_list : sends number of songs and list of song file names using UDP protocol
*/
void *udp_send_list(void *param)
{
	char *ip_addr = (char*)param;
	char buffer[BUFFSIZE];
	//char *message = "Hello Client";
	int listenfd, len;
	struct sockaddr_in servaddr, cliaddr;
	bzero(&servaddr, sizeof(servaddr));

	// Create a UDP Socket
	listenfd = socket(AF_INET, SOCK_DGRAM, 0);
	servaddr.sin_addr.s_addr = inet_addr(ip_addr);
	servaddr.sin_port = htons(PORT2);
	servaddr.sin_family = AF_INET;

	// bind server address to socket descriptor
	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	while (1)
	{
		//receive the datagram
		int size = 0;
		len = sizeof(cliaddr);
		int n = recvfrom(listenfd, buffer, sizeof(buffer),
						 0, (struct sockaddr *)&cliaddr, &len); //receive message from server

		char **arr = listFiles(&size);
		sendto(listenfd, &size, sizeof(size), 0,
			   (struct sockaddr *)&cliaddr, sizeof(cliaddr));

		for (int j = 0; j < size; j++)
		{
			sendto(listenfd, arr[j], BUFFSIZE, 0,
				   (struct sockaddr *)&cliaddr, sizeof(cliaddr));
		}
	}

	pthread_exit(NULL);
}

void usage()
{
	printf("./server <ip-address> <port-no-1> <port-no-2>\n");
	printf("<port-no-1> - This port is for song transmission.\n");
	printf("<port-no-2> - This port is for transmitting list of songs available on the server side.\n");
	exit(8);
}

int main(int argc, char **argv)
{	
	//system("clear");
	if(argc<4)
	{
		usage();
	}
	PORT1 = atoi(argv[2]);
	PORT2 = atoi(argv[3]);
	int sockfd;					   /* socket */
	int clientlen;				   /* byte size of client's address */
	struct sockaddr_in serveraddr; /* server's addr */
	struct sockaddr_in clientaddr; /* client addr */
	int n;						   /* message byte size */
	struct Datagram data;
	

	/* 
   	* socket: create the parent socket 
   	*/
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	/*
   	* build the server's Internet address
   	*/
	bzero((char *)&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
	serveraddr.sin_port = htons(PORT1);

	/* 
   	* bind: associate the parent socket with a port 
   	*/
	if (bind(sockfd, (struct sockaddr *)&serveraddr,
			 sizeof(serveraddr)) < 0)
		error("ERROR on binding");

	/* 
	* List of songs thread: A thread is created which will infinitely send number of songs and list of file names on PORT2 
   	*/
   	pthread_t pthread;
	pthread_create(&pthread, NULL, &udp_send_list, (void*)argv[1]);
	
	/*
	* 	Infinitely accepts name of the file to be played from client side and sending that file packets to client side 
	*/
	clientlen = sizeof(clientaddr);
	while (1)
	{

		memset(&data, 0, sizeof(data));
		n = recvfrom(sockfd, (struct Datagram *)&data, sizeof(data), 0,
					 (struct sockaddr *)&clientaddr, &clientlen);

		if (data.type != REQ)
			continue;

		if (n < 0)
			error("ERROR in recvfrom");
		/* 
     	* Opens file name received and shows error if it doesn't exist 
     	*/
		FILE *fp;
		printf("Client requested: %s\n", data.filename);
		fp = fopen(data.filename, "r");
		if (fp == NULL)
		{
			printf("File %s not found!\n", data.filename);

			data.Seq_no = -1;
			data.type = ERROR;
			sendto(sockfd, (struct Datagram *)&data, sizeof(data),
				   MSG_CONFIRM, (const struct sockaddr *)&clientaddr,
				   clientlen);
			continue;
		}
		int cnt = 0;

		while (1)
		{
			memset(&data, 0, sizeof(data));

			data.Seq_no = cnt;
			data.type = DATA;
			int siz = fread(&data.buffer, sizeof(data.buffer), 1, fp);

			if (siz <= 0)
			{
				data.type = ENDOFFILE;
				sendto(sockfd, &data, sizeof(data),
					   0, (const struct sockaddr *)&clientaddr,
					   sizeof(clientaddr));
				break;
			}
			sendto(sockfd, (struct Datagram *)&data, sizeof(data),
				   MSG_CONFIRM, (const struct sockaddr *)&clientaddr,
				   clientlen);

			/*
			* STOP AND WAIT : Infinitely keeps receiving until appropiate ACK is received
			*/
			while (1)
			{
				struct Datagram data2;
				n = recvfrom(sockfd, (struct Datagram *)&data2, sizeof(data2), 0,
							 (struct sockaddr *)&clientaddr, &clientlen);

				if (data2.Seq_no == cnt)
					break;

				sendto(sockfd, (struct Datagram *)&data, sizeof(data),
					   MSG_CONFIRM, (const struct sockaddr *)&clientaddr,
					   clientlen);

			}
			cnt++;
		}

		printf("Song Buffered to the Client side.\n");
	}
}
