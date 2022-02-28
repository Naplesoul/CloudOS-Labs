# CloudOS-Labs



## Lab 1 - Reliable Data Transport Protocol (RDT)

#### 1. Build & Run

```shell
make rdt_sim

./rdt_sim 1000 0.1 100 0.3 0.3 0.3 0
```

#### 2. Packet Layout

```
|<-  4 byte  ->|<-  1  byte  ->|<-  1 byte  ->|<- the rest (max 120 byte) ->|<-  2 byte  ->|
|  packet  ID  | function code | payload size |<-         payload         ->|   checksum   |
```

传输层最小发送单元就是一个packet，大小为128字节。其中，packet id 为32位无符号整数。payload size是8位无符号整数，表示后面的120字节中有多少字节是有效的数据。packet中最后的2个字节是前面126个字节的校验码。function code由8个bit组成，低4位每一位都是一个标记，可以用或运算组合多个function code，定义如下所示。

```c++
enum FunctionCode
{
    PKT_ACK = 1, NORMAL_MSG = 2, NEW_MSG = 4, END_MSG = 8
};
```

PKT_ACK是receiver给sender发的ack，NORMAL_MSG是sender给receiver发的数据包，并且每个msg的第一个包需要加上NEW_MSG，最后一个包要加上END_MSG，以此来区分msg的边界。

#### 3. Strategies - Sliding Window with Selective Repeat

当sender从上层收到一个msg后，根据其大小把data拆分成能放入packet的若干段，并且将packet id设置为前一个id + 1，每个packet的function code设置为NORMAL_MSG，该msg的第一个packet加上NEW_MSG，最后一个包加上END_MSG，然后将packet放入buffer中等待发送。发包是由sliding window控制的，通过收到的ack不断地推着window向后滑动。

由于packet id在前后两个msg中是连续的，并且由function code指明msg的起始和结尾，因此只要保证packet id的有序性就可以保证msg的有序性。

sender发送一个packet时会先设置它的packet，并且设置上timeout的倒计时。如果倒计时结束还没有收到ack，则认为丢包或者包出错，进行重发。

receiver有两个buffer：integrated_buffer和seq_buffer。integrated_buffer是存放完全连续的包的buffer，在integrated_buffer中的packet随时可以被组装成msg，只要msg的最后一个packet加入integrated_buffer，则说明该msg的所有packet都在integrated_buffer中，可以立即组装成一个完整的msg返回给上层。而seq_buffer与之相对，其中的packet id是不完全连续的，但是按照从小到大的顺序排列，是用来存放发生乱序或者包损坏时已经收到的那些包，如果没有seq_buffer，就变成了GBN。

receiver收到sender给它的包后，先校验完整性，如果checksum对不上，就忽略这个包。如果包是完整的，就将这个包按packet id从小到大的顺序放入seq_buffer中。然后检查integrated_buffer的最后一个包和seq_buffer的第一个包是不是连续的，如果是，就不断地将seq_buffer的第一个包移到integrated_buffer中，直到不连续为止（或seq_buffer为空）。最后检查integrated_buffer中有没有完整的一个msg，如果有，就组装msg并发给上层。

#### 4. Implementation of the Timer

由于系统只有一个时钟，因此在每一个包发出去之后，将packet id和超时的真实时间（GetSimulationTime() + Timeout）存入一个链表中。每次时钟timeout了就查看链表，取出超时的那几项，并再次设定定时为下一个包超时的真实时间减去当前时间。如此就能时间用一个时钟记录多个包的定时。

#### 5. Test Results

```shell
./rdt_sim 1000 0.1 100 0.15 0.15 0.15 0
...
## Simulation completed at time 1003.51s with
	1010348 characters sent
	1010348 characters delivered
	39958 packets passed between the sender and the receiver
## Congratulations! This session is error-free, loss-free, and in order.

./rdt_sim 1000 0.1 100 0.3 0.3 0.3 0
...
## Simulation completed at time 1034.22s with
	990734 characters sent
	990734 characters delivered
	60289 packets passed between the sender and the receiver
## Congratulations! This session is error-free, loss-free, and in order.
```

测试结果与文档中SR的参考值较为接近。由于我将window size增大到了100，因此传输吞吐速度有所提高。由于这个lab的模拟过程没有对传输带宽作限制，因此window size增大可以直接提高吞吐速率。但在实际场景下，window size需要根据传输带宽、RTT等因素综合考虑而定。

#### 6. Problems & Difficulties

##### 内存泄漏

sender和receiver经常需要拷贝packet中的数据，而且在msg发送完成后就不在需要了，因此要及时地释放其占用的空间。而显式地去释放内存会让代码变得更复杂，因此我才用了 shared_ptr 和 make_shared( ) 来存packet中的data，这样不再使用这个packet后便可以自动释放内存。

##### 关于传输效率

在我一开始的实现中，我用一个空包（payload部分是无效的，function code 为 NEW_MSG 或 END_MSG）来表示一个msg开始或结束。但是测试结果不理想，虽然是用了SR，但是发包数却跟GBN差不多。后来我计算了一下，一个msg平均100B，就是平均一个packet的大小，但是需要再发头尾两个空包，包的数量一下子提高到3倍，非常浪费资源。因此我改用function code 的每一个bit来表示包的信息。这样只需要在头一个和尾一个包的function code的一位上置1就可以表示一个的msg的边界。

##### 校验

我在测试中出现了很尴尬的情况，就是包已经被破坏了，但是还能通过校验。我采用的是16位的Internet Checksum，16位较短，还存在包坏了还能过校验的概率。后来我就额外增加对packet_id的范围判断，如果packet_id在window范围外就直接忽略这个包。由于window相较于32位空间来说已经非常小了，一个包损坏了之后还能过校验，并且packet_id还落在window范围内的概率就更小了，后续的测试中也再没有出现这样的情况。
