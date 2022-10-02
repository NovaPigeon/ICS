# lecture 1 : bits,bytes,and integers
> 信息和消除不确定性相互关联。
> 数据量，熵
ISO 标准：Byte = 8 bits
4 个二进制读作16进制：11111111 = 0xff
内存可以看作以 Byte 为单位的数组。
ASCII 码一般用前 127 位。
## bit 的位操作
布尔代数：&,|,~,^
逻辑运算：&&,||,!
将位合起来，形成位向量
左移/右移：<<,>>（逻辑/算数）
## Integers
unsigned,two's complement
### Numeric Ranges
- Umin = 0
- Umax = 0xfff...f
- Tmin = 100..00
- Tmax = 011..11
- |Tmin| = Tmax + 1
- Umax = 2*Tmax+1
```
-x=~x+1
```
### U&T Numeric Values
U2B , U2T 为双射
T2U & U2T(位表示不变，但对于T的负数，存在2^x的差值)
114514U -> 指定为无符号数
```c
int tx,ty;
unsigned ux,uy;
tx=(int)ux;
uy=(unsigned)ty;

tx=ux;
ty=uy;//Implicit casting
```
转换时主要看是否有无符号有符号的转换。
譬如：-1>1U,强制把-1转换为0xffff
### Sign Extension/truncation
为保持其解码后的值不变，将符号位复制至要扩充至高位。
1001->11001,0111->00111
要截取，则直接去掉高位。（mod 2^(w-1))
100001->00001(mod 32)
### Unsigned Addition


