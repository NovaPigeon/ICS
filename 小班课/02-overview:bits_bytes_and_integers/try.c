#include <stdio.h>
#include <string.h>
int main()
{
    int a=0;
    int b=-1;
    int c=2147483647;
    int d=0x80000000;
    int e=0xffffffff;
    int f=0x7fffffff;
    int g=-2147483648;
    int h=-2147483647-1;
    printf("%d %d %d %d %d %d %d %d\n",a,b,c,d,e,f,g,h);

    int aa=(unsigned)a;
    int bb=(unsigned)b;
    int cc=(unsigned)c;
    int dd=(unsigned)d;
    int ee=(unsigned)e;
    int ff=(unsigned)f;
    int gg=(unsigned)g;
    int hh=(unsigned)h;
    printf("%d %d %d %d %d %d %d %d\n",aa,bb,cc,dd,ee,ff,gg,hh);

    unsigned aaa=a;
    unsigned bbb=b;
    unsigned ccc=c;
    unsigned ddd=d;
    unsigned eee=e;
    unsigned fff=f;
    unsigned ggg=g;
    unsigned hhh=h;
    printf("%u %u %u %u %u %u %u %u\n",aaa,bbb,ccc,ddd,eee,fff,ggg,hhh);

    long aaaa=(unsigned)a;
    long bbbb=(unsigned)b;
    long cccc=(unsigned)c;
    long dddd=(unsigned)d;
    long eeee=(unsigned)e;
    long ffff=(unsigned)f;
    long gggg=(unsigned)g;
    long hhhh=(unsigned)h;
    printf("%ld %ld %ld %ld %ld %ld %ld %ld\n",aaaa,bbbb,cccc,dddd,eeee,ffff,gggg,hhhh);

    short aaaaa=(unsigned)a;
    short bbbbb=(unsigned)b;
    short ccccc=(unsigned)c;
    short ddddd=(unsigned)d;
    short eeeee=(unsigned)e;
    short fffff=(unsigned)f;
    short ggggg=(unsigned)g;
    short hhhhh=(unsigned)h;
    printf("%d %d %d %d %d %d %d %d\n",aaaaa,bbbbb,ccccc,ddddd,eeeee,fffff,ggggg,hhhhh);

    int aaaaaa=(unsigned short)a;
    int bbbbbb=(unsigned short)b;
    int cccccc=(unsigned short)c;
    int dddddd=(unsigned short)d;
    int eeeeee=(unsigned short)e;
    int ffffff=(unsigned short)f;
    int gggggg=(unsigned short)g;
    int hhhhhh=(unsigned short)h; 
    printf("%d %d %d %d %d %d %d %d\n",aaaaaa,bbbbbb,cccccc,dddddd,eeeeee,ffffff,gggggg,hhhhhh);

    unsigned aaaaaaa=(unsigned short)a;
    unsigned bbbbbbb=(unsigned short)b;
    unsigned ccccccc=(unsigned short)c;
    unsigned ddddddd=(unsigned short)d;
    unsigned eeeeeee=(unsigned short)e;
    unsigned fffffff=(unsigned short)f;
    unsigned ggggggg=(unsigned short)g;
    unsigned hhhhhhh=(unsigned short)h; 
    printf("%u %u %u %u %u %u %u %u\n",aaaaaaa,bbbbbbb,ccccccc,ddddddd,eeeeeee,fffffff,ggggggg,hhhhhhh);

    int d1=5,d2=2;
    printf("%d %d %d %d\n",d1%d2,(-d1)%d2,d1%(-d2),(-d1)%(-d2));
    return 0;
}