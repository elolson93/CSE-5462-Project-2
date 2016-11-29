/*
 * Eric Olson and James Baker
 * Project 2, CSE 5462
 * 12-6-2016
 *
 */
 
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/packet-sink.h"
#include <string>
 
using namespace ns3;
  
int main(int argc, char* argv[]) {
 
  //Create nodes A, B, C and D
  NodeContainer nodes;
  nodes.Create (4);
  
  //Create a point-to-point (wired) connection for our nodes
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("1Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("10ms"));

  //Install the connection between nodes. Save the network devices for later use.
  NetDeviceContainer devicesAB, devicesBC, devicesCD;
  devicesAB = pointToPoint.Install (nodes.Get (0), nodes.Get (1));
  devicesBC = pointToPoint.Install (nodes.Get (1), nodes.Get (2));
  devicesCD = pointToPoint.Install (nodes.Get (2), nodes.Get (3));
 
  //Create the bit error rates for the node connections
  //10^-6
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue (0.000001));
  //10^-5
  Ptr<RateErrorModel> emBC = CreateObject<RateErrorModel> ();
  emBC->SetAttribute ("ErrorRate", DoubleValue (0.00001));

  //Install the error rates on the receiving network device
  devicesAB.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
  devicesBC.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (emBC));
  devicesCD.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

  //Set up the internet protocol stack on our nodes
  InternetStackHelper stack;
  stack.Install (nodes);
 
  //Create network addresses for each connection and assign them to the network devices
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  address.Assign (devicesAB);
  address.SetBase ("10.1.2.0", "255.255.255.0");
  address.Assign (devicesBC);
  address.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devicesCD);
  
  //Set up our TCP sink application in node D
  uint16_t port = 8080;
  
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = packetSinkHelper.Install (nodes.Get (3));
  sinkApps.Start (Seconds (1.0));
  sinkApps.Stop (Seconds (120.0));
  
  //Set up our TCP sender application in node A
  BulkSendHelper source ("ns3::TcpSocketFactory", InetSocketAddress (interfaces.GetAddress (1), port));
  //This attribute value may not work
  source.SetAttribute ("MaxBytes", StringValue ("1MB"));
  ApplicationContainer sourceApps = source.Install (nodes.Get (0));
  sourceApps.Start (Seconds (2.0));
  sourceApps.Stop (Seconds (120.0));
 
  //Set up inter-network routing
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  //Run the simulator
  Simulator::Stop (Seconds (120.0));
  Simulator::Run ();
  Simulator::Destroy ();
 
}
