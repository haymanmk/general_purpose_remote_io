#include "stm32f7xx_remote_io.h"

#define MAX_INT_DIGITS 8

// Extracts a floating point value from a string. The following code is based loosely on
// the avr-libc strtod() function by Michael Stumpf and Dmitry Xmelkov and many freely
// available conversion method examples, but has been highly optimized for Grbl. For known
// CNC applications, the typical decimal value is expected to be in the range of E0 to E-4.
// Scientific notation is officially not supported by g-code, and the 'E' character may
// be a g-code word on some CNC systems. So, 'E' notation will not be recognized.
// NOTE: Thanks to Radu-Eosif Mihailescu for identifying the issues with using strtod().
uint8_t utils_read_float(char *line, uint8_t *char_counter, float *float_ptr)
{
  char *ptr = line + *char_counter;
  unsigned char c;

  // Grab first character and increment pointer. No spaces assumed in line.
  c = *ptr++;

  // Capture initial positive/minus character
  bool isnegative = false;
  if (c == '-') {
    isnegative = true;
    c = *ptr++;
  } else if (c == '+') {
    c = *ptr++;
  }

  // Extract number into fast integer. Track decimal in terms of exponent value.
  uint32_t intval = 0;
  int8_t exp = 0;
  uint8_t ndigit = 0;
  bool isdecimal = false;
  while(1) {
    c -= '0';
    if (c <= 9) {
      ndigit++;
      if (ndigit <= MAX_INT_DIGITS) {
        if (isdecimal) { exp--; }
        intval = (((intval << 2) + intval) << 1) + c; // intval*10 + c
      } else {
        if (!(isdecimal)) { exp++; }  // Drop overflow digits
      }
    } else if (c == (('.'-'0') & 0xff)  &&  !(isdecimal)) {
      isdecimal = true;
    } else {
      break;
    }
    c = *ptr++;
  }

  // Return if no digits have been read.
  if (!ndigit) { return(false); };

  // Convert integer into floating point.
  float fval;
  fval = (float)intval;

  // Apply decimal. Should perform no more than two floating point multiplications for the
  // expected range of E0 to E-4.
  if (fval != 0) {
    while (exp <= -2) {
      fval *= 0.01;
      exp += 2;
    }
    if (exp < 0) {
      fval *= 0.1;
    } else if (exp > 0) {
      do {
        fval *= 10.0;
      } while (--exp > 0);
    }
  }

  // Assign floating point value with correct sign.
  if (isnegative) {
    *float_ptr = -fval;
  } else {
    *float_ptr = fval;
  }

  *char_counter = ptr - line - 1; // Set char_counter to next statement

  return(true);
}