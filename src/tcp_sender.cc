#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) )
  , initial_RTO_ms_( initial_RTO_ms )
  , timer( initial_RTO_ms )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return outbytes_count;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return retransmissions_count;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  if ( sending_queue.empty() ) {
    return std::nullopt;
  }
  if ( !timer.timing() ) {
    timer.start();
  }
  TCPSenderMessage tosend = sending_queue.front();
  sending_queue.pop();
  return tosend;
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  // using temp var to ensure no back off rto in tick
  uint64_t cur_win_size = ( win_size > 0 ? win_size : 1 );
  while ( outbytes_count < cur_win_size ) {
    /*
    why inside the loop? wrong outside(cannt fill window, in fact like a random size)
    message contains Buffer, Buffer contans a shared_ptr<string> ,we must push each ptr(different) to
    string(different), rather than put the same ptr to only one thing(may not same as changed)
    */
    TCPSenderMessage tosend;
    if ( !SYN_sent ) {
      SYN_sent = true;
      tosend.SYN = true;
      outbytes_count++;
    }
    tosend.seqno = Wrap32::wrap( sentno, isn_ );
    read( outbound_stream, min( cur_win_size - outbytes_count, TCPConfig::MAX_PAYLOAD_SIZE ), tosend.payload );
    outbytes_count += tosend.payload.size();

    if ( !FIN_sent && outbound_stream.is_finished() && outbytes_count < cur_win_size ) {
      FIN_sent = true;
      tosend.FIN = true;
      outbytes_count++;
    }

    if ( !tosend.sequence_length() ) {
      break;
    }

    sentno += tosend.sequence_length();
    sending_queue.push( tosend );
    out_standing.push( tosend );

    // !
    if ( FIN_sent || outbound_stream.bytes_buffered() == 0 ) {
      break;
    }
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  return TCPSenderMessage { Wrap32::wrap( sentno, isn_ ), false, {}, false };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  win_size = msg.window_size;
  if ( msg.ackno.has_value() ) {
    auto ackno_recv = msg.ackno.value().unwrap( isn_, sentno );
    if ( ackno_recv > sentno ) {
      return;
    }
    if ( ackno_recv > ackno ) {
      ackno = ackno_recv;
      while ( !out_standing.empty() ) {
        TCPSenderMessage& first_msg = out_standing.front();
        // only having been acked, the message can be removed from out_standing
        if ( first_msg.seqno.unwrap( isn_, sentno ) + first_msg.sequence_length() <= ackno ) {
          outbytes_count -= first_msg.sequence_length();
          out_standing.pop();
        } else {
          break;
        }
      }
      timer.set_RTO();
      retransmissions_count = 0;
      if ( out_standing.empty() ) 
        timer.stop();
      else 
        timer.start();
    }
  }
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  timer.tick( ms_since_last_tick );
  if ( timer.expired() ) {
    sending_queue.push( out_standing.front() );
    if ( win_size != 0 ) {
      retransmissions_count++;
      timer.double_RTO();
    }
    timer.start();
  }
}
