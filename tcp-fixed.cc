#define NS_LOG_APPEND_CONTEXT \
if (m_node) { std::clog << Simulator::Now ().GetSeconds () << " [node " << m_node->GetId () << "] "    ; }

#include "tcp-fixed.h"
#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "ns3/node.h"
  
namespace ns3 {
  
NS_LOG_COMPONENT_DEFINE ("TcpFixed");
  
NS_OBJECT_ENSURE_REGISTERED (TcpNewReno);
 
TypeId
TcpFixed::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpFixed")
    .SetParent<TcpSocketBase> ()
    .SetGroupName ("Internet")
    .AddConstructor<TcpFixed> ()
    .AddAttribute ("ReTxThreshold", "Threshold for fast retransmit",
                    UintegerValue (3),
                    MakeUintegerAccessor (&TcpFixed::m_retxThresh),
                    MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("LimitedTransmit", "Enable limited transmit",
                   BooleanValue (false),
                   MakeBooleanAccessor (&TcpFixed::m_limitedTx),
                   MakeBooleanChecker ())
 ;
 return tid;
}
  
TcpFixed::TcpFixed (void)
: m_retxThresh (3), // mute valgrind, actual value set by the attribute system
    m_inFastRec (false),
    m_limitedTx (false) // mute valgrind, actual value set by the attribute system
{
  NS_LOG_FUNCTION (this);
}

TcpFixed::TcpFixed (const TcpFixed& sock)
  : TcpSocketBase (sock),
    m_retxThresh (sock.m_retxThresh),
    m_inFastRec (false),
    m_limitedTx (sock.m_limitedTx)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_LOGIC ("Invoked the copy constructor");
}
  
TcpFixed::~TcpFixed (void)
{
}

Ptr<TcpSocketBase>
TcpFixed::Fork (void)
{
  return CopyObject<TcpFixed> (this);
}
  
/* New ACK (up to seqnum seq) received. Increase cwnd and call TcpSocketBase::NewAck() */
void
TcpNewReno::NewAck (const SequenceNumber32& seq)
{
  NS_LOG_FUNCTION (this << seq);
  NS_LOG_LOGIC ("TcpNewReno received ACK for seq " << seq <<
                " cwnd " << m_cWnd <<
                " ssthresh " << m_ssThresh);

  // Check for exit condition of fast recovery
  if (m_inFastRec && seq < m_recover)
    { // Partial ACK, partial window deflation (RFC2582 sec.3 bullet #5 paragraph 3)
      //m_cWnd += m_segmentSize - (seq - m_txBuffer->HeadSequence ());
      m_txBuffer->DiscardUpTo(seq);  //Bug 1850:  retransmit before newack
      DoRetransmit (); // Assume the next seq is lost. Retransmit lost packet
      TcpSocketBase::NewAck (seq); // update m_nextTxSequence and send new data if allowed by window
      return;
    }
  else if (m_inFastRec && seq >= m_recover)
    { // Full ACK (RFC2582 sec.3 bullet #5 paragraph 2, option 1)
      //m_cWnd = std::min (m_ssThresh.Get (), BytesInFlight () + m_segmentSize);
      m_inFastRec = false;
    }

  // Increase of cwnd based on current phase (slow start or congestion avoidance)
  if (m_cWnd < m_ssThresh)
    { // Slow start mode, add one segSize to cWnd. Default m_ssThresh is 65535. (RFC2001, sec.1)
113       //m_cWnd += m_segmentSize;
115     }
116   else
117     { // Congestion avoidance mode, increase by (segSize*segSize)/cwnd. (RFC2581, sec.3.1)
118       // To increase cwnd for one segSize per RTT, it should be (ackBytes*segSize)/cwnd
119       double adder = static_cast<double> (m_segmentSize * m_segmentSize) / m_cWnd.Get ();
120       adder = std::max (1.0, adder);
121       //m_cWnd += static_cast<uint32_t> (adder);
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
137       //m_cWnd = m_ssThresh + 3 * m_segmentSize;
138       m_recover = m_highTxMark;
139       m_inFastRec = true;
142       DoRetransmit ();
143     }
144   else if (m_inFastRec)
145     { // Increase cwnd for every additional dupack (RFC2582, sec.3 bullet #3)
146       //m_cWnd += m_segmentSize;
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
178   //m_cWnd = m_segmentSize;
179   m_nextTxSequence = m_txBuffer->HeadSequence (); // Restart from highest Ack
182   DoRetransmit ();                          // Retransmit the packet
183 }
184 
185 } // namespace ns3
