
#include <stdlib.h>
#include <string.h>

#include "SDL.h"
#include "SDL2_gfxPrimitives.h"
#include "SDL_image.h"

#include "e8910.h"
#include "laser.h"
#include "osint.h"
#include "vecx.h"

#define EMU_TIMER 20 /* the emulators heart beats at 20 milliseconds */

static SDL_Window *screen = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *overlay = NULL;

static long scl_factor;
static long offx;
static long offy;

void osint_render(void) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  int v;
  for (v = 0; v < vector_draw_cnt; v++) {
    Uint8 c = vectors_draw[v].color * 256 / VECTREX_COLORS;
    aalineRGBA(renderer, offx + vectors_draw[v].x0 / scl_factor,
               offy + vectors_draw[v].y0 / scl_factor,
               offx + vectors_draw[v].x1 / scl_factor,
               offy + vectors_draw[v].y1 / scl_factor, c, c, c, 0xff);
  }
  if (overlay) {
    SDL_Rect dest_rect = {offx, offy, ((double)ALG_MAX_X / (double)scl_factor),
                          ((double)ALG_MAX_Y / (double)scl_factor)};
    SDL_RenderCopy(renderer, overlay, NULL, &dest_rect);
  }
  SDL_RenderPresent(renderer);
}

static void init(const char *romfilename, const char *cartfilename) {
  FILE *f;
  if (!(f = fopen(romfilename, "rb"))) {
    perror(romfilename);
    exit(EXIT_FAILURE);
  }
  if (fread(rom, 1, sizeof(rom), f) != sizeof(rom)) {
    printf("Invalid rom length\n");
    exit(EXIT_FAILURE);
  }
  fclose(f);

  memset(cart, 0, sizeof(cart));
  if (cartfilename) {
    FILE *f;
    if (!(f = fopen(cartfilename, "rb"))) {
      perror(cartfilename);
      exit(EXIT_FAILURE);
    }
    fread(cart, 1, sizeof(cart), f);
    fclose(f);
  }
}

void resize(int width, int height) {
  long sclx, scly;

  long screenx = width;
  long screeny = height;

  sclx = ALG_MAX_X / width;
  scly = ALG_MAX_Y / height;

  scl_factor = sclx > scly ? sclx : scly;

  offx = (screenx - ALG_MAX_X / scl_factor) / 2;
  offy = (screeny - ALG_MAX_Y / scl_factor) / 2;
}

static void readevents() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
    case SDL_QUIT:
      exit(EXIT_SUCCESS);
      break;
    case SDL_WINDOWEVENT:
      switch (e.window.event) {
      case SDL_WINDOWEVENT_RESIZED:
        resize(e.window.data1, e.window.data2);
        break;
      case SDL_WINDOWEVENT_SIZE_CHANGED:
        resize(e.window.data1, e.window.data2);
        break;
      }
      break;
    case SDL_KEYDOWN:
      switch (e.key.keysym.sym) {
      case SDLK_ESCAPE:
        exit(EXIT_SUCCESS);
      case SDLK_a:
        snd_regs[14] &= ~0x01;
        break;
      case SDLK_s:
        snd_regs[14] &= ~0x02;
        break;
      case SDLK_d:
        snd_regs[14] &= ~0x04;
        break;
      case SDLK_f:
        snd_regs[14] &= ~0x08;
        break;
      case SDLK_LEFT:
        alg_jch0 = 0x00;
        break;
      case SDLK_RIGHT:
        alg_jch0 = 0xff;
        break;
      case SDLK_UP:
        alg_jch1 = 0xff;
        break;
      case SDLK_DOWN:
        alg_jch1 = 0x00;
        break;
      default:
        break;
      }
      break;
    case SDL_KEYUP:
      switch (e.key.keysym.sym) {
      case SDLK_a:
        snd_regs[14] |= 0x01;
        break;
      case SDLK_s:
        snd_regs[14] |= 0x02;
        break;
      case SDLK_d:
        snd_regs[14] |= 0x04;
        break;
      case SDLK_f:
        snd_regs[14] |= 0x08;
        break;
      case SDLK_LEFT:
        alg_jch0 = 0x80;
        break;
      case SDLK_RIGHT:
        alg_jch0 = 0x80;
        break;
      case SDLK_UP:
        alg_jch1 = 0x80;
        break;
      case SDLK_DOWN:
        alg_jch1 = 0x80;
        break;
      default:
        break;
      }
      break;
    default:
      break;
    }
  }
}

void osint_emuloop() {
  Uint32 next_time = SDL_GetTicks() + EMU_TIMER;
  vecx_reset();
  for (;;) {
    vecx_emu((VECTREX_MHZ / 1000) * EMU_TIMER);
    readevents();

    {
      Uint32 now = SDL_GetTicks();
      if (now < next_time)
        SDL_Delay(next_time - now);
      else
        next_time = now;
      next_time += EMU_TIMER;
    }
  }
}

struct cli_options {
  const char *rom;
  const char *cart;
  const char *overlay;
  const char *device;
  int audio_l_ch;
  int audio_r_ch;
  int laser_x_ch;
  int laser_y_ch;
  int laser_z_ch;
  int laser_flip_x;
  int laser_flip_y;
  int list_audio_devices;
};

static void usage(const char *prog, int exitcode) {
  fprintf(
      exitcode ? stderr : stdout,
      "Usage: %s [OPTIONS]\n"
      "\n"
      "Options:\n"
      "  --rom FILE             ROM file to load (default: rom.dat)\n"
      "  --cart FILE            Cartridge ROM file\n"
      "  --overlay FILE         Overlay BMP image file\n"
      "  --device DEV           Audio output device (default: system default)\n"
      "  --audio-l NUM          PSG left  channel index (default: 0)\n"
      "  --audio-r NUM          PSG right channel index (default: 1)\n"
      "  --laser-x NUM          Laser X channel index (default: 2)\n"
      "  --laser-y NUM          Laser Y channel index (default: 3)\n"
      "  --laser-z NUM          Laser Z/blank channel index (default: 4)\n"
      "  --laser-flip-x         Invert the laser X axis\n"
      "  --laser-flip-y         Invert the laser Y axis\n"
      "  --list-audio-devices   List available audio output devices and exit\n"
      "  --help                 Show this help and exit\n",
      prog);
  exit(exitcode);
}

static const char *require_arg(const char *name, int *i, int argc, char **argv,
                               const char *inline_val) {
  if (inline_val)
    return inline_val;
  if (*i + 1 >= argc) {
    fprintf(stderr, "%s: option '--%s' requires an argument\n", argv[0], name);
    usage(argv[0], 1);
  }
  return argv[++(*i)];
}

static void parse_cli_options(int argc, char **argv, struct cli_options *opts) {
  opts->rom = "rom.dat";
  opts->cart = NULL;
  opts->overlay = NULL;
  opts->device = NULL;
  opts->audio_l_ch = 0;
  opts->audio_r_ch = 1;
  opts->laser_x_ch = 2;
  opts->laser_y_ch = 3;
  opts->laser_z_ch = 4;
  opts->laser_flip_x = 0;
  opts->laser_flip_y = 0;
  opts->list_audio_devices = 0;

  for (int i = 1; i < argc; i++) {
    const char *arg = argv[i];
    if (strncmp(arg, "--", 2) != 0) {
      fprintf(stderr, "%s: unexpected argument '%s'\n", argv[0], arg);
      usage(argv[0], 1);
    }
    const char *name = arg + 2;
    const char *val = NULL;
    char name_buf[64];
    const char *eq = strchr(name, '=');
    if (eq) {
      size_t len = (size_t)(eq - name);
      if (len >= sizeof(name_buf)) {
        fprintf(stderr, "%s: unknown option: %s\n", argv[0], arg);
        usage(argv[0], 1);
      }
      memcpy(name_buf, name, len);
      name_buf[len] = '\0';
      name = name_buf;
      val = eq + 1;
    }
    if (strcmp(name, "rom") == 0) {
      opts->rom = require_arg(name, &i, argc, argv, val);
    } else if (strcmp(name, "cart") == 0) {
      opts->cart = require_arg(name, &i, argc, argv, val);
    } else if (strcmp(name, "overlay") == 0) {
      opts->overlay = require_arg(name, &i, argc, argv, val);
    } else if (strcmp(name, "device") == 0) {
      opts->device = require_arg(name, &i, argc, argv, val);
    } else if (strcmp(name, "audio-l") == 0) {
      opts->audio_l_ch = atoi(require_arg(name, &i, argc, argv, val));
    } else if (strcmp(name, "audio-r") == 0) {
      opts->audio_r_ch = atoi(require_arg(name, &i, argc, argv, val));
    } else if (strcmp(name, "laser-x") == 0) {
      opts->laser_x_ch = atoi(require_arg(name, &i, argc, argv, val));
    } else if (strcmp(name, "laser-y") == 0) {
      opts->laser_y_ch = atoi(require_arg(name, &i, argc, argv, val));
    } else if (strcmp(name, "laser-z") == 0) {
      opts->laser_z_ch = atoi(require_arg(name, &i, argc, argv, val));
    } else if (strcmp(name, "laser-flip-x") == 0) {
      opts->laser_flip_x = 1;
    } else if (strcmp(name, "laser-flip-y") == 0) {
      opts->laser_flip_y = 1;
    } else if (strcmp(name, "list-audio-devices") == 0) {
      opts->list_audio_devices = 1;
    } else if (strcmp(name, "help") == 0) {
      usage(argv[0], 0);
    } else {
      fprintf(stderr, "%s: unknown option: %s\n", argv[0], arg);
      usage(argv[0], 1);
    }
  }
}

static void list_audio_devices(void) {
  int n = SDL_GetNumAudioDevices(0 /* output */);
  if (n < 0) {
    fprintf(stderr, "list-audio-devices: SDL error: %s\n", SDL_GetError());
    return;
  }
  fprintf(stdout, "%-4s  %-6s  %-8s  %s\n", "#", "Rate", "Channels", "Name");
  fprintf(stdout, "----  ------  --------  ----\n");
  for (int i = 0; i < n; i++) {
    const char *name = SDL_GetAudioDeviceName(i, 0);
    SDL_AudioSpec spec;
    if (SDL_GetAudioDeviceSpec(i, 0, &spec) == 0) {
      fprintf(stdout, "%-4d  %-6d  %-8d  %s\n", i, spec.freq, spec.channels,
              name);
    } else {
      fprintf(stdout, "%-4d  %-6s  %-8s  %s (could not query: %s)\n", i, "?",
              "?", name, SDL_GetError());
    }
  }
}

void load_overlay(const char *filename) {
  SDL_Surface *image;
  image = SDL_LoadBMP(filename);
  if (image) {
    overlay = SDL_CreateTextureFromSurface(renderer, image);
    SDL_FreeSurface(image);
  } else {
    fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
  }
}

int main(int argc, char *argv[]) {
  struct cli_options opts;
  parse_cli_options(argc, argv, &opts);

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
    fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
    exit(-1);
  }

  if (opts.list_audio_devices) {
    list_audio_devices();
    SDL_Quit();
    return 0;
  }
  SDL_CreateWindowAndRenderer(330 * 3 / 2, 410 * 3 / 2, SDL_WINDOW_RESIZABLE,
                              &screen, &renderer);
  if (screen == NULL || renderer == NULL) {
    fprintf(stderr, "Failed to initialize SDL window/renderer: %s\n",
            SDL_GetError());
    exit(-2);
  }

  resize(330 * 3 / 2, 410 * 3 / 2);

  if (opts.overlay)
    load_overlay(opts.overlay);

  init(opts.rom, opts.cart);

  e8910_init();
  laser_init(opts.device, opts.audio_l_ch, opts.audio_r_ch, opts.laser_x_ch,
             opts.laser_y_ch, opts.laser_z_ch, opts.laser_flip_x,
             opts.laser_flip_y);
  osint_emuloop();
  laser_done();
  SDL_Quit();

  return 0;
}
