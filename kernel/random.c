#include <stdarg.h>

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

struct
{
  struct spinlock lock;
  uint8 seed;
} rand;

/*
  This function implements a linear feedback shift register (LFSR)
  which is a simple pseudo-random number generator.
  The function is seeded with a value, and each call to
  it returns the next pseudo-random number. That is,
  after every sub- sequent call to it, you should update the seed
  with the returned value (which is also the output random number).
*/
// Linear feedback shift register
// Returns the next pseudo-random number
// The seed is updated with the returned value
uint8 lfsr_char(uint8 lfsr)
{
  uint8 bit;
  // this line preforms 4 different right shifts to lfsr and calculates the xor between them
  // then the right most bit is taken by preforming the & operator with the number 1, this returns 0 or 1
  bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 4)) & 0x01;
  // if bit == 1 then we shift 1 7 times to the left to get 10000000
  // then lfsr is shifted 1 bit to the left so that it's left most bit will be 0
  // finally the or operator is applied to place bit in the left most position of the output
  lfsr = (lfsr >> 1) | (bit << 7);
  return lfsr;
}

/*
  Output pseudo-random 8-bit numbers (char) when readץ
  A call to the read system call on this device file should
  read n pseudo-random bytes into the buffer dst.
  The function should return the number of bytes written to the buffer.
  On failure, the function should return the amount of bytes
  it managed to write before the failure.
  A failure example is when given dst is not a valid address.
*/
int randomread(int fd, uint64 dst, int n)
{
  // uint64 size is 8 bytes
  // uint8 size is 1 byte
  acquire(&rand.lock);
  if (n > 8)
  {
    n = 8;
  } // n can't be larger than 8
  int i;
  for (i = 0; i < n; i++)
  { // We concatenate the output of lsfr to dst n times
    uint8 lfsrRet = lfsr_char(rand.seed);
    rand.seed = lfsrRet;
    if (either_copyout(fd, dst, &rand.seed, 1) == -1)
    {
      break;
    };
  }

  release(&rand.lock);
  return i;
}

/*
  A call to the write system call on this device file, when n is 1,
  should seed the random number generator with the byte pointed to by src.
  If n is not 1, the function should return -1.
  The function should return 1 on success.
*/
int randomwrite(int fd, uint64 src, int n)
{
  if (n != 1)
  {
    return -1;
  }
  int ret = 1;
  acquire(&rand.lock);
  ret = either_copyin(&rand.seed, src, src, 1);
  release(&rand.lock);
  return ret;
}

/*
  At the initialization of the device file, the random number generator
  should be seeded with the value 0x2A.
  The rest of the initializtion should be done similarly to the console device.
*/
void randominit(void)
{
  initlock(&rand.lock, "rand");
  // connect read and write system calls
  // to consoleread and consolewrite.
  devsw[RANDOM].read = randomread;
  devsw[RANDOM].write = randomwrite;
  rand.seed = 0x2A;
}