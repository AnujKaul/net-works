#Please make necessary changes below in the value set provided to test
# ======================================================================
# =====================================================================
# Define transmission variable and node property options
# ======================================================================
set val(chan)         Channel/WirelessChannel  ;# channel type
set val(prop)         Propagation/TwoRayGround ;# radio-propagation model
set val(ant)          Antenna/OmniAntenna      ;# Antenna type
set val(ll)           LL                       ;# Link layer type
set val(ifq)          Queue/DropTail           ;# Interface queue type
set val(ifqlen)       100000                   ;# max packet in ifq
set val(netif)        Phy/WirelessPhy          ;# network interface type
set val(mac) 		  Mac/Anuj  			   ;# MAC type
set val(rp)           DumbAgent                ;# ad-hoc routing protocol 
set val(sn)           101       	           ;# number of mobilenodes
set val(x)	          50.0					   ;# topology width
set val(y)	          50.0					   ;#topology length
set val(simtime)      100.0					   ;#simulation time
set val(repeat)       7 					   ;#number of times to repeat the packet transmission

#**************************************************************************

#****************************GLOBAL VARIABLES*****************************

set ns_ [new Simulator]
set tracefd [open simulation.tr w]
$ns_ trace-all $tracefd

set namtrace [open single-hop.nam w]           ;# for nam tracing
$ns_ namtrace-all $namtrace			;# is the file handle

# set up topography object
set topo [new Topography]
$topo load_flatgrid $val(x) $val(y)


create-god $val(sn)

#**********BINDING THE Repeat times to a variable in our Protocol****************

$val(mac) set repeatTrans $val(repeat)

#*******************************************************************************#


#**************Configure the nodes ******************#
        $ns_ node-config -adhocRouting $val(rp) \
		-llType $val(ll) \
		-macType $val(mac) \
		-ifqType $val(ifq) \
		-ifqLen $val(ifqlen) \
		-antType $val(ant) \
		-propType $val(prop) \
		-phyType $val(netif) \
		-topoInstance $topo \
		-channelType $val(chan) \
		-agentTrace OFF \
		-routerTrace OFF \
		-macTrace ON \
		-movementTrace OFF \
		-mobileIP ON \
		-energyModel EnergyModel \
		-idlePower 0.035 \
		-rxPower 0 \
		-txPower 0.660 \
		-sleepPower 0.001 \
		-transitionPower 0.05 \
		-transitionTime 0.005 \
		-initialEnergy 1000
#*****************************************************#

#*****************************************************#
set rng [new RNG]
$rng seed 1


#
#Now to assign a value to each node position by placing it randoomlt
#

set randpos [new RandomVariable/Uniform]
$randpos use-rng $rng
$randpos set min_ -25.0
$randpos set max_ 25.0

#creating specified number of source nodes 
for {set i 0} {$i < $val(sn) } {incr i} {
	set node_($i) [$ns_ node ]
        $node_($i) random-motion 0       ;# disable random motion
	$node_($i) set X_ [expr 25 + [$randpos value]]
	$node_($i) set Y_ [expr 25 + [$randpos value]]
}  

set sink [new Agent/LossMonitor]		;#creating the sink node

#Attach a data-sink to destination

	$ns_ attach-agent $node_(0) $sink

##@
#creating specified number of source nodes 
for {set i 1} {$i < $val(sn) } {incr i} {
	$ns_ simplex-link $node_($i) $node_(0) 1Mb 2ms DropTail
}


##
# Binding the traffic type-> Constant Bit Rate(CBR) and ...make src talk to dst

for {set i 1} {$i < $val(sn)} {incr i} {	
    
    set proto_($i) [new Agent/UDP]
	$ns_ attach-agent $node_($i) $proto_($i)
	set cbr_($i) [new Application/Traffic/CBR]
	$cbr_($i) set packetSize_  128 					
	$cbr_($i) set interval_ 0.02					
	$cbr_($i) attach-agent $proto_($i)
	$ns_ connect $proto_($i) $sink
	$ns_ at 1.0 "$cbr_($i) start"
}

set stats [open statistic.dat w]

#Function to collect statistics on the losses in the traffic flow

proc update_stats {} {
	global sink
	global stats
	set ns_ [Simulator instance]	
	set bytes [$sink set bytes_]
	set now [$ns_ now]
	puts $stats "$now $bytes"

	$sink set bytes_ 0

	set time 1
	$ns_ at [expr $now+$time] "update_stats"
}


#
# Tell nodes when the simulation ends
#
#for {set i 0} {$i < $val(sn) } {incr i} {
#    $ns_ at $val(simtime) "$node_($i) reset";
#}
#$ns_ at $val(simtime) "stop"
#$ns_ at $val(simtime).01 "puts \"NS EXITING...\" ; $ns_ halt"


proc stop {} {
    global ns_ tracefd
    $ns_ flush-trace
    close $tracefd
    $ns_ halt
}

puts "Starting Simulation..."
$ns_ at 0.0 "update_stats"
$ns_ at $val(simtime) "stop"

#$ns_ halt

$ns_ run


