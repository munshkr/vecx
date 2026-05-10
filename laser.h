#ifndef __LASER_H
#define __LASER_H

/* Initialise laser DAC output.
 *   device_name    — SDL audio device name for the DAC-ILDA interface.
 *                   Pass NULL to use the system default output device.
 *                   Pass "" to disable laser output entirely.
 *   channel_map_str — comma-separated output channel indices for X, Y, Z.
 *                   e.g. "0,1,2" maps X→ch0, Y→ch1, Z→ch2 (default).
 *                   Pass NULL or "" to use the default mapping.
 * The device's native sample rate is used automatically. */
void laser_init(const char *device_name, const char *channel_map_str);
void laser_done(void);

/* Called once per Vectrex clock tick (1.5 MHz) from alg_sstep().
 * Downsamples to the DAC sample rate and enqueues into the ring buffer.
 * x in [0, ALG_MAX_X], y in [0, ALG_MAX_Y], blank: 1=beam on, 0=beam off. */
void laser_push(long x, long y, unsigned blank);

#endif
