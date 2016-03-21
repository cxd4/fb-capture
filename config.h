#ifndef _CONFIG_H_
#define _CONFIG_H_

/*
 * Pass -DFILEPATH="/home/myusername/" or the like in GCC flags.
 * Otherwise, the below test has the current directory as the default.
 */
#ifndef FILEPATH
#define FILEPATH        "./"
#endif

/*
 * My monitor's current configuration is 1680x1050 (real FB width = 1728).
 * Don't like it?  Change it--either below or in make.sh.
 */
#ifndef WIDTH
#define WIDTH           1024
#endif
#ifndef HEIGHT
#define HEIGHT          768
#endif

#if (WIDTH > 0xFFFFu) || (HEIGHT > 0xFFFFu)
#error Bitmap dimensions exceed the 16-bit storage limit of older BMP files.
#endif

static int dump_framebuffer(int device_ID);
static int correct_framebuffer(unsigned long screen_ID);

#endif
