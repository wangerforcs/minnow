#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return zero_point + (uint32_t)n;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  const uint64_t mod31 = 1ull << 31;
  const uint64_t mod32 = 1ull << 32;

  uint32_t added = zero_point.raw_value_ + checkpoint;
  uint32_t diff = ( raw_value_ - added );
  if ( diff <= mod31 || checkpoint + diff < mod32 )
    return checkpoint + diff;
  return checkpoint + diff - mod32;
}
