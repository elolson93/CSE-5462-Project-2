# CSE-5462-Project-2
Eric Olson and James Baker
Due December 6, 2016

###Install
*These instructions assume that ns3.24 is installed on your machine.*


##Part One: Plotting Transmitted Sequence Number

##Part Two: Plotting the Congestion Window

##Part Three: Creating an Alternate Path

####Description:
For this section of the project, in addition to the A-B-C-D topology of the previous sections, we have also included another 
point-to-point path A-E-F-G-D. A tcp sender and sink are installed on nodes A and D respectively, and at time equals two seconds,
the B-C link goes down. The routers must then adjust their routing tables to allow for the transmitted packets to take the longer 
A-E-F-G-D route. We trace and plot the sequence numbers of the packets received at node D.

####Compiling
In order to compile the third part of the project, first ensure that the file 'proj2-part3.cc' is located in the 'ns/scratch' directory.
Then, at the top level directory, run the command './waf'.

#####Testing
To run and test the third part of the project, after first compiling, type the command'./waf --run proj2-part3' from the top level 
directory. Several files will be produced including .pcap, .dat, .tr and .route files. The .route and .pcap files can be analyzed to
see that the routing tables are being adjusted after the B-C link goes down. 

To see the gnuplot diagram of the sequence numbers received at node D, 

##Part Four: 
####Description

####Compiling
In order to compile the fourth part of the project, first ensure that the files 'proj2-part4.cc' is located in the 'ns/scratch' directory.
Then, at the top level directory, run the command './waf'.

####Testing
