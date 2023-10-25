#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  (void)route_prefix;
  (void)prefix_length;
  (void)next_hop;
  (void)interface_num;

  if ( next_hop.has_value() ) {
    router_table.push_back( { route_prefix, prefix_length, next_hop.value().ipv4_numeric(), interface_num } );
  } else {
    router_table.push_back( { route_prefix, prefix_length, std::nullopt, interface_num } );
  }
}

void Router::route()
{
  // !!! &
  for ( auto& inter : interfaces_ ) {
    std::optional<InternetDatagram> maybe_dgram = inter.maybe_receive();
    if ( maybe_dgram.has_value() ) {
      InternetDatagram dgram = maybe_dgram.value();
      if ( dgram.header.ttl <= 1 ) {
        continue;
      }
      dgram.header.ttl--;
      // !!!!
      dgram.header.compute_checksum();
      uint32_t dst_ip = dgram.header.dst;
      int8_t max_prefix = -1;
      size_t max_prefix_index = 0;
      for ( size_t i = 0; i < router_table.size(); i++ ) {
          if ( !router_table[i].prefix_length || (( dst_ip >> ( 32 - router_table[i].prefix_length ) )
               == ( router_table[i].route_prefix >> ( 32 - router_table[i].prefix_length ) )) ) {
            if(router_table[i].prefix_length > max_prefix)
            {
              max_prefix = router_table[i].prefix_length;
              max_prefix_index = i;
            }
        }
      }
      if(max_prefix == -1)
        continue;
      interfaces_[router_table[max_prefix_index].interface_num].send_datagram(
        dgram, Address::from_ipv4_numeric( router_table[max_prefix_index].next_hop.value_or( dst_ip ) ) );
    }
  }
}
