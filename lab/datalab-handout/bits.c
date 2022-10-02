/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
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
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

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

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

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
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
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
/* Copyright (C) 1991-2022 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */
/* 
 * bitAnd - x&y using only ~ and | 
 *   Example: bitAnd(6, 5) = 4
 *   Legal ops: ~ |
 *   Max ops: 8
 *   Rating: 1
 */
int bitAnd(int x, int y) {
  return ~(~x|~y);//注意对称性
}
/* 
 * bitConditional - x ? y : z for each bit respectively
 *   Example: bitConditional(0b00110011, 0b01010101, 0b00001111) = 0b00011101
 *   Legal ops: & | ^ ~
 *   Max ops: 8
 *   Rating: 1
 */
int bitConditional(int x, int y, int z) {
  return (x&y)|((~x)&z);//前者取y中位数，后者取z中位数
}
/* 
 * byteSwap - swaps the nth byte and the mth byte
 *  Examples: byteSwap(0x12345678, 1, 3) = 0x56341278
 *            byteSwap(0xDEADBEEF, 0, 2) = 0xDEEFBEAD
 *  You may assume that 0 <= n <= 3, 0 <= m <= 3
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 2
 */
int byteSwap(int x, int n, int m) {
    int n_=n<<3,m_=m<<3;
    int t=0xff;
    int nth=(((x>>n_)&t)<<m_);//取第n个字节，并将之移至m的位置（其余位为0）
    int mth=(((x>>m_)&t)<<n_);
    x=x&(~(0xff<<n_))&(~(0xff<<m_));//将x的第n和第m个字节置为0
    return x|nth|mth;
}
/* 
 * logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 0 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3 
 */
int logicalShift(int x, int n) {
  int bias=~(((1<<n)+(~0))<<(32+~n+1));
  return (x>>n)&bias;
}
/* 
 * cleanConsecutive1 - change any consecutive 1 to zeros in the binary form of x.
 *   Consecutive 1 means a set of 1 that contains more than one 1.
 *   Examples cleanConsecutive1(0x10) = 0x10
 *            cleanConsecutive1(0xF0) = 0x0
 *            cleanConsecutive1(0xFFFF0001) = 0x1
 *            cleanConsecutive1(0x4F4F4F4F) = 0x40404040
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 25
 *   Rating: 4
 */
int cleanConsecutive1(int x){
  int bias=~((2+(~0))<<31);//逻辑右移
  return (~(((x<<1)&x)|(((x>>1)&bias)&x)))&x;//通过x与左移一位和逻辑右移一位的x的按位与运算，判断某一位1是否与其相邻两位相同，再将得到的两个结果通过按位或复合，得到连续重复的数
}
/* 
 * countTrailingZero - return the number of consecutive 0 from the lowest bit of 
 *   the binary form of x.
 *   YOU MAY USE BIG CONST IN THIS PROBLEM, LIKE 0xFFFF0000
 *   YOU MAY USE BIG CONST IN THIS PROBLEM, LIKE 0xFFFF0000
 *   YOU MAY USE BIG CONST IN THIS PROBLEM, LIKE 0xFFFF0000
 *   Examples countTrailingZero(0x0) = 32, countTrailingZero(0x1) = 0,
 *            countTrailingZero(0xFFFF0000) = 16,
 *            countTrailingZero(0xFFFFFFF0) = 8,
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 40
 *   Rating: 4
 */
int countTrailingZero(int x){
  //二分法
  int bit0,bit1,bit2,bit4,bit8,bit16;
  bit16=!(x<<16);//后16位如果有1的话，则要考察后16位，bit16=0,如果全为0，则只要考察前16位即可,bit16=1
  x=x<<((!bit16)<<4);//考察后16位，则舍去前16位
  bit8=!(x<<8);
  x=x<<((!bit8)<<3);
  bit4=!(x<<4);
  x=x<<((!bit4)<<2);
  bit2=!(x<<2);
  x=x<<((!bit2)<<1);
  bit1=!(x<<1);
  x=x<<((!bit1));
  bit0=!x;
  return bit0+(bit1)+(bit2<<1)+(bit4<<2)+(bit8<<3)+(bit16<<4);
}
/* 
 * divpwr2 - Compute x/(2^n), for 0 <= n <= 30
 *  Round toward zero
 *   Examples: divpwr2(15,1) = 7, divpwr2(-33,4) = -2
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int divpwr2(int x, int n) {
    //x>=0:bias=0
    //x<0:bias=2^n-1->为了向0舍入
    int bias=(x>>31)&((1<<n)+(~0));//算术右移
    return (x+bias)>>n;
}
/* 
 * oneMoreThan - return 1 if y is one more than x, and 0 otherwise
 *   Examples oneMoreThan(0, 1) = 1, oneMoreThan(-1, 1) = 0
 *   Legal ops: ~ & ! ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int oneMoreThan(int x, int y) {
  int signxy=(!((x>>31)&1))&((y>>31)&1);//如果x为正，y为负，则肯定不可能（溢出）
  int subyx=y+~x+1;//y-x
  return (!(1^subyx))&!signxy;
}
/*
 * satMul3 - multiplies by 3, saturating to Tmin or Tmax if overflow
 *  Examples: satMul3(0x10000000) = 0x30000000
 *            satMul3(0x30000000) = 0x7FFFFFFF (Saturate to TMax)
 *            satMul3(0x70000000) = 0x7FFFFFFF (Saturate to TMax)
 *            satMul3(0xD0000000) = 0x80000000 (Saturate to TMin)
 *            satMul3(0xA0000000) = 0x80000000 (Saturate to TMin)
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 3
 */
int satMul3(int x) {
  int y1=x<<1;
  int Tmin=1<<31;
  int isChangeSign1=((Tmin)&(y1^x))>>31;//首先通过判断x*2有无变号来判断第一重溢出
  int Tm=((Tmin)&x)+((~(x>>31))&(~(Tmin)));//根据符号算出溢出数
  int y2=y1+x;
  int isChangeSign2=((Tmin)&(y1^y2))>>31;//再根据x*2+x的过程中有无变号来判断第二重溢出
  return (isChangeSign1&Tm)+((~isChangeSign1)&((isChangeSign2&Tm)+((~isChangeSign2)&y2)));//如此嵌套两层判断，只要有一重变号就取极值
}
/* 
 * subOK - Determine if can compute x-y without overflow
 *   Example: subOK(0x80000000,0x80000000) = 1,
 *            subOK(0x80000000,0x70000000) = 0, 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int subOK(int x, int y) {
  int signxy=!(((x>>31)&1)^((y>>31)&1));//判断是否同号，同号不会溢出
  int subxy=x+~y+1;
  int signsubxy=!(((x>>31)&1)^((subxy>>31)&1));//根据是否变号来判断是否溢出
  return signxy|signsubxy;
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
  return !!(((!bitSignSub)&(!(!(~(~bitSign+1)))))+((bitSign>>31)&1));//考虑异号的情况(导致溢出),若同号，只需判断符号即可
}
/*
 * trueThreeFourths - multiplies by 3/4 rounding toward 0,
 *   avoiding errors due to overflow
 *   Examples: trueThreeFourths(11) = 8
 *             trueThreeFourths(-9) = -6
 *             trueThreeFourths(1073741824) = 805306368 (no overflow)
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 4
 */
int trueThreeFourths(int x)
{
  int y=x>>2;
  int bias=(x>>31)&(3);
  int ans1=(y<<1)+y;
  int s=x&3;//将后两位取出来
  int ans2=(((s<<1)+s+bias)>>2);//将偏置加至后两位，因为只有后两位才有误差
  return ans1+ans2;
}
/* 
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
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
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
  unsigned exp=0;
  unsigned frac=0;
  unsigned sign=x&(1<<31);
  unsigned rounding=0;
  unsigned X;
  unsigned t;
  if(sign)
    X=~x+1;
  else
    X=x;//取x的绝对值
  t=X;
  while((t=t>>1))
  {
    exp++;//计算X的有效位数
  }
  frac=(X<<(31-exp))<<1;//bias=31
  rounding=(frac<<23)>>23;//销掉frac的多余位数
  frac=frac>>9;
  if(rounding>256)
    rounding=1;
  else if(rounding==256)
    rounding=frac&1;
  else
    rounding=0;
  if(x)
    return rounding+(sign |((exp+127)<<23) | frac);
  else 
    return 0;


  

}
/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
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
int float_f2i(unsigned uf) {
  
  //uf向0舍入
  int s=(uf>>31)&1;//符号
  int bias=127;//2^(k-1)-1,k=8
  int exp=((uf&(0x7F800000))>>23)-bias;//阶码
  int frac=(uf&(0x007FFFFF))|(0x00800000);//尾数,|0x00800000->M=1+f，因为后23位表示f，故要将第24位置为零
  int result=0;
  int ansS;
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
  ansS = ((result>>31)&1);
  //如果前后符号一致，则直接返回
  //若原本为正而现在为负，则返回溢出值
  if(ansS)
    return 0x80000000U;
  else if(s==ansS)
    return result;
    //若原本为负而现在为正，则返回相反数  
  else
    return  ~result+1;
}
/* 
 * float_pwr2 - Return bit-level equivalent of the expression 2.0^x
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
unsigned float_pwr2(int x) {
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
