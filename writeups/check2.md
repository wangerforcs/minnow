Checkpoint 2 Writeup
====================

该lab主要实现TCP接收方，接受来自发送方的TCPSenderMessage数据包，并使用checkpoint 1的Reasembler将接收到的数据推流到ByteStream，然后发送TCPReceiverMessage，指明接收方的窗口大小(ByteStream可用容量)和ACK number。

本题借助了前两个实验的类实现，所以本地TCPReceiver类并不需要本地缓存。唯一需要的成员变量就是isn(初始序列号)，即包装类Wrap32。

### 主要函数实现

- Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
  根据初始序列号和绝对序列号计算序列号，只需要初始序列号加上绝对序列号即可。

- uint64_t unwrap( Wrap32 zero_point, uint64_t checkpoint ) const;
  根据初始序列号和近似绝对序列号，计算绝对序列号。将初始序列号与checkpoint相加得到added，并计算和本地序列号的差值diff，通过比较序列号大小、diff和mod31的大小，就能得出绝对序列号和checkpoint的关系，进而求解即可。
  细分种类
  1. 本地序列号小于added，且diff小于等于mod31，说明绝对序列号距checkpoint更近，此时绝对序列号为checkpoint+diff
  2. 本地序列号小于added，且diff大于mod31，说明绝对序列号距checkpoint-mod32更近，此时绝对序列号为checkpoint+diff-mod32
  3. 本地序列号大于added，且diff小于等于mod31，说明绝对序列号距checkpoint更近，此时绝对序列号为checkpoint+diff
  4. 本地序列号大于added，且diff大于mod31，说明绝对序列号距checkpoint更近，此时绝对序列号为checkpoint+diff-mod32
  5. 注意checkpoint+diff\<mod32时不用减，因为没法取到下界，只能用上界
  6. 总结一下就可以得到代码中的简单形式

- void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
  记录isn，并将接收到的数据放入Reasembler重排，注意第一次写时绝对序列号和索引相等，之后索引等于绝对序列号减一，因为绝对序列号包含了SYN

- TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
  返回ACK number和窗口大小(收到syn之前ACK为空)，同时注意ACK包含了SYN和FIN

代码如下：
```C++
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
```