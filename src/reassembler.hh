#pragma once

#include "byte_stream.hh"

#include <string>
#include <set>
class Reassembler
{
private:
  struct datagram
  {
    uint64_t first_index;
    std::string data;
    bool operator<( const datagram& rhs ) const { 
      if(first_index == rhs.first_index)  return data.size()<rhs.data.size();
      return first_index < rhs.first_index; 
    }
    datagram( uint64_t first_indext, const std::string& datat )
      : first_index( first_indext )
      , data( datat )
    {}
  };
  std::set<datagram> buffer {};
  uint64_t bytes_written = 0;
  uint64_t first_unwritten = 0;
  bool coming_last = false;
public:
  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring, Writer& output );

  // How many bytes are stored in the Reassembler itself?
  uint64_t bytes_pending() const;
};
