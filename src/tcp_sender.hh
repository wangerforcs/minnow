#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
class Timer
{
  uint64_t initial_RTO_ms_;
  uint64_t RTO_ms;
  uint64_t time_ms { 0 };
  bool is_timing { false };

public:
  Timer( uint64_t ms )
    : initial_RTO_ms_( ms )
    , RTO_ms( ms )
  {}
  void start()
  {
    time_ms = 0;
    is_timing = true;
  }
  void stop() { is_timing = false; }
  bool timing() { return is_timing; }
  bool expired()
  {
    return time_ms >= RTO_ms;
  }
  void tick( uint64_t ms_since_last_tick )
  {
    if ( is_timing ) {
      time_ms += ms_since_last_tick;
    }
  }
  void set_RTO() { RTO_ms = initial_RTO_ms_; }
  void double_RTO() { RTO_ms = 2 * RTO_ms; }
};

class TCPSender
{
private:
  Wrap32 isn_;
  bool SYN_sent { false };
  bool FIN_sent { false };
  uint64_t initial_RTO_ms_;
  /*
  ? why 0 wrong , 1 right
  first SYN may be back off retranmission, so not 0
  win_size = 0 we must send a 1 sized message
  */
  uint64_t win_size { 1 };
  uint64_t ackno { 0 };
  uint64_t sentno { 0 };
  uint64_t retransmissions_count { 0 };
  uint64_t outbytes_count { 0 };
  std::queue<TCPSenderMessage> out_standing {};
  std::queue<TCPSenderMessage> sending_queue {};
  Timer timer;

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
