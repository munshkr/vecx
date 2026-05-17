#ifndef __LASER_H
#define __LASER_H

#include "vecx.h" /* for vector_t */

/* Laser output mode. */
typedef enum {
  LASER_MODE_XYZ = 0, /* X + Y + Z/blank on three channels (default) */
  LASER_MODE_XY = 1   /* X + Y only; blank travel eliminated via a per-frame
                       * segment planner.  No Z channel required. */
} laser_mode_t;

/* Initialise the unified audio output device.
 *   device_name   — SDL audio device name. Pass NULL for the system default.
 *   audio_l_ch    — output channel index for PSG left  (default 0).
 *   audio_r_ch    — output channel index for PSG right (default 1).
 *   x_ch, y_ch, z_ch — output channel indices for laser X, Y, Z/blank
 *                       (defaults 2, 3, 4).  z_ch is unused in LASER_MODE_XY.
 *   flip_x, flip_y — non-zero to invert the respective laser axis.
 *   mode          — LASER_MODE_XYZ (default) or LASER_MODE_XY.
 * The device's native sample rate is used automatically. */
/* buf_samples: SDL audio buffer size in frames (power of 2, e.g. 256 or 512).
 * Smaller values reduce output latency at the cost of higher underrun risk. */
void laser_init(const char *device_name, int audio_l_ch, int audio_r_ch,
                int x_ch, int y_ch, int z_ch, int flip_x, int flip_y,
                laser_mode_t mode, int buf_samples);
void laser_done(void);

/* Called once per Vectrex clock tick (1.5 MHz) from alg_sstep().
 * In LASER_MODE_XYZ: downsamples to the DAC rate and enqueues into the ring.
 * In LASER_MODE_XY:  no-op; ring is fed by laser_submit_frame() instead.
 * x in [0, ALG_MAX_X], y in [0, ALG_MAX_Y], blank: 1=beam on, 0=beam off. */
void laser_push(long x, long y, unsigned blank);

/* Called once per emulator frame from vecx_emu() (no-op in LASER_MODE_XYZ).
 * Distributes the visible segment list across one frame's worth of DAC
 * samples.  Blank travel between segments is omitted; the beam jumps directly
 * to each segment start.  segs/count are vectors_draw/vector_draw_cnt taken
 * before the frame's vector lists are swapped. */
void laser_submit_frame(const vector_t *segs, int count);

#endif
