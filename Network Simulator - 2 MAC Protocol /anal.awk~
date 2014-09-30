#assigning counters for various packets
BEGIN {
  send=0;
  recv=0;
  drop=0;
  ctrl_pkt=0;
}

#Body containing the analysis script
{
  #total number of Dropped Packets at node 0
  if ($1=="D" && $3=="_0_")
  {
    ctrl_pkt++;
  }
  #total no of MAC cbr packets sent. This should be lower than (simulation time)*(no of node)*50*(no of repeat packets)
  #as it contains a lot of ARP exchanges(which is not included) in the initial part of the program, 
  #but it should be a justifiably close value to expected.
  if ( $1=="s" && $4=="MAC" && $7=="cbr" )
  {
    send++;
  }
  #no of MAC cbr packets successfully recieved at node 0
  if ($1=="r" && $4=="MAC" && $7=="cbr" && $3=="_0_")
  {
    recv++;
  } 
  #no of MAC cbr packets dropped at 0
  if ($1=="D" && $4=="MAC" && $7=="cbr" && $3=="_0_")
  {
    drop++;
  } 
  
  
}

END {
  #defines the probability of packets recieved of the type MAC cbr using the generated values.
  prob = (recv/send);
  printf ("Send:%d\n",send);
  printf ("Recv:%d\n",recv);
  printf ("Drop:%d\n",drop);
  printf ("Ctrl:%d\n",ctrl_pkt);
  printf ("Probability:%0.4f\n", prob);
  
}
