#ifndef __LASER_H
#define __LASER_H

/* Initialise laser DAC output. Only call this when laser output is desired.
 *   device_name  — SDL audio device name for the audio-to-ILDA interface.
 *                  Pass NULL to use the system default output device.
 *   x_ch, y_ch, z_ch — zero-based output channel indices for X, Y, Z/blank.
 *                  Defaults are 0, 1, 2.
 * The device's native sample rate is used automatically. */
void laser_init(const char *device_name, int x_ch, int y_ch, int z_ch);
void laser_done(void);

/* Called once per Vectrex clock tick (1.5 MHz) from alg_sstep().
 * Downsamples to the DAC sample rate and enqueues into the ring buffer.
 * x in [0, ALG_MAX_X], y in [0, ALG_MAX_Y], blank: 1=beam on, 0=beam off. */
void laser_push(long x, long y, unsigned blank);

#endif
