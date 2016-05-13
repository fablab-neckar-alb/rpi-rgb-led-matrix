// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Small example how write text.
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "led-matrix.h"
#include "graphics.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <stropts.h>
using namespace rgb_matrix;

Color convertangletocolor(int angle);
static int usage(const char *progname) {
  fprintf(stderr, "usage: %s [options]\n", progname);
  fprintf(stderr, "Reads text from stdin and displays it. "
          "Empty string: clear screen\n");
  fprintf(stderr, "Options:\n"
          "\t-f <font-file>: Use given font.\n"
          "\t-r <rows>     : Display rows. 16 for 16x32, 32 for 32x32. "
          "Default: 32\n"
          "\t-P <parallel> : For Plus-models or RPi2: parallel chains. 1..3. "
          "Default: 1\n"
          "\t-c <chained>  : Daisy-chained boards. Default: 1.\n"
          "\t-b <brightness>: Sets brightness percent. Default: 100.\n"
          "\t-x <x-origin> : X-Origin of displaying text (Default: 0)\n"
          "\t-y <y-origin> : Y-Origin of displaying text (Default: 0)\n"
          "\t-C <r,g,b>    : Color. Default 255,255,0\n");
  return 1;
}

static bool parseColor(Color *c, const char *str) {
  return sscanf(str, "%hhu,%hhu,%hhu", &c->r, &c->g, &c->b) == 3;
}

int main(int argc, char *argv[]) {
  Color color(255, 255, 0);
  const char *bdf_font_file = NULL;
  int rows = 32;
  int chain = 1;
  int parallel = 1;
  int x_orig = 0;
  int y_orig = -1;
  int brightness = 100;

  int opt;
  while ((opt = getopt(argc, argv, "r:P:c:x:y:f:C:b:")) != -1) {
    switch (opt) {
    case 'r': rows = atoi(optarg); break;
    case 'P': parallel = atoi(optarg); break;
    case 'c': chain = atoi(optarg); break;
    case 'b': brightness = atoi(optarg); break;
    case 'x': x_orig = atoi(optarg); break;
    case 'y': y_orig = atoi(optarg); break;
    case 'f': bdf_font_file = strdup(optarg); break;
    case 'C':
      if (!parseColor(&color, optarg)) {
        fprintf(stderr, "Invalid color spec.\n");
        return usage(argv[0]);
      }
      break;
    default:
      return usage(argv[0]);
    }
  }

  if (bdf_font_file == NULL) {
    fprintf(stderr, "Need to specify BDF font-file with -f\n");
    return usage(argv[0]);
  }

  /*
   * Load font. This needs to be a filename with a bdf bitmap font.
   */
  rgb_matrix::Font font;
  if (!font.LoadFont(bdf_font_file)) {
    fprintf(stderr, "Couldn't load font '%s'\n", bdf_font_file);
    return usage(argv[0]);
  }

  if (rows != 8 && rows != 16 && rows != 32 && rows != 64) {
    fprintf(stderr, "Rows can one of 8, 16, 32 or 32 "
            "for 1:4, 1:8, 1:16 and 1:32 multiplexing respectively.\n");
    return 1;
  }

  if (chain < 1) {
    fprintf(stderr, "Chain outside usable range\n");
    return 1;
  }
  if (chain > 8) {
    fprintf(stderr, "That is a long chain. Expect some flicker.\n");
  }
  if (parallel < 1 || parallel > 3) {
    fprintf(stderr, "Parallel outside usable range.\n");
    return 1;
  }
  if (brightness < 1 || brightness > 100) {
    fprintf(stderr, "Brightness is outside usable range.\n");
    return 1;
  }

  /*
   * Set up GPIO pins. This fails when not running as root.
   */
  GPIO io;
  if (!io.Init())
    return 1;

  /*
   * Set up the RGBMatrix. It implements a 'Canvas' interface.
   */
  RGBMatrix *canvas = new RGBMatrix(&io, rows, chain, parallel);
  canvas->SetBrightness(brightness);

  bool all_extreme_colors = brightness == 100;
  all_extreme_colors &= color.r == 0 || color.r == 255;
  all_extreme_colors &= color.g == 0 || color.g == 255;
  all_extreme_colors &= color.b == 0 || color.b == 255;
  //if (all_extreme_colors)
  //  canvas->SetPWMBits(1);

  int x = x_orig;
  int y = y_orig;

  if (isatty(STDIN_FILENO)) {
    // Only give a message if we are interactive. If connected via pipe, be quiet
    printf("Enter lines. Full screen or empty line clears screen.\n"
           "Supports UTF-8. CTRL-D for exit.\n");
  }
  char ndline[1024] = "CC2016 Darmstadt";
  char line[1024] = "Energies Technologies";

    int angle = 0;
  //while (fgets(line, sizeof(line), stdin)) {
  while (1) {
    
    y = 2;
    struct pollfd fds; 
    int ret;
    fds.fd = 0; /* this is STDIN */
    fds.events = POLLIN;
    ret = poll(&fds, 1, 0);
    int max = 0;
    if(ret == 1)
    {
      // we have exactly n bytes to read 
      strcpy(ndline,line);   
      fgets(line, sizeof(line), stdin);
    const size_t last = strlen(line);

    if (last > 0) line[last - 1] = '\0';  // remove newline.
    }
    canvas->Clear();
    angle++;
    Color c = convertangletocolor(angle); 
    Color ndc = convertangletocolor(angle+180); 
    int last = rgb_matrix::DrawText(canvas, font, x, y + font.baseline(), c, line);
    y += 2+ font.height();
    int ndlast = rgb_matrix::DrawText(canvas, font, x, y +  font.baseline(), ndc, ndline);
     if ( last > ndlast)
    {
       max=last;
    }
    else
    {
       max=ndlast;
    }
    x-=1;
    if(x < - (max))
    {
       x=128;
    }
    else{ max = ndlast;}
    usleep(20000);
  }
  // Finished. Shut down the RGB matrix.
  canvas->Clear();
  delete canvas;

  return 0;
}
Color convertangletocolor(int angle)
    {
                angle%=360;
		unsigned char seg = angle / 60;
		unsigned int f = (angle % 60) * 255 /59 ;
		unsigned char t = 255 - f;
       //         printf("seg = %d f= %d t= %d\n" , seg, f, t); 
                int r, g, b;
		switch (seg)
		{
		case 0:
		default:
			r = 255;
			g = f;
			b = 0;
			break;

		case 1:
			r = t;
			g = 255;
			b = 0;
			break;
		case 2:
			r = 0;
			g = 255;
			b = f;
			break;
		case 3:
			r = 0;
			g = t;
			b = 255;
			break;
		case 4:
			r = f;
			g = 0;
			b = 255;
			break;
		case 5:

			r = 255;
			g = 0;
			b = t;
			break;
		}
                Color ret(r,g,b);
                return ret;
     	
}
