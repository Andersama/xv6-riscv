#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#include <stdarg.h>

static char digits[] = "0123456789ABCDEF";

static void
putc(int fd, char c)
{
  write(fd, &c, 1);
}

static void
printint(int fd, int xx, int base, int sgn)
{
  char buf[16];
  int i, neg;
  uint x;
  uint d;
  uint r;
  
  neg = 0;
  if(sgn && xx < 0){
    neg = 1;
    x = -xx;
  } else {
    x = xx;
  }

  i = 0;
  do{
    //Optimization Tip:
    //On certain instruction sets / and % operations
    //are performed at the same time with one
    //instruction. Grouping / and % nearby can help
    //your compiler pick up on this case.
    //The riscv specifications recommend performing
    //division and then remainder operations on the
    //chance underlying hardware might do this for
    //you at runtime.
    d = x / base;
    r = x % base;
    x = d;
    buf[i++] = digits[r];
  }while(x != 0);

  //if(neg)
  //  buf[i++] = '-';
  
  //Optimization Example:
  //When appending data so long as the cost of writing
  //to memory on target hardware would be less than
  //a branch it can be a good idea to always write
  //and adjust the location being written to as needed

  buf[i] = '-';
  i += neg;

  //Note: we use the boolean result to increment i
  //While the end result here is similar, this isn't
  //an optimization a compiler will perform without a
  //bit more work potentially making this possible
  //as these aren't semantically equivalent. This
  //version for example will always write a '-'
  //character into buf, while the original might have.

  while(--i >= 0) {
    putc(fd, buf[i]);
    //
    buf[i] = 0;
  }
}

static void
printptr(int fd, uint64 x) {
  int i;
  putc(fd, '0');
  putc(fd, 'x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    putc(fd, digits[x >> (sizeof(uint64) * 8 - 4)]);
}

// Print to the given fd. Only understands %d, %x, %p, %s.
void
vprintf(int fd, const char *fmt, va_list ap)
{
  char *s;
  int c, i, state;

  state = 0;
  for(i = 0; fmt[i]; i++){
    c = fmt[i] & 0xff;
    if(state == 0){
      if(c == '%'){
        state = '%';
      } else {
        putc(fd, c);
      }
    } else if(state == '%'){
      if(c == 'd'){
        printint(fd, va_arg(ap, int), 10, 1);
      } else if(c == 'l') {
        printint(fd, va_arg(ap, uint64), 10, 0);
      } else if(c == 'x') {
        printint(fd, va_arg(ap, int), 16, 0);
      } else if(c == 'p') {
        printptr(fd, va_arg(ap, uint64));
      } else if(c == 's'){
        s = va_arg(ap, char*);
        if(s == 0)
          s = "(null)";
        while(*s != 0){
          putc(fd, *s);
          s++;
        }
      } else if(c == 'c'){
        putc(fd, va_arg(ap, uint));
      } else if(c == '%'){
        putc(fd, c);
      } else {
        // Unknown % sequence.  Print it to draw attention.
        putc(fd, '%');
        putc(fd, c);
      }
      state = 0;
    }
  }
}

void
fprintf(int fd, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vprintf(fd, fmt, ap);
}

void
printf(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  vprintf(1, fmt, ap);
}
