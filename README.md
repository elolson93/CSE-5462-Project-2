# CSE-5462-Project-2
Eric Olson and James Baker
Due December 6, 2016

###Install
*These instructions assume that ns3.24 is installed on your machine.*


##Part One: Plotting Transmitted Sequence Number

####Description
This section of the project required us to trace the transmitted sequence numbers of packets from node A to node D. Our nodes used the
TcpNewReno class. We then plotted our data, displaying the packet retransmissions in a different color.

####Compiling

####Testing

##Part Two: Plotting the Congestion Window

####Description

####Compiling

####Testing

##Part Three: Creating an Alternate Path

####Description:
For this section of the project, in addition to the A-B-C-D topology of the previous sections, we have also included another 
point-to-point path A-E-F-G-D. A tcp sender and sink are installed on nodes A and D respectively, and at time equals two seconds,
the B-C link goes down. The routers must then adjust their routing tables to allow for the transmitted packets to take the longer 
A-E-F-G-D route. We trace and plot the sequence numbers of the packets received at node D.

####Compiling
In order to compile the third part of the project, first ensure that the file `proj2-part3.cc` is located in the `ns/scratch` directory.
Then, at the top level directory, run the command `./waf`.

####Testing
To run and test the third part of the project, after first compiling, type the command `./waf --run proj2-part3` from the top level 
directory. Several files will be produced including .pcap, .dat, .tr and .route files. The .route and .pcap files can be analyzed to
see that the routing tables are being adjusted after the B-C link goes down. 

To see the gnuplot diagram of the sequence numbers received at node D, 

##Part Four: Extending ns-3

####Description
For this section of the project, we have extended ns-3 to include a custom made class called TcpFixed based off of the TcpNewReno class.
Our new class, TcpFixed, behaves much the same as TcpNewReno except that it's congestion window has been hardcoded to always be
the size 100 * (1 MSS) where 1 MSS is equal to 536 bytes. We then trace, plot and compare both the congestion window and the transmitted
packet sequence number using both TcpFixed and TcpNewReno.

####Compiling
In order to compile the fourth part of the project, first make sure that the files `tcp-fixed.cc` and `tcp-fixed.h` are located in the
`ns/src/internet/model` directory. Both `tcp-fixed.cc` and `tcp-fixed.h` must also be added to their respective lists based on file type
in the file `ns/src/internet/wscript`. In order to test that the above actions were completed successfully, at the top level directory
type the command `./test.py`. Next, ensure that the files `proj2-part4-cwnd.cc`, `proj2-part4-cwnd-reno.cc`,
`proj2-part4-seq.cc` and `proj2-part4-seq-reno.cc` are all located in the `ns/scratch` directory. Then, at the top level directory,
run the command `./waf`.

####Testing
Testing the fourth part of this project is simple but a little repetitive. After completing the compiling steps above, go the the top-
level directory and type the following commands one after the other: `./waf --run proj2-part4-cwnd`, `./waf --run proj2-part4-cwnd-reno`,
 `./waf --run proj2-part4-seq` and `./waf --run proj2-part4-seq-reno`. These commands will have produced the necessary output files needed
 to plot the data.
 
 To see the gnuplot diagrams, 
