/*
 *The following is a mac protocol modified according to the project guidelines and requirements.
 *This credit for the base code goes to mac-simple.cc, zilong and ram who provided the seed for my project  
 *changes. 
 *Author : Anuj Kaul 
 *UBID#  : 50098142
 */


#include "ll.h"
#include "mac.h"

/********Header for the project req**********/
#include "mac-anuj.h"

#include "random.h"
#include "agent.h"
#include "basetrace.h"

#include "cmu-trace.h"

static class MacAnujClass : public TclClass {
public:
	MacAnujClass() : TclClass("Mac/Anuj") {}
	TclObject* create(int, const char*const*) {
		return new MacAnuj();
	}
} class_MacAnuj;


void MacAnuj::trace_event(char *eventtype, Packet *p)
{
	if (et_ == NULL) return;
	char *wrk = et_->buffer();
	char *nwrk = et_->nbuffer();

	hdr_ip *iph = hdr_ip::access(p);
	char *src_nodeaddr =
		Address::instance().print_nodeaddr(iph->saddr());
	char *dst_nodeaddr =
		Address::instance().print_nodeaddr(iph->daddr());

	if (wrk != 0) 
	{
		sprintf(wrk, "E -t "TIME_FORMAT" %s %s %s",
			et_->round(Scheduler::instance().clock()),
			eventtype,
			src_nodeaddr,
			dst_nodeaddr);
	}
	if (nwrk != 0)
	{
		sprintf(nwrk, "E -t "TIME_FORMAT" %s %s %s",
		et_->round(Scheduler::instance().clock()),
		eventtype,
		src_nodeaddr,
		dst_nodeaddr);
	}
	et_->dump();
}

MacAnuj::MacAnuj() : Mac() {
	
	/*
	*
	*calling the instance of TCL CLass to bind a new variable from the tcl file to our class file
	*/
	Tcl& tcl = Tcl::instance();				
	
	rx_state_ = tx_state_ = MAC_IDLE;
	tx_active_ = 0;
	waitTimer = new MacAnujWaitTimer(this);
	sendTimer = new MacAnujSendTimer(this);
	recvTimer = new MacAnujRecvTimer(this);
	
	et_ = new EventTrace();
	busy_ = 0;
	//bind("fullduplex_mode_", &fullduplex_mode_);
	
	/****TO BIND THE VALUE OF NUMBER OF COPIES THAT NEEDS TO BE TRANSMITTED*******/
	tcl.evalf("Mac/Anuj set repeatTrans");
	bind("repeatTrans", &repeatTrans);
	/*---------XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX----------*/
}


int MacAnuj::command(int argc, const char*const* argv)
{
	if (argc == 3) {
		if(strcmp(argv[1], "eventtrace") == 0) {
			et_ = (EventTrace *)TclObject::lookup(argv[2]);
			return (TCL_OK);
		}
	}
	return Mac::command(argc, argv);
}

void MacAnuj::recv(Packet *p, Handler *h) {

	struct hdr_cmn *hdr = HDR_CMN(p);
	/* let MacAnuj::send handle the outgoing packets */
	if (hdr->direction() == hdr_cmn::DOWN) {
		send(p,h);
		//send(p,h);
		return;
	}
	else if(hdr->direction() == hdr_cmn::UP){
		/* handle an incoming packet */

		/*
		 * If we are transmitting i.e. the direction is going up, then set the error bit in the packet
		 * so that it will be thrown away
		 */
	
		// in full duplex mode it can recv and send at the same time
		if (tx_active_)
		{
			hdr->error() = 1;

		}

		/*
		 * check to see if we're already receiving a different packet
		 */
	
		if (rx_state_ == MAC_IDLE) {
			/*
			 * We aren't already receiving any packets, so go ahead
			 * and try to receive this one.
			 */
			rx_state_ = MAC_RECV;
			pktRx_ = p;
			/* schedule reception of the packet */
			recvTimer->start(txtime(p));
		} else {
			/*
			 * We are receiving a different packet, so decide whether
			 * the new packet's power is high enough to notice it.
			 */
			if (pktRx_->txinfo_.RxPr / p->txinfo_.RxPr
				>= p->txinfo_.CPThresh) {
				/* power too low, ignore the packet */
				Packet::free(p);
			} else {
				/* power is high enough to result in collision */
				rx_state_ = MAC_COLL;

				/*
				 * look at the length of each packet and update the
				 * timer if necessary
				 */

				if (txtime(p) > recvTimer->expire()) {
					recvTimer->stop();
					Packet::free(pktRx_);
					pktRx_ = p;
					recvTimer->start(txtime(pktRx_));
				} else {
					Packet::free(p);
				}
			}
		}
	}
}


void MacAnuj::recvHandler()
{
	hdr_cmn *ch = HDR_CMN(pktRx_);
	Packet* p = pktRx_;
	MacState state = rx_state_;
	pktRx_ = 0;
	int dst = hdr_dst((char*)HDR_MAC(p));
	
	//busy_ = 0;

	rx_state_ = MAC_IDLE;

	// in full duplex mode we can send and recv at the same time
	// as different chanels are used for tx and rx'ing
	if (tx_active_) {
		// we are currently sending, so discard packet
		Packet::free(p);
	} else if (state == MAC_COLL) {
		// recv collision, so discard the packet
		drop(p, DROP_MAC_COLLISION);
		//Packet::free(p);
	} else if (dst != index_ && (u_int32_t)dst != MAC_BROADCAST) {
		
		/*  address filtering
		 *  We don't want to log this event, so we just free
		 *  the packet instead of calling the drop routine.
		 */
		Packet::free(p);
	//} //else if (ch->error()) {
		// packet has errors, so discard it
		//Packet::free(p);
		//drop(p, DROP_MAC_PACKET_ERROR);
	
	} else {
		uptarget_->recv(p, (Handler*) 0);
	}
}


double
MacAnuj::txtime(Packet *p)
 {
	 struct hdr_cmn *ch = HDR_CMN(p);
	 double t = ch->txtime();
	 if (t < 0.0)
	 	t = 0.0;
	 return t;
 }



void MacAnuj::send(Packet *p, Handler *h)
{
	hdr_cmn* ch = HDR_CMN(p);

	/* store data tx time */
 	ch->txtime() = Mac::txtime(ch->size());

	// Added by Sushmita to support event tracing (singal@nunki.usc.edu)
	trace_event("SENSING_CARRIER",p);

	/* check whether we're idle */
	if (tx_state_ != MAC_IDLE) {
		// already transmitting another packet .. drop this one
		// Note that this normally won't happen due to the queue
		// between the LL and the MAC .. the queue won't send us
		// another packet until we call its handler in sendHandler()

		Packet::free(p);
		return;
	}

	pktTx_ = p;
	txHandler_ = h;
	
	
	// rather than sending packets out immediately, add in some
	// jitter to reduce chance of unnecessary collisions
	double jitter = Random::random()%40 * 100/bandwidth_;

	if(rx_state_ != MAC_IDLE) {
		trace_event("BACKING_OFF",p);
	}

	if (rx_state_ == MAC_IDLE ) {
		repeat_count = 1;
		double pktTxTime = ch->txtime();
		// i divide the packet handling time  by no of times i wish to repeat the Transmission and create my 			temporary jitter.
		induced_jitter = pktTxTime / repeatTrans; 
		// we're idle, so start sending now
		// here an very small quantity of jitter is added calculated locally to avoid all nodes from 			// transmitting at the same time and colliding everytime. lesson learnt from mac-simple.cc!!!
		waitTimer->restart(jitter + induced_jitter);
		sendTimer->restart(jitter + induced_jitter + ch->txtime());
		
	} else {
		repeat_count = 1;
		double pktTxTime = ch->txtime();
		// i divide the packet handling time  by no of times i wish to repeat the Transmission and create my 			temporary jitter.
		induced_jitter = pktTxTime / repeatTrans; 
		// we're idle, so start sending now
		// here an very small quantity of jitter is added calculated locally to avoid all nodes from 			// transmitting at the same time and colliding everytime. lesson learnt from mac-simple.cc!!!
		waitTimer->restart(jitter + induced_jitter);
		sendTimer->restart(jitter + induced_jitter + ch->txtime()
				 + HDR_CMN(pktRx_)->txtime());
	}
}


void MacAnuj::waitHandler()
{

	/**This notifies the handler to handle the packet and prepare it for transmission
	  *	we maintain a copy of the packet, otherwise it gives an invalid UID error of the packet
	  *	
	  **/
	tx_state_ = MAC_SEND;
	tx_active_ = 1;
	Packet *pktcpy = pktTx_->copy();

	downtarget_->recv(pktTx_, txHandler_);

	if(repeat_count<repeatTrans)
	{
		pktTx_ = pktcpy;
	}
	else{
		Packet::free(pktcpy);
	}
}

void MacAnuj::sendHandler()
{
	Handler *h = txHandler_;
	Packet *p = pktTx_;
	Packet *pktcpy = pktTx_->copy();
	
	//using jitter for transmitting subsequent packets and avoiding unnecessary collisions......
	double jitter = Random::random()%40 * 100/bandwidth_;

	//setting variables and state....
	pktTx_ = 0;
	txHandler_ = 0;
	tx_state_ = MAC_IDLE;
	tx_active_ = 0;

	//busy_ = 1;
	//busy_ = 0;
	
	
	/*
	*	handling the packets for x-1 subsequent transmission .
	*/
	if(repeat_count < repeatTrans)
	{
		hdr_cmn* ch_temp = HDR_CMN(pktcpy);
		pktTx_ = pktcpy;
		txHandler_ = h;
		induced_jitter = ((ch_temp->txtime()));
		waitTimer->restart(jitter + induced_jitter);
		sendTimer->restart(jitter + induced_jitter + ch_temp->txtime());
		repeat_count++;
	}
	else{
	/*
	*	When im done handling the packets for x-1 subsequent trans mission notify the layer above me.
	* 	To send the second packet for transmission after 0.02secs :)
	*/
		// I have to let the guy above me know I'm done with the packet
		h->handle(p);
		Packet::free(pktcpy);
	}
	
	
}




//  Timers

void MacAnujTimer::restart(double time)
{
	if (busy_)
		stop();
	start(time);
}

	

void MacAnujTimer::start(double time)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_ == 0);
	
	busy_ = 1;
	stime = s.clock();
	rtime = time;
	assert(rtime >= 0.0);

	s.schedule(this, &intr, rtime);
}

void MacAnujTimer::stop(void)
{
	Scheduler &s = Scheduler::instance();

	assert(busy_);
	s.cancel(&intr);
	
	busy_ = 0;
	stime = rtime = 0.0;
}


void MacAnujWaitTimer::handle(Event *e)
{
	busy_ = 0;
	stime = rtime = 0.0;

	mac->waitHandler();
}

void MacAnujSendTimer::handle(Event *e)
{
	busy_ = 0;
	stime = rtime = 0.0;

	mac->sendHandler();
}

void MacAnujRecvTimer::handle(Event *e)
{
	busy_ = 0;
	stime = rtime = 0.0;

	mac->recvHandler();
		
}


