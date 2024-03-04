#pragma once

#include <stdlib.h>
#include <math.h>

#define max(a,b) ((a) > (b) ? a : b)
#define min(a,b) ((a) < (b) ? a : b)

/**@memberof t_bc_virfun
 * @brief Recursive function to compute virtual fundamentals*/
double rec_virfun(double* freqs, double* end, double divmin, double divmax, double approx)
{
    double inf,sup;
    double quo_min, quo_max;
    double quotient;
    double resu = 0.0;
  if (divmin <= divmax)
  {
    if (freqs==end) {
            return((divmin + divmax) / 2.);
        } else {
      sup = freqs[0] * approx;
      inf = freqs[0] / approx;
      quo_min = ceil(inf/divmax);
      quo_max = floor(sup/divmin);
      quotient = quo_min;
      while (quotient <= quo_max)
      {
        resu = rec_virfun(&freqs[1],end, max(inf/quotient, divmin), min(sup/quotient, divmax), approx);
        if (resu > 1.)
          return resu;
        quotient++;
      }
      return 1.;
    }
  }
  return 1.;
}

/**@memberof t_bc_virfun
 * @brief Convert the tolerance (in floating point MIDI) to a frequency factor*/
float midi2freq_approx(float midi)
{
  return pow(2,(midi/12.));//-1.;
}

/**@memberof t_bc_virfun
 * @brief Convert Hz to (floating point) MIDI*/
float freq2midi(float freqin)
{
  return 69+12*log2(freqin/440.);
}

/**@memberof t_bc_virfun
 * @brief Convert (floating point) MIDI to Hz*/
float midi2freq(float midin)
{
  return 440.*pow(2, (midin-69.)/12.);
}