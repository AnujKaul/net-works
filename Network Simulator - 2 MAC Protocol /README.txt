1) Copy the .cc file and the .h file to the mac folder of NS2 and add the corresponding named .o in MakeFile.

2) Copy the nsmac.tcl & anal.awk file to the root NS2 folder

3) To change the number of nodes for the simulation please open the tcl file and update the variable 'sn' 
to the desired value.Currently nodes is set to 101
(Remember the nodes = number of nodes + sink node)

4) To set the number of copies of packets to be transmitted set the variable 'repeat' to the desired value.
Currently the value is set to 10.

5) To change the simulation time please change the variable simtime in the tcl.

6) to run NS2 simulation call command 

ns nsmac.tcl

7) After the simulation a trace file with name simulation.tr is generated in NS2 root folder. 
To analyse the total number of packets sent and the total number of packets recieved run the following command in linux
terminal.

awk -f anal.awk -F " " simulation.tr
