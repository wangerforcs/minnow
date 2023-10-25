Checkpoint 0 Writeup
====================

该lab要实现一个字节流，兼具写入和读出的能力，并且buffer空间受限。
根据要实现的函数和读写功能，内部要存储的成员为
- std::queue\<std::string\> buffer_ {}; 用于存储写入的字符串(原本用的std::queue<char>，但由于queue内存不连续，调用peek返回string_view时还要将char组合为string或者每次只返回一个字符，导致空间和时间效率低，所以选择了直接存储string，更高效的版本还可以存储一个std::queue\<std::string_view\>)
- uint64_t capacity_ buffer容量
- uint64_t bytes_pushed_ 写入的字节数
- uint64_t bytes_popped_ 读出的字节数
- bool is_closed_ 是否关闭
- bool has_error_ 是否有错误
- uint64_t cur 当前读取的位置，指向buffer队首字符串

### 主要函数实现
- void Writer::push( string data )
  将字符串写入buffer，buffer中不允许有空字符串，所以如果data为空或者无可用空间，直接返回；否则将data子串(大小有data长度和可用空间确定)入队

- string_view Reader::peek() const
  返回buffer中的剩余字符串，如果buffer非空，至少返回长度为1的string_view

- void Reader::pop( uint64_t len )
  删除buffer中累计长度为len的字符串，配合cur移动

- 其余函数主要是返回ByteStream状态和数据，利用成员变量即可获得
```C++
class ByteStream
{
protected:
  uint64_t capacity_;
  // Please add any additional state to the ByteStream here, and not to the Writer and Reader interfaces.
  /*
  simple version cause queue stores char, and the memory of queue is uncontinuous,
  so we can't use pointer to get the address of the next element, in peek we can only get one char each time
  https://zhuanlan.zhihu.com/p/622628007
  std::queue<char> buffer_{};
  uint64_t bytes_pushed_ = 0;
  uint64_t bytes_popped_ = 0;
  bool is_closed_ = false;
  bool has_error_ = false;
  */
  // good enough
  std::queue<std::string> buffer_ {};
  uint64_t bytes_pushed_ = 0;
  uint64_t bytes_popped_ = 0;
  bool is_closed_ = false;
  bool has_error_ = false;
  uint64_t cur = 0;
  /*
  better https://zhuanlan.zhihu.com/p/630739394
  using string_view.remove_prefix and queue<string_view> to quikly pop data
  */
public:
  explicit ByteStream( uint64_t capacity );

  // Helper functions (provided) to access the ByteStream's Reader and Writer interfaces
  Reader& reader();
  const Reader& reader() const;
  Writer& writer();
  const Writer& writer() const;
};
void Writer::push( string data )
{
  // Your code here.
  uint64_t len = data.size();
  len = min( len, available_capacity() );
  // do not just test data, or you will push "", which is hard to correct
  if ( len == 0 )
    return;
  buffer_.push( data.substr( 0, len ) );
  bytes_pushed_ += len;
}
void Reader::pop( uint64_t len )
{
  // Your code here.
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
```