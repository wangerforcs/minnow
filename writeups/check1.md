Checkpoint 1 Writeup
====================

该lab要根据首字母索引来对收到的字符串进行重组，还原为原始数据(字符串可能乱序到达，可能有重叠)
思路是将按顺序并小于可用容量的字符串(可能是部分子串)直接推流到输出流，将失序但在可用容量内的字符串放入本地buffer。
考虑到最好用首字符索引对收到的字符串进行排序，这样可以保证在buffer中的字符串是有序的，然后再从buffer中取出并推流到输出流；而且由于并不对buffer中的字符串去重(即去除各子串重叠部分)，在统计存储在本地buffer而未推流的有效字符串长度时，还要遍历buffer，求区间覆盖最大长度。因此我选择了set来存储字符串，包装秤datagram结构体，包含首字符索引和字符串，定义了datagrem的严格弱序。

这样会导致的一个可能的缺点是buffer中的字符串重叠导致本地存储较大，不过通过在每次推流后尽可能将buffer中所有按序可推流字符串推流，将已经无用的字符串除去，可以减轻存储压力。
类设计如下：
```C++
class Reassembler
{
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
};
```
### 主要函数
- void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )  
  接受一个字符串，并根据情况丢弃、推流或存入本地buffer。并在收到最后一个字符串且buffer清空(即全部推流)后关闭字节流。下面详细说明如何操作   
  - 根据是否是最后一个字符串设置本地标志位，注意必须要本地存储，因为字符串会失序到达，收到is_last_substring后可能还有字符串。
  - 处理该字符串
    - 如果已推流部分和收到的字符串之间无空隙，且具有新的可以推流的部分，则直接推流(超出可用容量部分丢弃)。
    - 如果已推流部分和收到的字符串之间有空隙，则将字符串中位于字节流可用容量之内的部分存入本地buffer(超出可用容量部分丢弃)。
  - 处理本地buffer，将与已推流部分相邻的字符串(中间无间隔)推流，注意除去与已推流部分重叠的部分。
- uint64_t Reassembler::bytes_pending() const
  返回存在本地的字符串的总长度(去处重复部分)，把每个字符串看做一个区间，相当于求区间覆盖总长度，并且由于set已经排好序，求解很方便

自我感觉这部分代码实现的简洁清晰，代码量也比较少。
举一个例子，ByteStream容量为4，并且只存入，不读出：
| first_index | data |buffer|pending|out|
| ----------- | ---- |---|---|---|
| 1 | "b" | "b" | 1 | NULL |
| 1 | "bcde" | "b","bcd" | 2 | NULL |
| 0 | "abc" | NULL | 0 | "abcd" |

源代码
```C++
void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
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
```