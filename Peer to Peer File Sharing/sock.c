#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#define MAX_CLIENT 10

#define NUMBER_OF_CLIENTS 5

#define MAX_DATA_PER_CLIENT 512

#define MAXPORTSIZE 6

const char* get_my_ip();
void *get_in_addr(struct sockaddr *sa);
const char* getlist(char id[3],char name[50], char ip[16], char port[6]);
const char* translist(char servlist[10][512],int cid);
int commandtokenizer(char ui[20]);

int flag_registered = 0;

fd_set c_master;		//collecting file descriptor
fd_set c_read_fds;
fd_set c_peers;
//fd_set c_server;
// fd sets for server...............................................
fd_set s_master;		//collecting file descriptor
fd_set s_read_fds;
fd_set s_client;
fd_set s_working_set;



int main(int argc, char *argv[])
{

	char mode[3];
	strcpy(mode,argv[1]);
	int flags = fcntl(0,F_GETFL,0);
	if(fcntl(0,F_SETFL,flags | O_NONBLOCK))
	{
		printf("Control error");
	}
	if(strcmp(mode,"c")==0){
		printf("in client");
		callclient(argv[2]);
	}
	else if(strcmp(mode,"s")==0){
		printf("in server");
		callserver(argv[2]);
	}


}


int callserver(char myhome_port[MAXPORTSIZE]){

	int sd_server,sd_client;		//socket Descriptor for the litening  on server and for accept()ing clients
	static int conn_id =0;
	struct addrinfo ai_server , *result, *rp;
	struct sockaddr_storage client_addrs;
	struct timeval t;
	socklen_t addr_len;

	char myhome_ip4[INET_ADDRSTRLEN];

	int int_myhome_port;

	char client_data[256];
	int numbytes;

	int fdmax;

	char c[3],buf[1024];
	char ip4_client[INET_ADDRSTRLEN];
	char client_request[100];
	char hostname[50];
	char server_ip_list[NUMBER_OF_CLIENTS][MAX_DATA_PER_CLIENT];

	int i,j,s, status;
	int yes=1;


	char ds[5][20];
//====================================================================================================================================

	int_myhome_port = atoi(myhome_port);
	strcpy(myhome_ip4,get_my_ip());
	printf("LISTENING on PORT NUMBER : %s\n", myhome_port);
	printf("My IP has been retrieved to be: %s\n", myhome_ip4);


//====================================================================================================================================

//Declaring parameters for my struct..............................

	memset(&ai_server, 0, sizeof(ai_server));
	ai_server.ai_flags = AI_PASSIVE;
	ai_server.ai_family = AF_INET;
	ai_server.ai_socktype = SOCK_STREAM;
	ai_server.ai_protocol = 0;
	t.tv_sec=1;
	t.tv_usec=1000;
	FD_ZERO(&s_master);
	FD_ZERO(&s_read_fds);
	FD_ZERO(&s_client);

	s = getaddrinfo(myhome_ip4, myhome_port, &ai_server, &result);
        if (s != 0) {
               perror(" server  : getaddrinfo: \n");
               return 2;
        }


	for (rp = result; rp != NULL; rp = rp->ai_next) {
               sd_server = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if (sd_server == -1)
                  continue;

		setsockopt(sd_server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        	if (bind(sd_server, rp->ai_addr, rp->ai_addrlen) < 0)
                   continue;

		break;                  /* Success */

        }


	if (rp == NULL) {               /* No address succeeded */
		//close(sd_server);
		perror(" server : Could not bind\n");
               	return 3;
        }

	freeaddrinfo(result);

	if (listen(sd_server, MAX_CLIENT) == -1) {
		perror("server : listen");
		return 4;
	}

	FD_SET(0,&s_master);// stdin master
	FD_SET(sd_server, &s_master); //keep track the biggest file descriptor

	fdmax = sd_server; // so far, it's this one

	printf("server: waiting for connections...\n");
	while(1)
	{ // main accept() loop
		s_read_fds = s_master;

		if (select(fdmax+1, &s_read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(0);
		}

		for(i=0 ; i<= fdmax; i++){
		   if (FD_ISSET(i, &s_read_fds)) { // we got one!!
			if (i == sd_server) {//handle new connections
					addr_len = sizeof(client_addrs);
					sd_client = accept(sd_server, (struct sockaddr *)&client_addrs, &addr_len);
					if (sd_client == -1) {
					perror("server : accept error!!");
					continue;
					}
					else{
					FD_SET(sd_client, &s_client);	// add to client list
					FD_SET(sd_client, &s_master); // add to master set
					if (sd_client > fdmax) {// keep track of the max
						fdmax = sd_client;
					}
					inet_ntop(client_addrs.ss_family,get_in_addr((struct sockaddr *)&client_addrs),ip4_client, 										sizeof ip4_client);
					getnameinfo((struct sockaddr *)&client_addrs, &addr_len, hostname, sizeof(hostname), NULL, 0, 0);
					printf("\nserver: got connection from %s\n on socket :%d ", ip4_client,sd_client);
				}
			}else if(i == sd_client){
					if ((numbytes = recv(sd_client, client_request, 100, 0)) <= 0) {
						//FD_CLR(sd_client,&s_master);
						perror("recvfrom");
						return 5;

					}
					else{

						printf("\nNumber of bytes : %d", numbytes);
						client_request[numbytes]='\0';
						printf("\nClient listening port is : %s", client_request);
						printf("\nName of the Client is : %s", hostname);
						sprintf(c, "%d", conn_id);
						strcat(c,"\0");
						strcpy(server_ip_list[conn_id],getlist(c,hostname,ip4_client,client_request));
						printf("\nConnection details are : %s", server_ip_list[conn_id]);
						conn_id++;
						for(j = 0; j <= fdmax; j++) {
							printf("im in the loop  and j = %d",j)
							if (FD_ISSET(j, &s_client)) {
							// except the listener and ourselves
								if (j != sd_server && j != 0) {
									strcpy(buf,"");
									strcat(buf,c);
									strcat(buf,",");
									strcat(buf,translist(server_ip_list,conn_id));
									if (send(j, buf, 1024, 0) == -1) {
										perror("send");
									}
								}
							}
						}

					}
		fflush(stdout);}
		}

	}

}
	return 0;


}

int callclient(char myhome_port[MAXPORTSIZE]){

	int my_sd,sd_peer,sd_client;		//socket Descriptor for the litening  on server and for accept()ing clients

	struct addrinfo my_server, my_ai, ai_client, *result, *rp;
	struct sockaddr_storage peer_addrs;
	socklen_t addr_len;
	struct timeval t;
	char myhome_ip4[INET_ADDRSTRLEN];
	int int_myhome_port;

	char my_data[256];
	int numbytes,nc;

	char client_data[256];
	int cli_numbytes;

	int fdmax;

	char c[3],buf[1024];
	char ip4_client[INET_ADDRSTRLEN];
	char client_request[100];
	char hostname[50];
	char server_ip_list[NUMBER_OF_CLIENTS][MAX_DATA_PER_CLIENT];

	int n=0,cval,i,j,s, status,conn_count;
	int yes=1;
	char ds[5][20];
	char user_input[50];

//========================================================================================================================================

	int_myhome_port = atoi(myhome_port);
	strcpy(myhome_ip4,get_my_ip());
	printf("LISTENING on PORT NUMBER : %s\n", myhome_port);
	printf("My IP has been retrieved to be: %s\n", myhome_ip4);


//=======================================================================================================================================

//Declaring parameters for my struct..............................



	// my socket open for listening....to peers................................................
	memset(&my_ai, 0, sizeof(my_server));
	my_ai.ai_flags = AI_PASSIVE;
	my_ai.ai_family = AF_INET;
	my_ai.ai_socktype = SOCK_STREAM;
	my_ai.ai_protocol = 0;

	s = getaddrinfo(myhome_ip4, myhome_port, &my_ai, &result);
        if (s != 0) {
               perror(" client as server  : getaddrinfo: \n");
               return 2;
        }

	for (rp = result; rp != NULL; rp = rp->ai_next) {
               sd_peer = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if (sd_peer == -1)
                  continue;

		setsockopt(sd_peer, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        	if (bind(sd_peer, rp->ai_addr, rp->ai_addrlen) < 0)
                   continue;

		break;                  /* Success */

        }


	if (rp == NULL) {               /* No address succeeded */
		//close(sd_server);
		perror(" server : Could not bind\n");
               	return 3;
        }



	freeaddrinfo(result);
	//---=========---------============---------------==============-----------------============------------==============--------

	FD_ZERO(&c_master);
	FD_ZERO(&c_read_fds);
	FD_ZERO(&c_peers);
	//fcntl(0,F_SETFD,O_NONBLOCK);


	FD_SET(0, &c_master);// stdin master
	FD_SET(sd_peer, &c_master); // my listening socket

	fdmax = sd_peer; // so far, it's this one
	//t.tv_usec=1000;
	t.tv_sec = 5;
	int_myhome_port = atoi(myhome_port);

	while(1) { // main accept() loop
		c_read_fds = c_master;
		memset(user_input,0,sizeof(user_input));
		if (select(fdmax+1, &c_read_fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(0);
		}

		for(i=0 ; i<= fdmax; i++){
		  if(i == 0){

			cval=0;
			if((n=read(0, user_input,sizeof(user_input)))<0)
			continue;

			user_input[n-1]='\0';
			printf("User input : %s %d",user_input,n);
			for(n=0;n<5;n++){
				strcpy(ds[n],"");
			}
			n=0;
			char *token = strtok(user_input, " ");
			while ( token )
   			{
			   if ( strlen(token) > 1 ){

				strcpy(ds[n], token);
				 n++;
		           }
      				token = strtok(NULL, " ");
  			}
			token = NULL;
			printf("\nno of tokens : %d", n);
			printf("\ntokens : %s", ds[0]);

			//commandtokenizer = NULL;
			cval = commandtokenizer(ds[0]);    // passing the command name

			printf("switch value is : %d" , cval);
			switch(cval)
			{
    			case 1:
        			printf("do something to help!!");
      	 		break;
    			case 2:
      		               	strcpy(myhome_ip4,get_my_ip());
				printf("My IP has been retrieved to be: %s\n", myhome_ip4);
      			break;
    			case 3:
				if(int_myhome_port > 65535 || int_myhome_port < 0)
				{
					printf("\nInvalid port number... \n For now I'm TERMINATING!!\n\n");
					return 1;
				}
				printf("PORT NUMBER : %s\n", myhome_port);
        		break;
			case 4:
				printf("REGISTER");
				my_sd = register_client(ds[1],ds[2],myhome_port);
				printf("\nclient registered");
				FD_SET(my_sd, &c_master);
				FD_SET(my_sd, &c_read_fds); 	//keep track the biggest file descriptor
				if (my_sd > fdmax) {		// keep track of the max
					fdmax = my_sd;
				}
			break;
 			case 5:
				printf("CONNECT");
			break;
			case 6:
				printf("LIST");
			break;
			case 7:
				printf("TERMINATE");
			break;
			case 8:
				printf("EXIT");
			break;
			case 9:
				printf("DOWNLOAD");
			break;
			case 10:
				printf("\nCREATOR--->>");
				printf("\nStudent Name : Anuj Kaul");
				printf("\nPerson Number : 50098142");
			break;
			default:
				printf("\n Invaild Command");

			}

		   }else if (FD_ISSET(i, &c_read_fds)) { // we got one!!
			/*printf("Iminside the FD_ISSET");*/
			if (i == my_sd) {//get data from the server....
			   if ((recv(my_sd, buf, 1024, 0)) == -1) {
				perror("recv: error ");
				return 5;
			   }
			   strcat(buf,"\0");
			   printf("Data recieved : %s\n", buf);
			   fflush(stdout);
			//conn_count = atoi(buf[0]);
			//store_list(buf);
			}
			else if (i == sd_peer)
			{
			  /* addr_len = sizeof(peer_addrs);
			   sd_client = accept(sd_peer, (struct sockaddr *)&peer_addrs, &addr_len);
			   if (sd_client == -1) {
				//perror("server : accept error!!");
			   continue;
			   }
			   else{
				FD_SET(sd_client, &c_peers);	// add to client list
				FD_SET(sd_client, &c_master); // add to master set
				if (sd_client > fdmax) {// keep track of the max
					fdmax = sd_client;
				}
				inet_ntop(peer_addrs.ss_family,get_in_addr((struct sockaddr *)&peer_addrs),ip4_client,sizeof ip4_client);
				printf("\nserver: got connection from %s\n on socket :%d ", ip4_client,sd_client);
			   }*/
		 	}
			else{

			}
		    }
	       }

	}

	return 0;
}


void list_it()
{



}

/*void store_list(char buffer_val[1024]){

	int i=1;
	int j=-1;
	int k=0;
	char iplist[5][100];

	while(buffer_val[i]="\0"){
		if(buffer_val[i]=','){
		{
			j++;
			strcpy(iplist[j],"");
		}
		else{
	   		strcat(iplist[j],buffer_val[i]);
		}
		i++;
	}

}*/

const char* translist(char servlist[10][512],int cid){
	static char listbuf[1024];
	int i;
	strcpy(listbuf,"");
	for(i=0;i<cid;i++){
		strcat(listbuf,servlist[i]);
		strcat(listbuf,",");
	}
	return listbuf;
}

//do stuff with my peers
					/*if ((numbytes = recv(sd_client, client_request, 100, 0)) == -1) {
						perror("recvfrom");
						return 5;
					}
					else{

						printf("\nNumber of bytes : %d", numbytes);
						client_request[numbytes]='\0';
						printf("\nClient listening port is : %s", client_request);
						printf("\nName of the Client is : %s", hostname);
						sprintf(c, "%d", conn_id);
						strcat(c,"\0");
						strcpy(server_ip_list[conn_id],getlist(c,hostname,ip4_client,client_request));
						printf("\nConnection details are : %s", server_ip_list[conn_id]);
						conn_id++;
						for(j = 0; j <= fdmax; j++) {
									// send to everyone!
							if (FD_ISSET(j, &s_client)) {
							// except the listener and ourselves
								if (j != sd_server && j != 0) {
								strcpy(buf,"");
								strcat(buf,c);
								strcat(buf,",");
								strcat(buf,translist(server_ip_list,conn_id));
								if (send(j, buf, 1024, 0) == -1) {
									perror("send");
								}
							}
						}
					}*/
/***********************************************************************************************/
int commandtokenizer(char ui[20]){
	int index=0;
	printf("Command is to : %s", ui);
	if(strcmp(ui,"help") ==0 ||strcmp(ui,"HELP") ==0 ){
		return 1;
	}
	else if(strcmp(ui,"myip") ==0 ||strcmp(ui,"MYIP") ==0 ){
		return 2;
	}
	else if(strcmp(ui,"myport") ==0 ||strcmp(ui,"MYPORT") ==0){
		return 3;
	}
	else if(strcmp(ui,"register") ==0 ||strcmp(ui,"REGISTER") ==0 ){
		return 4;
	}
	else if(strcmp(ui,"connect") ==0 ||strcmp(ui,"CONNECT") ==0 ){
		return 5;
	}
	else if(strcmp(ui,"list") ==0 ||strcmp(ui,"LIST") ==0){
		return 6;
	}
	else if(strcmp(ui,"terminate") ==0 ||strcmp(ui,"TERMINATE") ==0){
		return 7;
	}
	else if(strcmp(ui,"exit") ==0 ||strcmp(ui,"EXIT") ==0){
		return 8;
	}
	else if(strcmp(ui,"download") ==0 ||strcmp(ui,"DOWNLOAD") ==0){
		return 9;
	}
	else if(strcmp(ui,"creator") ==0 ||strcmp(ui,"CREATOR") ==0 ){
		return 10;
	}
	return 0;
}

/**********************************************************************************************/
const char* getlist(char id[3],char name[50], char ip[16], char port[6]){

	static char retstr[100];
	strcpy(retstr,id);
	strcat(retstr,"|");
	strcat(retstr,name);
	strcat(retstr,"|");
	strcat(retstr,ip);
	strcat(retstr,"|");
	strcat(retstr,port);
return retstr;
}

/*************************************************************************************************/


void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*****************************************************************************************************/

int register_client(char serverip[INET_ADDRSTRLEN],char serverport[MAXPORTSIZE],char myport[MAXPORTSIZE]){

	struct addrinfo my_server, ai_client, *result, *rp;
	int my_sd,s;
	//my socket connecting to the server..........................................................
	memset(&my_server, 0, sizeof(my_server));
	my_server.ai_flags = AI_PASSIVE;
	my_server.ai_family = AF_INET;
	my_server.ai_socktype = SOCK_STREAM;
	my_server.ai_protocol = 0;

	if ((s = getaddrinfo(serverip, serverport, &my_server, &result)) != 0) {
 		perror(" client : getaddrinfo: \n");
		return 1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
               my_sd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

		if (my_sd == -1){
			perror("client : socket");
			 continue;
		}
		if (connect(my_sd, rp->ai_addr, rp->ai_addrlen) == -1) {
			//close(my_sd);
			perror("client: connect");
			continue;

		}
	break;

        }


	if (rp == NULL) {               /* No address succeeded */
		//close(sd_server);
		perror(" client : connect failed!!\n");
               	return 3;
        }

	freeaddrinfo(result);
	//========================================================================================


	if (send(my_sd, myport, MAXPORTSIZE, 0) == -1){
		perror("send : error");
		//close(my_sd);
		return 3;
	}

	return my_sd;
}


/*************************************************************************************************/



/*********************============= FUNCTION TO RETRIEVE MY IP ================*************************/

const char* get_my_ip()
{
	//For UDP to
	int udp_sd_myip;		//socket Descriptor to get my IP of type DGRAM
	static char my_ip[INET_ADDRSTRLEN];
	struct sockaddr_in udp_connect_addr, udp_myaddr;
	unsigned int mysock_addrlen;

	/*====================DECLARING AND DEDUCING ALL THE VALUES IN THE structs FOR MY MACHINE =========================*/
	memset(&udp_myaddr, 0, sizeof(udp_myaddr));
	udp_connect_addr.sin_family = AF_INET;				// IPv4 Family
	udp_connect_addr.sin_port = htons(53);    				// Port number in IANA for connecting to DNS
	udp_connect_addr.sin_addr.s_addr = inet_addr("8.8.8.8");     		// Google DNS IP

	udp_myaddr.sin_family = AF_INET;				// IPv4 Family

	/*******CREATING SOCKETS***********/

	udp_sd_myip = socket(AF_INET,SOCK_DGRAM,0);
	if(udp_sd_myip<0){
		perror("UDP Socket for Retrieving MY IP Failed :( !!!");
		return (char *)1;   			//??????????????CHECK THIS
	}
	/*********CONNECT TO GOOGLE DNS****************/
	if(connect(udp_sd_myip, (struct sockaddr *) &udp_connect_addr, sizeof(udp_connect_addr))<0)
	{
		close(udp_sd_myip);
		perror("Connect : to DNS Failed!!");
		return (char *)2;
	}
	else{
		/************** GETTING MY IP USING GETSOCK NAME***************/
		mysock_addrlen = sizeof(udp_myaddr);
		if(getsockname(udp_sd_myip, (struct sockaddr *) &udp_myaddr, &mysock_addrlen)<0)
		{
			close(udp_sd_myip);
			perror("Connect : to DNS Failed!!");
			return (char *)3;
		}
		else{
			/*************** COVERTING IP TO READABLE FORM *********************/
			inet_ntop(udp_myaddr.sin_family, &(udp_myaddr.sin_addr), my_ip, sizeof(my_ip));
		}
	}

	return my_ip;
}


/*********************============= END OF FUNCTION TO RETRIEVE MY IP ================*************************/

