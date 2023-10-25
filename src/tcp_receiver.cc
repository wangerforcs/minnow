#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  if ( message.SYN ) {
    isn = message.seqno;
  }
  if ( !isn.has_value() )
    return;
  reassembler.insert( message.seqno.unwrap( isn.value(), inbound_stream.bytes_pushed() ) + message.SYN - 1,
                      (std::string)message.payload,
                      message.FIN,
                      inbound_stream ); 
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  TCPReceiverMessage res;
  if ( isn.has_value() )
    res.ackno = Wrap32::wrap( inbound_stream.bytes_pushed(), isn.value() ) + 1 + inbound_stream.is_closed();
  res.window_size = (uint16_t)min( inbound_stream.available_capacity(), 65535ul );
  return res;
}
