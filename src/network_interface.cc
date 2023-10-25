#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t next_hop_ip = next_hop.ipv4_numeric();
  if(arp_table.count(next_hop_ip)){
    EthernetFrame frame;
    frame.header.dst=arp_table[next_hop_ip].first;
    frame.header.src=ethernet_address_;
    frame.header.type=EthernetHeader::TYPE_IPv4;
    frame.payload=serialize(dgram);
    sending_queue.push(frame);
  }else{
      EthernetFrame frame;
      frame.header.src=ethernet_address_;
      frame.header.type=EthernetHeader::TYPE_IPv4;
      frame.payload=serialize(dgram);
      waiting_queue[next_hop_ip].push_back(frame);
      if(!arp_request_table.count(next_hop_ip)){
        ARPMessage arp;
        arp.opcode=ARPMessage::OPCODE_REQUEST;
        arp.sender_ethernet_address=ethernet_address_;
        arp.sender_ip_address=ip_address_.ipv4_numeric();
        arp.target_ip_address=next_hop_ip;
        arp_request_table[next_hop_ip]=0;
        frame.header.dst=ETHERNET_BROADCAST;
        frame.header.src=ethernet_address_;
        frame.header.type=EthernetHeader::TYPE_ARP;
        frame.payload=serialize(arp);
        sending_queue.push(frame);
      }
  }
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if(frame.header.dst!=ethernet_address_ && frame.header.dst!=ETHERNET_BROADCAST){
    return {};
  }
  if(frame.header.type==EthernetHeader::TYPE_IPv4){
    InternetDatagram dgram;
    if(parse(dgram,frame.payload)){
      return dgram;
    }
  }else if(frame.header.type==EthernetHeader::TYPE_ARP){
    ARPMessage arp;
    if(parse(arp,frame.payload)){
      arp_table[arp.sender_ip_address]={arp.sender_ethernet_address,0};
      if(arp.opcode==ARPMessage::OPCODE_REQUEST){
        if(arp.target_ip_address==ip_address_.ipv4_numeric()){
          ARPMessage reply;
          reply.opcode=ARPMessage::OPCODE_REPLY;
          reply.sender_ethernet_address=ethernet_address_;
          reply.sender_ip_address=ip_address_.ipv4_numeric();
          reply.target_ethernet_address=arp.sender_ethernet_address;
          reply.target_ip_address=arp.sender_ip_address;
          EthernetFrame newframe;
          newframe.header.dst=arp.sender_ethernet_address;
          newframe.header.src=ethernet_address_;
          newframe.header.type=EthernetHeader::TYPE_ARP;
          newframe.payload=serialize(reply);
          sending_queue.push(newframe);
        }
        // else if(frame.header.dst == ETHERNET_BROADCAST){
        //   sending_queue.push(frame);
        // }
      }
      auto datagrams=waiting_queue[arp.sender_ip_address];
      for(auto& i:datagrams){
        i.header.dst=arp.sender_ethernet_address;
        sending_queue.push(i);
      }
      waiting_queue.erase(arp.sender_ip_address);
    }
  }
  return {};
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  for(auto it=arp_table.begin();it!=arp_table.end();){
    it->second.second+=ms_since_last_tick;
    if(it->second.second>map_lmt){
      it=arp_table.erase(it);
    }else{
      it++;
    }
  }
  for(auto it=arp_request_table.begin();it!=arp_request_table.end();){
    it->second+=ms_since_last_tick;
    if(it->second>arp_request_lmt){
      it=arp_request_table.erase(it);
    }else{
      it++;
    }
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if(sending_queue.empty()){
    return {};
  }
  EthernetFrame frame=sending_queue.front();
  sending_queue.pop();
  return frame;
}
