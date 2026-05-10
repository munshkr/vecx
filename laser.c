#include "laser.h"
#include "SDL.h"
#include "vecx.h"
#include <stdio.h>

/* Ring buffer capacity — must be a power of 2.
 * At 44100 Hz the emulator fills ~882 samples per 20 ms burst; 4096 gives
 * roughly 93 ms of headroom, more than enough to absorb timer jitter. */
#define LASER_RING_SIZE 4096
#define LASER_RING_MASK (LASER_RING_SIZE - 1)

/* Hint passed to SDL; ALLOW_FREQUENCY_CHANGE lets the driver use its native
 * rate instead, which laser_dda_rate picks up from given.freq. */
#define LASER_REQUESTED_FREQ 44100

typedef struct {
  float x, y, z;
} laser_sample_t;

/* SPSC ring buffer shared between emulation thread (producer) and SDL audio
 * callback thread (consumer).  head is owned by the producer; tail is owned
 * by the consumer.  SDL_AtomicGet/Set include full memory fences so no
 * additional synchronisation is required. */
static laser_sample_t laser_ring[LASER_RING_SIZE];
static SDL_atomic_t laser_head; /* next write slot  */
static SDL_atomic_t laser_tail; /* next read  slot  */

static SDL_AudioDeviceID laser_device_id = 0;
static int laser_channels = 3;
/* channel_map[0/1/2] = which output channel index receives X/Y/Z */
static int laser_channel_map[3] = {0, 1, 2};

/* DDA accumulator for integer-ratio downsampling from VECTREX_MHZ to the
 * actual device frequency.  laser_dda_rate is the device sample rate. */
static unsigned laser_dda_accum = 0;
static unsigned laser_dda_rate = LASER_REQUESTED_FREQ;

/* -------------------------------------------------------------------------
 * Audio callback — runs in the SDL audio thread.
 * ------------------------------------------------------------------------- */
static void laser_callback(void *userdata, Uint8 *stream, int len) {
  float *out = (float *)(void *)stream;
  int nframes = len / (laser_channels * (int)sizeof(float));
  int ch = laser_channels;

  (void)userdata;

  for (int i = 0; i < nframes; i++) {
    int head = SDL_AtomicGet(&laser_head);
    int tail = SDL_AtomicGet(&laser_tail);
    float x, y, z;

    if (tail == head) {
      /* Underrun: park at centre with beam off — safe for high-power
       * lasers (a stuck lit beam can damage surfaces). */
      x = 0.0f;
      y = 0.0f;
      z = -1.0f;
    } else {
      x = laser_ring[tail].x;
      y = laser_ring[tail].y;
      z = laser_ring[tail].z;
      SDL_AtomicSet(&laser_tail, (tail + 1) & LASER_RING_MASK);
    }

    /* Zero the full frame, then place signals at the mapped channel indices.
     * This handles any channel count the device negotiated (e.g. 4 on macOS).
     */
    for (int c = 0; c < ch; c++)
      out[i * ch + c] = 0.0f;
    if (laser_channel_map[0] < ch)
      out[i * ch + laser_channel_map[0]] = x;
    if (laser_channel_map[1] < ch)
      out[i * ch + laser_channel_map[1]] = y;
    if (laser_channel_map[2] < ch)
      out[i * ch + laser_channel_map[2]] = z;
  }
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */
static void parse_channel_map(const char *str, int map[3]) {
  map[0] = 0;
  map[1] = 1;
  map[2] = 2; /* defaults: X→0, Y→1, Z→2 */
  if (str && str[0])
    sscanf(str, "%d,%d,%d", &map[0], &map[1], &map[2]);
}

void laser_init(const char *device_name, const char *channel_map_str) {
  SDL_AudioSpec req, given;
  int req_channels, max_ch;
  SDL_zero(req);

  if (device_name && device_name[0] == '\0')
    return; /* empty string disables laser output */

  parse_channel_map(channel_map_str, laser_channel_map);

  /* Determine how many channels the device must provide. */
  max_ch = laser_channel_map[0];
  if (laser_channel_map[1] > max_ch)
    max_ch = laser_channel_map[1];
  if (laser_channel_map[2] > max_ch)
    max_ch = laser_channel_map[2];
  req_channels = max_ch + 1;

  req.freq = LASER_REQUESTED_FREQ;
  req.format = AUDIO_F32SYS;
  req.channels = req_channels;
  req.samples = 512;
  req.callback = laser_callback;
  req.userdata = NULL;

  /* Allow the driver to adjust channel count (e.g. macOS rounds to 4) and
   * frequency (so we use the device's native rate rather than resampling). */
  laser_device_id = SDL_OpenAudioDevice(device_name, 0, &req, &given,
                                        SDL_AUDIO_ALLOW_CHANNELS_CHANGE |
                                            SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);

  if (laser_device_id == 0) {
    fprintf(stderr, "laser: couldn't open device '%s': %s\n",
            device_name ? device_name : "(default)", SDL_GetError());
    return;
  }

  laser_channels = given.channels;
  laser_dda_rate = (unsigned)given.freq;

  fprintf(stdout,
          "laser: opened '%s' — %d Hz, %d ch, fmt 0x%04x, "
          "map X=%d Y=%d Z=%d\n",
          device_name ? device_name : "(default)", given.freq, given.channels,
          given.format, laser_channel_map[0], laser_channel_map[1],
          laser_channel_map[2]);

  if (given.channels < req_channels) {
    fprintf(stderr,
            "laser: WARNING — device gave %d channel(s), need %d; "
            "some signals may be missing\n",
            given.channels, req_channels);
  }

  SDL_AtomicSet(&laser_head, 0);
  SDL_AtomicSet(&laser_tail, 0);
  laser_dda_accum = 0;

  SDL_PauseAudioDevice(laser_device_id, 0);
}

void laser_done(void) {
  if (laser_device_id != 0) {
    SDL_CloseAudioDevice(laser_device_id);
    laser_device_id = 0;
  }
}

void laser_push(long x, long y, unsigned blank) {
  int head, next_head, tail;
  float fx, fy, fz;

  if (laser_device_id == 0)
    return;

  /* DDA downsampling: accumulate device_rate per Vectrex tick; emit one
   * sample whenever the accumulator reaches VECTREX_MHZ. */
  laser_dda_accum += laser_dda_rate;
  if (laser_dda_accum < VECTREX_MHZ)
    return;
  laser_dda_accum -= VECTREX_MHZ;

  /* Normalise to [-1.0, +1.0] and clamp (beam can stray out of bounds
   * during repositioning moves). */
  fx = (float)x * (2.0f / ALG_MAX_X) - 1.0f;
  fy = (float)y * (2.0f / ALG_MAX_Y) - 1.0f;
  if (fx < -1.0f)
    fx = -1.0f;
  if (fx > 1.0f)
    fx = 1.0f;
  if (fy < -1.0f)
    fy = -1.0f;
  if (fy > 1.0f)
    fy = 1.0f;

  /* Z: +1.0 = beam on, -1.0 = beam off.
   * Invert the sign here if your DAC-ILDA uses opposite polarity. */
  fz = blank ? 1.0f : -1.0f;

  head = SDL_AtomicGet(&laser_head);
  tail = SDL_AtomicGet(&laser_tail);
  next_head = (head + 1) & LASER_RING_MASK;

  if (next_head == tail)
    return; /* ring full: drop sample rather than blocking */

  laser_ring[head].x = fx;
  laser_ring[head].y = fy;
  laser_ring[head].z = fz;
  SDL_AtomicSet(&laser_head, next_head);
}
