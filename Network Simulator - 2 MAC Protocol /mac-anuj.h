
/*
 *The following is a mac protocol modified according to the project guidelines and requirements
 *This credit for the base code goes to mac-simple.h which was the seed for my project changes. 
 *Author : Anuj Kaul 
 *UBID#  : 50098142
 */

#ifndef ns_mac_anuj_h
#define ns_mac_anuj_h

// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
#include "address.h"
#include "ip.h"

class MacAnujWaitTimer;
class MacAnujSendTimer;
class MacAnujRecvTimer;

// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
class EventTrace;


class MacAnuj : public Mac {
	//Added by Sushmita to support backoff
	friend class BackoffTimer;
public:
	MacAnuj();
	void recv(Packet *p, Handler *h);
	void send(Packet *p, Handler *h);

	void waitHandler(void);
	void sendHandler(void);
	void recvHandler(void);
	double txtime(Packet *p);

	// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
	void trace_event(char *, Packet *);
	int command(int, const char*const*);
	EventTrace *et_;
	
	
private:
	Packet *	pktRx_;
	Packet *	pktTx_;
    MacState        rx_state_;      // incoming state (MAC_RECV or MAC_IDLE)
	MacState        tx_state_;      // outgoing state
    int             tx_active_;
	int             fullduplex_mode_;
	Handler * 	txHandler_;
	MacAnujWaitTimer *waitTimer;
	MacAnujSendTimer *sendTimer;
	MacAnujRecvTimer *recvTimer;
	
	int busy_ ;
	
	/****************************************************************/
	/*****VARIABLES INTRODUCED TO HANDLE THE NEW PROTOCOL****/
	
	int 	repeat_count;		//keeps a count of current number of transmissions per packet
	double 	induced_jitter;		//the random induced jitter dependence !!
	double 	repeatTrans;		//to hold the vaue for X number of copies!!!
	
	
	/*------------------xxxxxxxxxxxxxxxxxx--------------------------*/
};

class MacAnujTimer: public Handler {
public:
	MacAnujTimer(MacAnuj* m) : mac(m) {
	  busy_ = 0;
	}
	virtual void handle(Event *e) = 0;
	virtual void restart(double time);
	virtual void start(double time);
	virtual void stop(void);
	inline int busy(void) { return busy_; }
	inline double expire(void) {
		return ((stime + rtime) - Scheduler::instance().clock());
	}

protected:
	MacAnuj	*mac;
	int		busy_;
	Event		intr;
	double		stime;
	double		rtime;
	double		slottime;
	
};

// Timer to use for delaying the sending of packets
class MacAnujWaitTimer: public MacAnujTimer {
public: MacAnujWaitTimer(MacAnuj *m) : MacAnujTimer(m) {}
	void handle(Event *e);
};

//  Timer to use for finishing sending of packets
class MacAnujSendTimer: public MacAnujTimer {
public:
	MacAnujSendTimer(MacAnuj *m) : MacAnujTimer(m) {}
	void handle(Event *e);
};

// Timer to use for finishing reception of packets
class MacAnujRecvTimer: public MacAnujTimer {
public:
	MacAnujRecvTimer(MacAnuj *m) : MacAnujTimer(m) {}
	void handle(Event *e);
};



#endif
