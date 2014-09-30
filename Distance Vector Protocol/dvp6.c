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
#include <sys/time.h>
#include "dvp.h"

/**************..........FUNCTION DECLARATION................*****************/

const char* get_my_ip();            // gets my ip addr to help me get my server ID
int get_topology_from_file();
void set_server_cost(char *buff);
void set_server_details(char *buff);
void set_topology_struct(char *line,int count);
void printstruct();

const char* get_ip_from_id(int node_id);
const char* get_port_from_id(int node_id);
int* get_neighbors();

int get_sockDesc(int id);
int create_fd_listening(char myport[MAXPORTSIZE]);
void *get_in_addr(struct sockaddr *sa);

int init_create_DV();
int init_create_RT();
void display_RT();
const char* create_msg();
void read_vector(char msg[1024]);
void display_DV();

int send_DV(char neighbor_ip[INET_ADDRSTRLEN],char neighbor_port[MAXPORTSIZE],char *buf);
void network_interface(int mode, int fd, char s_msg[1024]);
int recv_DV(int sockfd, char* buf[1024]);

void init_chkdis();
void bellman_ford();


/*******************.........GLOBAL VARIABLES !!! .........************************/


struct server_topology *st;
struct distancevector dv;
struct routingtable rt;
struct recvector rv;

fd_set udp_master;
fd_set udp_read;
int* neighbor_id;
int my_fd;
int packetcounter[MAXNODES];
int lastupdate[MAXNODES];
int disable_neigh[MAXNODES];
int num_of_packets;


/************.....................MAIN......................**********/

int main(int argc, char *argv[]){

	int i,n,p;
    	int my_fd,fdmax;
	int steptime,interval;
	int sel_val,cval;
	int init;
	char sendermsg[1024];
	char user_input[51];
	char ds[5][20];
	char *token;
    	int flags;
	int nbr_dis;
	int uhid;
        int unid;
        int ucost;
	struct timeval tv;
	flags = fcntl(0,F_GETFL,0);
	num_of_packets=0;
	if(fcntl(0,F_SETFL,flags | O_NONBLOCK))
	{
		printf("Control error");
	}
	interval = 30;
//initialize my servertopolgy structure
	st=malloc(sizeof(struct server_topology));
	st->_head_serv_det = NULL;
	st->_head_serv_cost = NULL;

	FD_ZERO(&udp_master);
	FD_ZERO(&udp_read);

//get topology!!!
	if(argc != 5){

       		 printf("\nThe execution string is not of the type : \"server -t <topology-file-name> -i <routing-update-interval>\" \n Exiting Program!!!\n");
       		 exit(0);
	}
	if(strcmp(argv[1],"server")){
        	printf("\nStart up field should be : \"server\" \n Exiting Program!!!");
        	exit(0);
    	}
    	if(!(strcmp(argv[2],"-t"))){
		if((get_topology_from_file(argv[3]))<0){
       			 printf("Error in Topology File!!! \nExiting Program!!!");
       			 exit(0);
		}        	

   	 }
    	if(!(strcmp(argv[4],"-i"))){
     	   	interval = atoi(argv[5]);
   	}

	FD_SET(0,&udp_master);

//get socket decriptors for UPD communication.
	my_fd = get_sockDesc(st->_head_serv_det->sd_id);
	//printf("MY FD IS : %d\n",my_fd);
	FD_SET(my_fd, &udp_master);


	fdmax = my_fd;
//non blocking socket
	flags = fcntl(my_fd,F_GETFL,0);
	if(fcntl(my_fd,F_SETFL,flags | O_NONBLOCK))
	{
		printf("Control error");
	}

/***Getting the list of my current neighbors ****/
	neighbor_id = get_neighbors();
/*
	for(n= 0; n < st->t_num_neighbors; n++){

		printf( "\nNeighbors is : %d", neighbor_id[n]);

	}
	fflush(stdout);
	*/

    init_chkdis();
	init_create_DV();

	init_create_RT();

	tv.tv_sec = 0;
        tv.tv_usec = 0;

	while(1){
		steptime = interval;
//printf("\n\nTimeout occourred!!!\n");
		strcpy(sendermsg,create_msg());

		network_interface(0,interval,sendermsg);
		struct timeval starttime;
		gettimeofday(&starttime,NULL);

		while(1)
		{
			udp_read = udp_master;
			struct timeval endtime;
            		gettimeofday(&endtime,NULL);
            		if(endtime.tv_sec - starttime.tv_sec >= steptime)
	            	{

	                	break;
	            	}
	                if (select(fdmax+1, &udp_read, NULL, NULL,&tv) == -1) {
		                perror("select : failed!!");
		                exit(0);
		        }
	        	for(i=0 ; i < fdmax+1; i++){
	                	if(FD_ISSET(i,&udp_read)){
	               		     if(i == 0){
					int j=read(0, user_input,sizeof(user_input));
					if(j>0)
					{
						user_input[j-1] = '\0';
						for(n=0;n<5;n++){
			                            strcpy(ds[n],"");
			                        }
						token = strtok(user_input," ");
						n=0;
						while ( token!= NULL)
	                        		{
					            if ( strlen(token) > 0 ){
					                strcpy(ds[n], token);
					                n++;
					            }
	                           		 token = strtok(NULL, " ");
	                        		}
	                        		token = NULL;

						cval = commandtokenizer(ds[0]);    // passing the command name

						switch(cval)
						{
						    case 1:
						        printf("\nim in help!!");
						    break;
						    case 2:
						    //printf("\nin update");
                                uhid = atoi(ds[1]);
                                unid = atoi(ds[2]);
                                ucost = 0;
                                if((strcmp(ds[3],"inf")==0)||(strcmp(ds[3],"INF")==0)){
                                        ucost = 9999;
                                    }
                                    else{
                                        ucost = atoi(ds[3]);
                                    }
                                if(dv.my_nodeID != uhid){
                                   printf("\n<%s %s %s %s> , Invalid Host Id!!!\n",user_input,ds[1],ds[2],ds[3]);
                                }
                                else if(!(isneighbor(unid))){
                                printf("\n<%s %s %s %s> , Invalid Neighbor Id!!!\n",user_input,ds[1],ds[2],ds[3]);
                                }
                                else if(chkcost(atoi(ds[3]))||(strcmp(ds[3],"inf")==0)||(strcmp(ds[3],"INF")==0)){
                                    if((strcmp(ds[3],"inf")==0)||(strcmp(ds[3],"INF")==0)){
                                        dv.dvEntry[unid].cost = 9999;
                                    }
                                    else{
                                        dv.dvEntry[unid].cost = atoi(ds[3]);
                                    }

			                        if(rt.rtEntry[unid].nextHopID == unid){
                                        rt.rtEntry[unid].currCost = ucost;
                                    }
                                    else{

                                        if(rt.rtEntry[unid].currCost > ucost){
                                            rt.rtEntry[unid].currCost = ucost;
                                            rt.rtEntry[unid].nextHopID = unid;
                                        }
                                    }
                                    printf("\n<%s %s %s %s> ... SUCCESS :D\n",user_input,ds[1],ds[2],ds[3]);
                                }
                                else{
                                    printf("\n<%s %s %s %s> ,Invalid Cost{ 0 - 9999(inf|INF) }for Updation!!!\n",user_input,ds[1],ds[2],ds[3]);
                               }
						    break;
						    case 3:
						    //printf("\nstep");
						          steptime = 0;
						     printf("\n<%s> ... SUCCESS :D\n",user_input);
						    break;
						    case 4:
                                printf("\nThe number of packets recieved since last <packets> check : %d\n", num_of_packets);
                                num_of_packets = 0;
						    break;
						    case 5:
							display_RT();
						    	printf("\n<%s> ... SUCCESS :D\n",user_input);
						    break;
						    case 6:
							
                                nbr_dis = atoi(ds[1]);
                                if(isneighbor(nbr_dis)){
                                    if(rt.rtEntry[neighbor_id[nbr_dis]].nextHopID == neighbor_id[nbr_dis]){
                                        rt.rtEntry[nbr_dis].currCost = 9999;
                                    }
                                    dv.dvEntry[nbr_dis].cost = 9999;
                                    printf("\n<%s %s> ... SUCCESS :D\n",user_input,ds[1]);
                                }
                                else{
                                    printf("\n<%s %s> ,Invalid Neighbor ID!!!\n",user_input,ds[1]);
                                }
						    break;
						    case 7:
                				printf("\n<%s> ... SUCCESS :D\n",user_input);
                                for( ; ; ){}
						    break;
						    default:
                                printf("\nInvaild Command!!!\n");
						    break;
                            //printf("\nterminal>");
						}
						fflush(stdout);
					}
				    }
				    else if(i == my_fd){
				        network_interface(1,my_fd,"\0");
				    }

                }
            }
        }
    }
}

/***initialize...disable***/

void init_chkdis(){
    struct timeval inittime;
    gettimeofday(&inittime,NULL);
    int i;
    for(i = 0; i< MAXNODES; i++){
        lastupdate[i]=inittime.tv_sec;
    }

}


void network_interface(int mode, int fd,char s_msg[1024]){

    struct timeval checktime;
    gettimeofday(&checktime,NULL);
	int n,numb;
	struct sockaddr_in nodeaddr;
	char node_ip[INET_ADDRSTRLEN];
	char node_port[MAXPORTSIZE];
	char recvermsg[1024];
	//s_msg[strlen(s_msg)]='\0';
	for(n= 0; n < st->t_num_neighbors; n++){

		if(mode ==0){
            if((checktime.tv_sec - lastupdate[neighbor_id[n]]) < 3*fd){
                printf( "\nSending to Neighbor : %d", neighbor_id[n]);
                strcpy(node_ip,get_ip_from_id(neighbor_id[n]));
                strcpy(node_port,get_port_from_id(neighbor_id[n]));
               // printf("\n*****************************************************");
               // printf("\nNeighbor IP : %s \t Port : %s \n Data : %s",node_ip,node_port,s_msg);
                send_DV(node_ip,node_port,s_msg);
            }
            else{
                if(rt.rtEntry[neighbor_id[n]].nextHopID == neighbor_id[n]){
                    //printf("DISABLING!!!");
                    rt.rtEntry[neighbor_id[n]].currCost = 9999;
                    dv.dvEntry[neighbor_id[n]].cost = 9999;
                    //printf("Modifying RT table");
                    //display_RT();
                }
            }
		}
		else if(mode == 1){
			numb = 0;
			numb = recv_DV(fd,&recvermsg);
			if(numb > 100){
                    //printf("Data size : %d \nData is: %s\n", numb, recvermsg);
				    read_vector(recvermsg);
                    num_of_packets++;
                    bellman_ford();
            }

		}
	}
}


/****************...............FUNTION TO RECIEVE DISTANCE VECTOR.................******************/


int recv_DV(int sockfd ,char *buf[1024]){

	int numbytes;
	struct sockaddr_storage their_addr;
	char neighbor_ip[INET_ADDRSTRLEN];
	char buffer[1024];
	socklen_t addr_len;
	memset(buffer,0,sizeof(buffer));
	addr_len = sizeof(their_addr);
	if ((numbytes = recvfrom(sockfd, buffer, sizeof(buffer) , 0,(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		return -1;
	}
	//printf("listener: got packet from %s\n",inet_ntop(their_addr.ss_family,	get_in_addr((struct sockaddr *)&their_addr),neighbor_ip, sizeof(neighbor_ip)));
	//printf("\nlistener: packet is %d bytes long\n", numbytes);
	buffer[numbytes] = '\0';
	if(numbytes > 0){
		strcpy(buf,buffer);
	}

	return numbytes;
}

/*************************************************************************************************/

/****************...............FUNTION TO SEND DISTANCE VECTOR.................******************/

int send_DV(char neighbor_ip[INET_ADDRSTRLEN],char neighbor_port[MAXPORTSIZE], char *buf){

	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int r,numbytes;
	//printf("\nSend DV got message to be sent as : %s",buf);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	if ((r = getaddrinfo(neighbor_ip, neighbor_port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(r));
		return 1;
	}
	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
		perror("neighbor: socket");
		continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "neighbor: failed to bind socket\n");
		return 2;
	}

	if ((numbytes = sendto(sockfd, buf, strlen(buf), 0,p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	freeaddrinfo(servinfo);
	//printf("talker: sent %d bytes to %s\n", numbytes, neighbor_ip);
	close(sockfd);
	return 0;

}

/*************************************************************************************************/

/************...........AUX NETWORK FUNTIONS..........*************/

int get_sockDesc(int id){

		char nip[INET_ADDRSTRLEN],nport[MAXPORTSIZE];
		int p;
		strcpy(nip,get_ip_from_id(id));
		strcpy(nport,get_port_from_id(id));
		p =create_fd_listening(nport);
		return p;
}


void *get_in_addr(struct sockaddr *sa){
if (sa->sa_family == AF_INET) {
return &(((struct sockaddr_in*)sa)->sin_addr);
}
return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/***********..........create the listening socket for host id............************/


int create_fd_listening(char myport[MAXPORTSIZE]){

	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int r;
	memset(&hints, 0, sizeof hints);
	int yes=1;

	hints.ai_family = AF_INET; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((r = getaddrinfo(NULL, myport, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(r));
		return 1;
	}
// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
		perror("listener: socket");
		continue;
		}

		setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}
		break;
	}
	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}
	freeaddrinfo(servinfo);

	return sockfd;
}




/*******************************************************/


/**     BELLMAN FORD !!
        we use dv which is to be updated after every recieve....
        we use the recv dv to check for updates to neighbors using the id of the sending server as the next hop
        *check for all updates if belman ford is run
        *after recieving the update on DV if any then update the the routing table too

        algois{


        }
****/
void bellman_ford(){

        int i,checkcost;
	        
	for( i = 1; i < MAXNODES ; i++){
		//printf("\n Updating FOR Ith entry = > %d",i);
            if( i == dv.my_nodeID){
                continue;
            }
            else if(rv.sender_id == i){
                continue;
            }
            else{
                checkcost = rv.recvEntry[i].sndr_ncost + dv.dvEntry[rv.sender_id].cost;     // USING THE RECIEVED VECTOR COST FOR CORRESPONDING ENTRY
                if(rt.rtEntry[i].nextHopID == rv.sender_id){                     //checking if next hop is present using the sender
                    rt.rtEntry[i].currCost = checkcost;
                    dv.dvEntry[i].cost = checkcost;         //for udpating the DV too
                }
                else if(checkcost < rt.rtEntry[i].currCost){            // else compare the total cost
                    rt.rtEntry[i].currCost = checkcost;
                    dv.dvEntry[i].cost = checkcost;         // for updating the DV too
                    rt.rtEntry[i].nextHopID = rv.sender_id;
                }
            }
	}
    	display_RT();
}





/**************************AUX FUNCTIONS***************************************/


int chkcost(int val){
    if(val >= 1 ){
        if(val <= 9999){
            return 1;
        }
        else{
            printf("\nCost can't be greater than infinity(9999)!!!\n");
            return 0;
        }
    }
    return 0;
}


int isneighbor(int id){
    printf("Checking for neighbors!! Recieved id : %d ", id);
    int n;
    for(n= 0; n < st->t_num_neighbors; n++){
        if(neighbor_id[n] == id){
            return 1;
        }
    }
    return 0;
}
/*******************************************************************/


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*******************MESSAGE FUNCTIONS*************************/

/**************************************************************************************/



void read_vector(char msg[1024]){

    struct timeval inittime;
    char checkrowbuf[100];
    char checkcolbuff[20];
    char rip[INET_ADDRSTRLEN],rport[MAXPORTSIZE];
    int param_count = 0;
    int field = 0;
    int index,i;
    char *token = strtok(msg,"|");
    char *token2;
    while(token != NULL){
		strcpy(checkrowbuf,token);
		field = 0;
		if ( param_count == 0   ){
           	 token2 = strtok(checkrowbuf,",");
			while(token2 != NULL){
                if(field == 0)
                {
			rv.sender_id = atoi(token2);
			rv.recvEntry[rv.sender_id].sndr_nid = rv.sender_id;
			rv.recvEntry[rv.sender_id].sndr_ncost = 0;
		}
                else if(field == 1)
                {
			strcpy(rv.sender_ip,token2);
	  	        strcpy(rv.recvEntry[rv.sender_id].sndr_nip,token2);
		}
                else if(field == 2)
		{
                	strcpy(rv.sender_port,token2);
        	        strcpy(rv.recvEntry[rv.sender_id].sndr_nport,token2);

		}
                	field++;
		        token2 = strtok(NULL, ",");
            }
        }
		else{
			token2 = strtok(checkrowbuf,",");
            index = atoi(token2);
            while(token2 != NULL){
			    if(field == 0)
        	       rv.recvEntry[index].sndr_nid = index;
        	    else if(field == 1)
        	        strcpy(rv.recvEntry[index].sndr_nip,token2);
        	    else if(field == 2)
        	        strcpy(rv.recvEntry[index].sndr_nport,token2);
        	    else if(field == 3)
        	       rv.recvEntry[index].sndr_ncost = atoi(token2);

	            field++;
	            token2 = strtok(NULL, ",");
            }
        }

		param_count++;
		token = strtok( token + strlen(token) + 1, "|");

    }


        gettimeofday(&inittime,NULL);
        lastupdate[rv.sender_id]=inittime.tv_sec;


        printf("\nRecieved updated distance vector from NODE ID : %d  <%s,%s>",rv.sender_id,rv.sender_ip,rv.sender_port);
        /*printf("\nNeighbor ID\t Neighbor IP\t Neighbor Port\t Neighbor Cost");
        for (i = 1 ;  i< MAXNODES ; i++){
             printf("\n%d\t\t%s\t%s\t\t%d",rv.recvEntry[i].sndr_nid,rv.recvEntry[i].sndr_nip,rv.recvEntry[i].sndr_nport,rv.recvEntry[i].sndr_ncost);
        }*/

    fflush(stdout);
}

/***
*THIS CREATES THE MESSAGE WHICH IS SENT TO THE NEIGHBORS
*RETURNS THE MESSAGE AS CONST CHAR
*GENERATES MESSAGE USING THE DISTANCE VECTOR
*
*/

const char* create_msg(){


	int u;
    static char msg[1024];
	char id[3];
	char node_ip[INET_ADDRSTRLEN];
	char node_port[MAXPORTSIZE];

	sprintf(msg, "%d",st->_head_serv_det->sd_id);
 	strcat(msg,",");
	strcat(msg, st->_head_serv_det->sd_ip);
	strcat(msg,",");
	strcat(msg, st->_head_serv_det->sd_port);
	strcat(msg,"|");

	for(u = 1; u < MAXNODES ; u++){
       	if(u == dv.my_nodeID){
                 continue;
       		 }else{

		    sprintf(id, "%d",dv.dvEntry[u].neigh_nodeID);
		    strcat(msg,id);
		    strcat(msg,",");

		    strcat(msg,get_ip_from_id(dv.dvEntry[u].neigh_nodeID));
		    strcat(msg,",");
		    strcat(msg,get_port_from_id(dv.dvEntry[u].neigh_nodeID));
		    strcat(msg,",");

		    sprintf(id, "%d",dv.dvEntry[u].cost);
		    strcat(msg,id);
		    strcat(msg,"|");
		}
	}
        //printf("\nMessage(%d) is :%s", strlen(msg), msg);


	return msg;
}

/******************************************************************************/



int init_create_RT(){

    int i;

    rt.my_node = st->_head_serv_det->sd_id;
    for (st->_curr_serv_det = st->_head_serv_det;  st->_curr_serv_det!=NULL ;st->_curr_serv_det = st->_curr_serv_det->next){
		rt.rtEntry[st->_curr_serv_det->sd_id].destNodeID = st->_curr_serv_det->sd_id;
		rt.rtEntry[st->_curr_serv_det->sd_id].currCost = INFINITY;
        rt.rtEntry[st->_curr_serv_det->sd_id].nextHopID = NOHOP;
	}

    for (i = 1 ; i < MAXNODES ; i++){
		for (st->_curr_serv_cost = st->_head_serv_cost;  st->_curr_serv_cost!=NULL ;st->_curr_serv_cost = st->_curr_serv_cost->next){
			if(i == rt.my_node){    //head will always be the server ID of MY Node!!!
				rt.rtEntry[i].nextHopID=NOHOP;
				rt.rtEntry[i].currCost = 0;
			}
			else if(st->_curr_serv_cost->sc_nid == i){
				rt.rtEntry[i].currCost = st->_curr_serv_cost->sc_lcost;
                rt.rtEntry[i].nextHopID = st->_curr_serv_cost->sc_nid;
			}
		}

	}

    display_RT();

return 0;
}

/**************...........FUNCTION TO DISPLAY THE ROUTING TABLE...........********************/

void display_RT(){

	int i;
	system("clear");
   	printf("\nRouting Table for Node (%d) -->>", rt.my_node);
	printf("\nDestination ID \t Next Hop \t Cost");
	for(i = 1; i < MAXNODES; i++){
		if(rt.rtEntry[i].currCost != INFINITY){
			printf("\n%d \t\t %d \t\t%d ",rt.rtEntry[i].destNodeID,rt.rtEntry[i].nextHopID,rt.rtEntry[i].currCost);
		}
		else{
			printf("\n%d \t\t NOHOP \t\tINF ",rt.rtEntry[i].destNodeID);
		}
	}
	printf("\nterminal>");
	fflush(stdout);

}

/*************************************************************************************/


void display_DV(){

	int i;
	printf("\nDistance Vector Table for Node (%d) -->>", dv.my_nodeID);
	printf("\nNeighbor ID \t Cost");
	for(i = 1; i < MAXNODES; i++){
		if(dv.dvEntry[i].cost != INFINITY){
			printf("\n%d \t\t %d ",dv.dvEntry[i].neigh_nodeID,dv.dvEntry[i].cost);
		}
		else{
			printf("\n%d \t\t INF ",dv.dvEntry[i].neigh_nodeID);
		}
	}
	fflush(stdout);
}

/****************........INITIALIZE DISTANCE VECTOR........********************/


int init_create_DV(){
	int i;

	dv.my_nodeID = st->_head_serv_det->sd_id;

	for (st->_curr_serv_det = st->_head_serv_det ;  st->_curr_serv_det!=NULL ;st->_curr_serv_det = st->_curr_serv_det->next){

        dv.dvEntry[st->_curr_serv_det->sd_id].neigh_nodeID = st->_curr_serv_det->sd_id;
		dv.dvEntry[st->_curr_serv_det->sd_id].cost = INFINITY;

    }

	for (i = 1 ; i < MAXNODES ; i++){
		for (st->_curr_serv_cost = st->_head_serv_cost;  st->_curr_serv_cost!=NULL ;st->_curr_serv_cost = st->_curr_serv_cost->next){
			if(i == st->_head_serv_det->sd_id){    //head will always be the server ID of MY Node!!!
				dv.dvEntry[i].cost = 0;
			}
			else if(st->_curr_serv_cost->sc_nid == i){
				dv.dvEntry[i].cost = st->_curr_serv_cost->sc_lcost;
			}
		}
	}

    display_DV();
	return 0;
}

/********************************************************************************************/


/************************************************************************************************/
/*****............................GETTER FUNCTIONS......................................*********/

/***********...........get neighbor from id // returns port number..............***********/

int* get_neighbors(){
	int i;
	static int n_id[MAXNBORS];
	for (st->_curr_serv_cost = st->_head_serv_cost , i=0;  st->_curr_serv_cost!=NULL ;st->_curr_serv_cost = st->_curr_serv_cost->next , i++)	{
			n_id[i]	= st->_curr_serv_cost->sc_nid;
	}
	return n_id;

}

/***********...........get ip from id // returns port number..............***********/

const char* get_ip_from_id(int node_id){

	static char nip[INET_ADDRSTRLEN];
	for (st->_curr_serv_det = st->_head_serv_det;  st->_curr_serv_det!=NULL ;st->_curr_serv_det = st->_curr_serv_det->next){
		if(st->_curr_serv_det->sd_id == node_id){
			strcpy(nip,st->_curr_serv_det->sd_ip);
			break;
		}
	}
	return nip;
}

/***********...........get port from id // returns port number..............***********/
const char* get_port_from_id(int node_id){

	static char nport[MAXPORTSIZE];
	for (st->_curr_serv_det = st->_head_serv_det;  st->_curr_serv_det!=NULL ;st->_curr_serv_det = st->_curr_serv_det->next){
		if(st->_curr_serv_det->sd_id == node_id){
			strcpy(nport,st->_curr_serv_det->sd_port);
			break;
		}
	}
	return nport;
}


/*XXXXXXXXXXXXXXXXXXXXXXXXX........End of getters..........XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/



/********************************************TOPOLOGY FILE RELATED FUNCTIONS*****************************************/
/***=============================================================================================================***/

void printstruct(){

 printf("\n/***************************************************************************/");
 printf("\nNumber of Servers: %d",st->t_num_servers);
 printf("\nNumber of Neighbors: %d",st->t_num_neighbors);

 for (st->_curr_serv_det = st->_head_serv_det;  st->_curr_serv_det!=NULL ;st->_curr_serv_det = st->_curr_serv_det->next){

		printf("\nServer ID : %d",st->_curr_serv_det->sd_id);
		printf("\nServer IP : %s",st->_curr_serv_det->sd_ip);
		printf("\nServer Port : %s",st->_curr_serv_det->sd_port);
	}

for (st->_curr_serv_cost = st->_head_serv_cost;  st->_curr_serv_cost!=NULL ;st->_curr_serv_cost = st->_curr_serv_cost->next){
        printf("\nHost ID : %d",st->_curr_serv_cost->sc_hid);
		printf("\nNeighbor ID : %d",st->_curr_serv_cost->sc_nid);
		printf("\nLink Cost : %d",st->_curr_serv_cost->sc_lcost);
	}


}


/******************........SET THE DETIALS OF ALL THE SERVERS PRESENT.........************************/

void set_server_details(char *buff){

		char checkbuf[100];
		int param_count = 0;
		char s_id[5];
		char s_ip[INET_ADDRSTRLEN],s_port[MAXPORTSIZE];
		strcpy(checkbuf,buff);
		char *token = strtok(checkbuf," ");
		while(token != NULL){
			if ( param_count == 0   ){
                strcpy(s_id,token);
            }
			if(param_count == 1){
			    strcpy(s_ip,token);
		        }
			if(param_count == 2){
			      strcpy(s_port,token);
            }

			param_count++;
			token = strtok(NULL, " ");

		}

		 /******SETTING VALUES TO MY STRUCTURES********/


		if(st->_head_serv_det == NULL){
			st->_head_serv_det=malloc(sizeof(struct server_details));
			st->_head_serv_det->sd_id = atoi(s_id);
			strcpy(st->_head_serv_det->sd_ip,s_ip);
			strcpy(st->_head_serv_det->sd_port,s_port);
			st->_head_serv_det->next = NULL;
			st->_curr_serv_det = st->_head_serv_det;
		}
		else{
			struct server_details *temp=malloc(sizeof(struct server_details));
			temp->sd_id = atoi(s_id);
			strcpy(temp->sd_ip,s_ip);
			strcpy(temp->sd_port,s_port);
			temp->next = NULL;
			st->_curr_serv_det->next = temp;
			st->_curr_serv_det = temp;

		}
}

/**************..SET THE COST TO NEGHBORING NODES INITIALLY FRO THE TOPOLOGY FILE...************************/

void set_server_cost(char *buff){

		char checkbuf[100];
		int param_count = 0;
		char s_hid[3];
		char s_nid[3],s_cost[5];
		strcpy(checkbuf,buff);
		char *token = strtok(checkbuf," ");
		while(token != NULL){
			if ( param_count == 0   ){
       	 		strcpy(s_hid,token);
            }
			if(param_count == 1){
			    strcpy(s_nid,token);
            }
			if(param_count == 2){
			      strcpy(s_cost,token);
			}

			param_count++;
			token = strtok(NULL, " ");

		}
        /******SETTING VALUES TO MY STRUCTURES********/

		if(st->_head_serv_cost == NULL){
			st->_head_serv_cost = malloc(sizeof(struct server_costs));
			st->_head_serv_cost->sc_hid = atoi(s_hid);
			st->_head_serv_cost->sc_nid = atoi(s_nid);
			st->_head_serv_cost->sc_lcost = atoi(s_cost);
			st->_head_serv_cost->next = NULL;
			st->_curr_serv_cost = st->_head_serv_cost;
		}
		else{
			struct server_costs *temp1=malloc(sizeof(struct server_costs));
			temp1->sc_hid = atoi(s_hid);
			temp1->sc_nid = atoi(s_nid);
			temp1->sc_lcost = atoi(s_cost);
			temp1->next = NULL;
			st->_curr_serv_cost->next = temp1;
			st->_curr_serv_cost = temp1;

		}


}


/********************************************************************************************/


/******************........SET THE SERVER TOPOLOGY STRUCTURE.........************************/
void set_topology_struct(char *line,int count){

		char *ch;
		ch = line;
		int i=0;
		if (count == 1){
		    while(*ch != '\0')
		    {
			st->t_num_servers = *ch - '0';           //  get number of total servers
		    	ch++;
			getchar();
		    }
		}
	 	else if (count == 2){

		    while(*ch != 0)
		    {
			st->t_num_neighbors = *ch - '0' ;       // get number of neighboring nodes
		    	ch++;
		    }
		}
		else if (count >= 3 && count <= 7){

			set_server_details(line);               // get details of all servers
		}
		else{

			set_server_cost(line);                  //get cost details for neighbor
		}
}

/********************************************************************************************/


/******************........GET DATA FROM THE FILE.........************************/

int get_topology_from_file(char *file){

	FILE *tfp;
	char myip[INET_ADDRSTRLEN];
	char *filename;                 //"/home/jarvis/Pro/" get filename
	filename = file;
	int count = 0;
	char *line = NULL;
	size_t len = 0;
	size_t read;

	if((tfp = fopen(filename,"r"))==NULL)           //file open
	{
	   perror(filename);
	   return -1;
	}
	while ((read = getline(&line, &len, tfp)) != -1) {      // file readline
           line[strlen(line)-1] = '\0';
	   set_topology_struct(line,++count);
	}

	strcpy(myip,get_my_ip());

	/************* arranging my strucutre for topology file refer my dvp.h file****************/
		if(strcmp(st->_head_serv_det->sd_ip, myip) != 0) {

			struct server_details *temp, *temp1;
			temp = st->_head_serv_det;
			temp1 = temp->next;

			while(strcmp(myip,temp1->sd_ip) != 0) {
            			temp = temp->next;
            			temp1 = temp1->next;
                if(temp1==NULL){

				break;
			}
			}
			if(temp1==NULL){
			printf("\nCould Not Find the NODE ID for the current Server's IP<%s>...\nPlease Enter the topology file again!!!\n",myip);
			return -1;
			}
			else{
				temp->next = temp1->next;
				temp1->next = st->_head_serv_det;
				st->_head_serv_det = temp1;
			}
		}
    return 1;
}


/***********************************************************************************************************/


/******
*MY IP to retrieve ip and assign Host id
*****************************************/

const char* get_my_ip(){
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
		//return 1;   			//??????????????CHECK THIS
	}
	/*********CONNECT TO GOOGLE DNS****************/
	if(connect(udp_sd_myip, (struct sockaddr *) &udp_connect_addr, sizeof(udp_connect_addr))<0)
	{
		close(udp_sd_myip);
		perror("Connect : to DNS Failed!!");
		//return 2;
	}
	else{
		/************** GETTING MY IP USING GETSOCK NAME***************/
		mysock_addrlen = sizeof(udp_myaddr);
		if(getsockname(udp_sd_myip, (struct sockaddr *) &udp_myaddr, &mysock_addrlen)<0)
		{
			close(udp_sd_myip);
			perror("Connect : to DNS Failed!!");
			//return 3;
		}
		else{
			/*************** COVERTING IP TO READABLE FORM *********************/
			inet_ntop(udp_myaddr.sin_family, &(udp_myaddr.sin_addr), my_ip, sizeof(my_ip));
		}
	}

	//printf("\nMy IP is  :%s",my_ip);
	//fflush(stdout);

	return my_ip;
}
/*************.............END OF MY IP FUNCTION..............******************/


/******
*  Command tokenizer for Switch case function
*  Returns integer value to switch case
*  Takes user input as parameter
***/


int commandtokenizer(char ui[20]){
	if(strcmp(ui,"help") ==0 ||strcmp(ui,"HELP") ==0 ){
		return 1;
	}
	else if(strcmp(ui,"update") ==0 ||strcmp(ui,"UPDATE") ==0 ){
		return 2;
	}
	else if(strcmp(ui,"step") ==0 ||strcmp(ui,"STEP") ==0){
		return 3;
	}
	else if(strcmp(ui,"packets") ==0 ||strcmp(ui,"PACKETS") ==0 ){
		return 4;
	}
	else if(strcmp(ui,"display") ==0 ||strcmp(ui,"DISPLAY") ==0 ){
		return 5;
	}
	else if(strcmp(ui,"disable") ==0 ||strcmp(ui,"DISPLAY") ==0){
		return 6;
	}
	else if(strcmp(ui,"crash") ==0 ||strcmp(ui,"CRASH") ==0){
		return 7;
	}
	else if(strcmp(ui,"exit") ==0 ||strcmp(ui,"EXIT") ==0){
		return 8;
	}
	else if(strcmp(ui,"creator") ==0 ||strcmp(ui,"CREATOR") ==0 ){
		return 10;
	}
	return 0;
}


/**************************END OF COMMAND TOKENIZER************************************/



