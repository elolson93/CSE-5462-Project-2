#define NS_LOG_APPEND_CONTEXT \
if (m_node) { std::clog << Simulator::Now ().GetSeconds () << " [node " << m_node->GetId () << "] "    ; }

 24 #include "tcp-fixed.h"
 25 #include "ns3/log.h"
 26 #include "ns3/trace-source-accessor.h"
 27 #include "ns3/simulator.h"
 28 #include "ns3/abort.h"
 29 #include "ns3/node.h"
 30 
 31 namespace ns3 {
 32 
 33 NS_LOG_COMPONENT_DEFINE ("TcpFixed");
 34 
 35 NS_OBJECT_ENSURE_REGISTERED (TcpNewReno);
 36 
 37 TypeId
 38 TcpFixed::GetTypeId (void)
 39 {
 40   static TypeId tid = TypeId ("ns3::TcpFixed")
 41     .SetParent<TcpSocketBase> ()
 42     .SetGroupName ("Internet")
 43     .AddConstructor<TcpNewReno> ()
 44     .AddAttribute ("ReTxThreshold", "Threshold for fast retransmit",
 45                     UintegerValue (3),
 46                     MakeUintegerAccessor (&TcpNewReno::m_retxThresh),
 47                     MakeUintegerChecker<uint32_t> ())
 48     .AddAttribute ("LimitedTransmit", "Enable limited transmit",
 49                    BooleanValue (false),
 50                    MakeBooleanAccessor (&TcpNewReno::m_limitedTx),
 51                    MakeBooleanChecker ())
 52  ;
 53   return tid;
 54 }
 55 
 56 TcpNewReno::TcpNewReno (void)
 : m_retxThresh (3), // mute valgrind, actual value set by the attribute system
 58     m_inFastRec (false),
 59     m_limitedTx (false) // mute valgrind, actual value set by the attribute system
 60 {
 61   NS_LOG_FUNCTION (this);
 62 }
 63 
 64 TcpNewReno::TcpNewReno (const TcpNewReno& sock)
 65   : TcpSocketBase (sock),
 66     m_retxThresh (sock.m_retxThresh),
 67     m_inFastRec (false),
 68     m_limitedTx (sock.m_limitedTx)
 69 {
 70   NS_LOG_FUNCTION (this);
 71   NS_LOG_LOGIC ("Invoked the copy constructor");
 72 }
 73 
 74 TcpNewReno::~TcpNewReno (void)
 75 {
 76 }
 77 
 78 Ptr<TcpSocketBase>
 79 TcpNewReno::Fork (void)
 80 {
 81   return CopyObject<TcpNewReno> (this);
 82 }
 83 
 84 /* New ACK (up to seqnum seq) received. Increase cwnd and call TcpSocketBase::NewAck() */
 85 void
 86 TcpNewReno::NewAck (const SequenceNumber32& seq)
 87 {
 88   NS_LOG_FUNCTION (this << seq);
 89   NS_LOG_LOGIC ("TcpNewReno received ACK for seq " << seq <<
 90                 " cwnd " << m_cWnd <<
 91                 " ssthresh " << m_ssThresh);
 92 
 93   // Check for exit condition of fast recovery
 94   if (m_inFastRec && seq < m_recover)
 95     { // Partial ACK, partial window deflation (RFC2582 sec.3 bullet #5 paragraph 3)
 96       m_cWnd += m_segmentSize - (seq - m_txBuffer->HeadSequence ());
 97       NS_LOG_INFO ("Partial ACK for seq " << seq << " in fast recovery: cwnd set to " << m_cWnd);
 98       m_txBuffer->DiscardUpTo(seq);  //Bug 1850:  retransmit before newack
 99       DoRetransmit (); // Assume the next seq is lost. Retransmit lost packet
100       TcpSocketBase::NewAck (seq); // update m_nextTxSequence and send new data if allowed by window
101       return;
102     }
103   else if (m_inFastRec && seq >= m_recover)
104     { // Full ACK (RFC2582 sec.3 bullet #5 paragraph 2, option 1)
105       m_cWnd = std::min (m_ssThresh.Get (), BytesInFlight () + m_segmentSize);
106       m_inFastRec = false;
107       NS_LOG_INFO ("Received full ACK for seq " << seq <<". Leaving fast recovery with cwnd set to "     << m_cWnd);
108     }
109 
110   // Increase of cwnd based on current phase (slow start or congestion avoidance)
111   if (m_cWnd < m_ssThresh)
    { // Slow start mode, add one segSize to cWnd. Default m_ssThresh is 65535. (RFC2001, sec.1)
113       m_cWnd += m_segmentSize;
114       NS_LOG_INFO ("In SlowStart, ACK of seq " << seq << "; update cwnd to " << m_cWnd << "; ssthresh     " << m_ssThresh);
115     }
116   else
117     { // Congestion avoidance mode, increase by (segSize*segSize)/cwnd. (RFC2581, sec.3.1)
118       // To increase cwnd for one segSize per RTT, it should be (ackBytes*segSize)/cwnd
119       double adder = static_cast<double> (m_segmentSize * m_segmentSize) / m_cWnd.Get ();
120       adder = std::max (1.0, adder);
121       m_cWnd += static_cast<uint32_t> (adder);
122       NS_LOG_INFO ("In CongAvoid, updated to cwnd " << m_cWnd << " ssthresh " << m_ssThresh);
123     }
124 
125   // Complete newAck processing
126   TcpSocketBase::NewAck (seq);
127 }
128 
129 /* Cut cwnd and enter fast recovery mode upon triple dupack */
130 void
131 TcpNewReno::DupAck (const TcpHeader& t, uint32_t count)
132 {
133   NS_LOG_FUNCTION (this << count);
134   if (count == m_retxThresh && !m_inFastRec)
135     { // triple duplicate ack triggers fast retransmit (RFC2582 sec.3 bullet #1)
136       m_ssThresh = std::max (2 * m_segmentSize, BytesInFlight () / 2);
137       m_cWnd = m_ssThresh + 3 * m_segmentSize;
138       m_recover = m_highTxMark;
139       m_inFastRec = true;
140       NS_LOG_INFO ("Triple dupack. Enter fast recovery mode. Reset cwnd to " << m_cWnd <<
141                    ", ssthresh to " << m_ssThresh << " at fast recovery seqnum " << m_recover);
142       DoRetransmit ();
143     }
144   else if (m_inFastRec)
145     { // Increase cwnd for every additional dupack (RFC2582, sec.3 bullet #3)
146       m_cWnd += m_segmentSize;
147       NS_LOG_INFO ("Dupack in fast recovery mode. Increase cwnd to " << m_cWnd);
148       if (!m_sendPendingDataEvent.IsRunning ())
149         {
150           SendPendingData (m_connected);
151         }
152     }
153   else if (!m_inFastRec && m_limitedTx && m_txBuffer->SizeFromSequence (m_nextTxSequence) > 0)
154     { // RFC3042 Limited transmit: Send a new packet for each duplicated ACK before fast retransmit
155       NS_LOG_INFO ("Limited transmit");
156       uint32_t sz = SendDataPacket (m_nextTxSequence, m_segmentSize, true);
157       m_nextTxSequence += sz;                    // Advance next tx sequence
158     };
159 }
160 
161 /* Retransmit timeout */
162 void
163 TcpNewReno::Retransmit (void)
{
165   NS_LOG_FUNCTION (this);
166   NS_LOG_LOGIC (this << " ReTxTimeout Expired at time " << Simulator::Now ().GetSeconds ());
167   m_inFastRec = false;
168 
169   // If erroneous timeout in closed/timed-wait state, just return
170   if (m_state == CLOSED || m_state == TIME_WAIT) return;
171   // If all data are received (non-closing socket and nothing to send), just return
172   if (m_state <= ESTABLISHED && m_txBuffer->HeadSequence () >= m_highTxMark) return;
173 
174   // According to RFC2581 sec.3.1, upon RTO, ssthresh is set to half of flight
175   // size and cwnd is set to 1*MSS, then the lost packet is retransmitted and
176   // TCP back to slow start
177   m_ssThresh = std::max (2 * m_segmentSize, BytesInFlight () / 2);
178   m_cWnd = m_segmentSize;
179   m_nextTxSequence = m_txBuffer->HeadSequence (); // Restart from highest Ack
180   NS_LOG_INFO ("RTO. Reset cwnd to " << m_cWnd <<
181                ", ssthresh to " << m_ssThresh << ", restart from seqnum " << m_nextTxSequence);
182   DoRetransmit ();                          // Retransmit the packet
183 }
184 
185 } // namespace ns3
