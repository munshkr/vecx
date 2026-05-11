#ifndef __LASER_H
#define __LASER_H

/* Initialise the unified audio output device.
 *   device_name   — SDL audio device name. Pass NULL for the system default.
 *   audio_l_ch    — output channel index for PSG left  (default 0).
 *   audio_r_ch    — output channel index for PSG right (default 1).
 *   x_ch, y_ch, z_ch — output channel indices for laser X, Y, Z/blank
 *                       (defaults 2, 3, 4).
 *   flip_x, flip_y — non-zero to invert the respective laser axis.
 * The device's native sample rate is used automatically. */
void laser_init(const char *device_name, int audio_l_ch, int audio_r_ch,
                int x_ch, int y_ch, int z_ch, int flip_x, int flip_y);
void laser_done(void);

/* Called once per Vectrex clock tick (1.5 MHz) from alg_sstep().
 * Downsamples to the DAC sample rate and enqueues into the ring buffer.
 * x in [0, ALG_MAX_X], y in [0, ALG_MAX_Y], blank: 1=beam on, 0=beam off. */
void laser_push(long x, long y, unsigned blank);

#endif
