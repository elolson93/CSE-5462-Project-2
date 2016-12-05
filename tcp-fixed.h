 #ifndef TCP_NEWRENO_H
 #define TCP_NEWRENO_H
  
 #include "tcp-socket-base.h"
 
 namespace ns3 {
  
 /**
 * \ingroup socket
 * \ingroup tcp
 *
 * \brief An implementation of a stream socket using TCP.
 *
 * This class contains the NewReno implementation of TCP, as of \RFC{2582}.
 */
 class TcpFixed : public TcpSocketBase
 {
 public:
 /**
 * \brief Get the type ID.
 * \return the object TypeId
 */
 static TypeId GetTypeId (void);
 /**
 * Create an unbound tcp socket.
 */
 TcpFixed (void);
 48   /**
 49    * \brief Copy constructor
 50    * \param sock the object to copy
 51    */
 52   TcpFixed (const TcpFixed& sock);
 53   virtual ~TcpFixed (void);
 54 
 55 protected:
 56   virtual Ptr<TcpSocketBase> Fork (void); // Call CopyObject<TcpNewReno> to clone me
 57   virtual void NewAck (SequenceNumber32 const& seq); // Inc cwnd and call NewAck() of parent
 virtual void DupAck (const TcpHeader& t, uint32_t count);  // Halving cwnd and reset nextTxSequence
 59   virtual void Retransmit (void); // Exit fast recovery upon retransmit timeout
 60 
 61 protected:
 62   SequenceNumber32       m_recover;      //!< Previous highest Tx seqnum for fast recovery
 63   uint32_t               m_retxThresh;   //!< Fast Retransmit threshold
 64   bool                   m_inFastRec;    //!< currently in fast recovery
 65   bool                   m_limitedTx;    //!< perform limited transmit
 66 };
 67 
 68 } // namespace ns3
 69 
 70 #endif /* TCP_NEWRENO_H */
