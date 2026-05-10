#ifndef __LASER_H
#define __LASER_H

/* Initialise laser DAC output.  device_name is the SDL audio device name for
 * the DAC-ILDA interface (3-channel: ch0=X, ch1=Y, ch2=Z/blank).
 * Pass NULL to use the system default output device.
 * If device_name is an empty string ("") laser output is disabled entirely. */
void laser_init(const char *device_name);
void laser_done(void);

/* Called once per Vectrex clock tick (1.5 MHz) from alg_sstep().
 * Downsamples to the DAC sample rate and enqueues into the ring buffer.
 * x in [0, ALG_MAX_X], y in [0, ALG_MAX_Y], blank: 1=beam on, 0=beam off. */
void laser_push(long x, long y, unsigned blank);

#endif
