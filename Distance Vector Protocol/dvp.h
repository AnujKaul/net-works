#ifndef DVP_H
#define DVP_H

#define MAXNODES 6
#define MAXPORTSIZE 6

#define MAXNBORS MAXNODES-1

#define INFINITY 9999

#define NOHOP 0


//stores servers and neighbors details
struct server_details{
	int  sd_id;
	char sd_ip[INET_ADDRSTRLEN];
	char sd_port[MAXPORTSIZE];
	struct server_details *next;
};

//stores the initial costs specified in the topology file
struct server_costs{

	int sc_hid;
	int sc_nid;
	int sc_lcost;
	struct server_costs *next;
};

//Data structure to harness the topology file and its data...
struct server_topology{

	int 			t_num_servers;
	int 			t_num_neighbors;
	struct server_details 	*_head_serv_det;
	struct server_costs 	*_head_serv_cost;

	struct server_details   *_curr_serv_det;
	struct server_costs	*_curr_serv_cost;

};

//stores the recv vectors from neibors sets row by row

struct recv_vector_Entry{
	int sndr_nid;
	int sndr_ncost;
	char sndr_nip[INET_ADDRSTRLEN];
	char sndr_nport[MAXPORTSIZE];
};

//stores values recieved from neighbors

struct recvector{
	int sender_id;
	char sender_ip[INET_ADDRSTRLEN];
	char sender_port[MAXPORTSIZE];
	struct recv_vector_Entry recvEntry[MAXNODES];
};

//stores the distance vector record by record

struct distancevectorentry {
	int neigh_nodeID;		//destnation nodeID
	unsigned int cost;	//cost to the destination
};

//stores distance vector...

struct distancevector {
	int my_nodeID;		//source nodeID
	struct distancevectorentry dvEntry[MAXNODES];
};

//stores the routing record by record

struct routingtable_entry {
	int destNodeID;		//destination node ID
	int nextHopID;		//next node ID to which the packet should be forwarded
	int currCost;
};


//stores routing table

struct routingtable {
	int my_node;
	struct routingtable_entry rtEntry[MAXNODES];
};

#endif



