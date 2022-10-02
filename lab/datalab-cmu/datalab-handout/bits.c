/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * shouchenchen 2022
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {

  return ~(~(x&(~y))&~((~x)&y));
  /*
  * Xor运算是可交换的，所以表达式必然是对称的
  * x&~y:
  *   | 1| 0
  * 1 | 0| 1
  * 0 | 0| 0
  * ~x&y:
  *   | 1| 0
  * 1 | 0| 0
  * 0 | 1| 0
  */
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {

  return 1<<31;

}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) {
  int a=x+1;
  //Tmax:0x011..11+1=0x100..00
  //-1:-1+1=0
  a=a+x;
  //Tmax:0x10..00+0x01..11=0xff..ff
  //-1:0+0xff..ff=0xff..ff
  a=~a;
  //~0xff..ff=0
  a=a+!(x+1);
  //Tmax:0+0=0
  //-1:0+!(0)=1
  //others:num1+0=num1
  return !a;
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
  //构造奇数位都为1的数(0~255)
  int odd_8=0xAA;//0x10101010
  int odd_16=(odd_8<<8)+odd_8;
  int odd_32=(odd_16<<16)+odd_16;
  int res=(x&odd_32)^odd_32;//x&odd_32将x的偶数位全置零,x^x=0
  return !res;
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return ~x+1;
  //详见CS:APP课本p66
}
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
  int negateX=~x+1;
  int subUpper=negateX+0x39;
  int subLower=negateX+0x2F;//0x30为特殊情况
  int tstUpper=(subUpper>>31)&1;
  int tstLower=(subLower>>31)&1;//取符号位
  return (tstUpper^tstLower)&!(x>>31);//仅考虑x为正数时，否则会出现溢出，而且x为负数本身就不满足条件
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
  int z_=z&((!x<<31)>>31);
  int y_=y&(~((!x<<31)>>31));
  //得0或0xff..ff
  return y_+z_;
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  int sub=y+(~x+1);//y-x
  int bitSign=((y>>31)&1)+~((x>>31)&1)+1;
  int bitSignSub=(sub>>31)&1;
  return !!(((!bitSignSub)&(!(!(~(~bitSign+1)))))+((bitSign>>31)&1));
  //考虑异号的情况(导致溢出),若同号，只需判断符号即可
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
  int negX=~x+1;
  int sign=((x|negX)>>31)&1;
  //0与其相反数的符号相同，其他数则相异，取符号位判断即可
  return 1+~sign+1;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
  //从高位到低位依次筛查(二分法)
  //如果该数为负，则按位取反，因为需要去掉符号位进行计数(最后会加上符号位)
  int sign=x>>31;
  int bit16,bit8,bit4,bit2,bit1,bit0;//用于计数
  x=x^sign;
  //1^1=0;0^1=1;-->~x=x^0xff..ff
  //1^0=1;0^0=0;-->x=x^0
  bit16=(!!(x>>16))<<4;
  //判断16-32位是否存在1，若存在，则计bit16=16，否则bit16=0
  x=x>>bit16;
  //若16-32位存在1，则可不考虑0-16位，否则x不变，继续搜索0-16位
  bit8=(!!(x>>8))<<3;
  //若16-32位存在1，则考察24-32位是否存在1；否则，考察8-32位是否存在1
  x=x>>bit8;
  bit4=(!!(x>>4))<<2;
  x=x>>bit4;
  bit2=(!!(x>>2))<<1;
  x=x>>bit2;
  bit1=(!!(x>>1));
  x=x>>bit1;
  bit0=x;
  return bit16+bit8+bit4+bit2+bit1+bit0+1;
}
//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) {
  int E=((uf&(0x7F800000))>>23);//阶码
  int S=uf&(1<<31);//符号
  //若uf=NaN/无穷大
  if(E==0xFF)
    return uf;
  //若E=0，uf为规格化的数，则uf*2可以用uf<<1表示，|S保留原本的符号位
  if(E==0)
    return (uf<<1)|S;
  //如果当E+1(即uf*2)后E=0xff，说明溢出，返回无穷大(保留符号)
  if(E+1==0xFF)
    return 0x7F800000|S;
  //普通情况,先将uf的阶级码置0，然后将+1后的阶码赋值给uf
  return ((E+1)<<23)|(uf&0x807FFFFF);
}
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) {
  /*这题好像有点问题*/
  //uf向0舍入
  /*
  int s=(uf>>31)&1;//符号
  int bias=127;//2^(k-1)-1,k=8
  int exp=((uf&(0x7F800000))>>23)-bias;//阶码
  int frac=(uf&(0x007FFFFF))|(0x00800000);//尾数,|0x00800000->M=1+f，因为后23位表示f，故要将第24位置为零
  int result=0;
  //若本身为0
  if(!(uf&(0x7FFFFFFF)))
    return 0;
  //如果exp为负数，则返回0(因为一定不存在整数部分)
  if(exp<0 || exp+bias==0)
    return 0;
  //如果exp大于31，则一定越界(exp=23时，恰好res=frac)
  if(exp>31 || exp+bias==255)
    return 0x80000000U;
  //如果0<=exp<23,则res=frac>>(23-exp)
  //如果23<=exp<=31,则res=frac<<(exp-23)
  if(exp>23)
    result=frac<<(exp-23);
  else
    result=frac>>(23-exp);
  int ansS = (result>>31)&1;
  //如果前后符号一致，则直接返回
  if(s==ansS)
    return result;//若原本为正而现在为负，则返回溢出值
  else if(ansS)
    return 0x80000000U;//若原本为负而现在为正，则返回相反数  
  else
    return  ~result+1;
  */
  unsigned s = uf & (1 << 31), e = (uf >> 23) & 255, f = uf << 9 >> 9;
  if (e == 255) return 0x80000000u;
  if (e <= 126) return 0;
  e -= 127;
  if (e >= 31) return 0x80000000u;
  f |= 1 << 23;
  if (e <= 23) return (s ? -1 : 1) * (f >> (23 - e));
  return (s ? -1 : 1) * (f << (e - 23));
}
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned floatPower2(int x) {
  //临界值
  //float所能表达的最小的2的幂数为0x00..001
  //此时(非规格化),E=1-bias=-126,M=f=2^-23,x=-126-23=-149
  //能表示的最大的二的幂次为0x7F000000
  //此时(规格化),E=0x11111110-bias=127
  //非规格化的最大值:1<<22,E=-126,M=f=2^-1,x=-127
  //非规格化时,M=2^(x+126),2^x=1<<(23-(-(x+126)))=1<<(x+149)
  //规格化时,M=0+1=0,E=x=eps-127,2^x=eps<<23=(x+127)<<23
  if(x<-149)
    return 0;
  if(x>127)
    return 0x7F800000;
  if(x<-126)
    return 1<<(x+149);
  else
    return (x+127)<<23;
}
