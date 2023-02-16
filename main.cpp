#include <stdio.h>
#include <memory>
#include <math.h>

#include "image.h"

#define HELP "The format is: ImageScaler <src> <dest> <scale_x> <scale_y>\nMin scale factor: 0.01, Max scale factor: 50\n"

int main(int argc, char * argv[]) {
  if (argc < 5 || argc > 5) {
    printf(HELP);
    return 1;
  }

  ImageScaler::Image image(argv[1]);

  double scale_factor_x = strtod(argv[3], nullptr);
  double scale_factor_y = strtod(argv[4], nullptr);

  if (!(scale_factor_x > 1.0 && scale_factor_x < 50.0 || scale_factor_y > 1.0 && scale_factor_y < 50.0)) {
    printf("The scale factor is not a valid value or not a valid double.\n");
    printf(HELP);
    return 1;
  }

  ImageScaler::Image image2(image.get_width() * scale_factor_x, image.get_height() * scale_factor_y);

  for (int y = 0; y < image.get_height() * scale_factor_y; y++) {
    for (int x = 0; x < image.get_width() * scale_factor_x; x++) {
      ImageScaler::Color interpolated_color = image.get_pixel_bicubic((double)x * (1/scale_factor_x), (double)y * (1/scale_factor_y));
      image2.set_pixel(interpolated_color, x, y);
    }
  }

  image2.save_image(argv[2]);
  return 0;
}
