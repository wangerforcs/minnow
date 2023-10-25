#include <stdexcept>

#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  // Your code here.
  (void)data;
  uint64_t len = data.size();
  len = min( len, available_capacity() );
  // do not just test data, or you will push "", which is hard to correct
  if ( len == 0 )
    return;
  buffer_.push( data.substr( 0, len ) );
  bytes_pushed_ += len;
}

void Writer::close()
{
  // Your code here.
  is_closed_ = true;
}

void Writer::set_error()
{
  // Your code here.
  has_error_ = true;
}

bool Writer::is_closed() const
{
  // Your code here.
  return is_closed_;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ + bytes_popped_- bytes_pushed_;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_;
}

string_view Reader::peek() const
{
  // Your code here.
  string_view s = string_view(buffer_.front());
  s = s.substr(cur);
  return s;
}

bool Reader::is_finished() const
{
  // Your code here.
  return is_closed_ && bytes_buffered() == 0;
}

bool Reader::has_error() const
{
  // Your code here.
  return has_error_;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  (void)len;
  len = min( len, bytes_buffered() );
  uint64_t poped = 0;
  while ( poped < len ) {
    uint64_t sz = buffer_.front().size();
    if ( sz - cur <= len - poped ) {
      poped += sz - cur;
      buffer_.pop();
      cur = 0;
    } else {
      cur += len - poped;
      poped = len;
    }
  }
  bytes_popped_ += poped;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return bytes_pushed_ - bytes_popped_;
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}
