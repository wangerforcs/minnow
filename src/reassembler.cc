#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // Your code here.
  if ( is_last_substring ) {
    coming_last = true;
  }
  // directly write to output
  if ( first_index <= first_unwritten ) {
    if ( first_index + data.size() > first_unwritten ) {
      uint64_t len = min( data.size() + first_index - first_unwritten, output.available_capacity() );
      if ( len != 0 ) {
        output.push( data.substr( first_unwritten - first_index, len ) );
        bytes_written += len;
        first_unwritten += len;
      }
    }
  }
  // store in buffer
  else {
    if ( output.available_capacity() + first_unwritten > first_index ) {
      uint64_t len = output.available_capacity() + first_unwritten - first_index;
      buffer.insert( datagram( first_index, data.substr( 0, len ) ) );
    }
  }
  // write to output from buffer
  for ( auto it = buffer.begin(); it != buffer.end(); ) {
    if ( it->first_index > first_unwritten )
      break;
    if ( it->first_index + it->data.size() > first_unwritten ) {
      uint64_t len = it->data.size() + it->first_index - first_unwritten;
      output.push( it->data.substr( first_unwritten - it->first_index) );
      bytes_written += len;
      first_unwritten += len;
    }
    it = buffer.erase( it );
  }
  if ( buffer.empty() && coming_last )
    output.close();
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  if ( buffer.empty() ) {
    return 0;
  }
  uint64_t nextpos = 0;
  uint64_t bytes_buffered = 0;
  for ( auto it = buffer.begin(); it != buffer.end(); it++ ) {
    if ( nextpos >= it->first_index ) {
      if ( it->first_index + it->data.size() > nextpos ) {
        bytes_buffered += it->data.size() - ( nextpos - it->first_index );
        nextpos = max( it->first_index + it->data.size(), nextpos );
      }
    } else {
      bytes_buffered += it->data.size();
      nextpos = it->data.size() + it->first_index;
    }
  }
  return bytes_buffered;
}
