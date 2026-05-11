#ifndef __E8910_H
#define __E8910_H

#include <stdint.h>

/* Initialise the AY-3-8910 PSG state. Call before e8910_write or
 * e8910_fill_samples. */
void e8910_init(void);

void e8910_write(int r, int v);

/* Synthesise num_samples mono signed-16 PCM samples into buf.
 * Called from the unified audio callback in laser.c. */
void e8910_fill_samples(int16_t *buf, int num_samples);

#endif
