#ifndef IMAGE_HH
#define IMAGE_H

#include <cstdio>
#include <png.h>
#include <memory>
#include <cmath>

// Creates an editable Image Object.
namespace ImageScaler {
    typedef struct Color {
        double r;
        double g;
        double b;
    } Color;

    class Image {
        public:
            static constexpr Color DEFAULT_COLOR = {0, 0, 0};

            Image(int height, int width);
            Image(Color fill, int height, int width);
            Image(const char* path);
            int load_image(const char* path);
            int save_image(const char* path);
            Color get_pixel(int x, int y);
            int set_pixel(Color value, int x, int y);
            unsigned char* get_raw_data();
            int get_height();
            int get_width();
            Color get_pixel_bicubic(double x, double y);


        private:
            double* data;
            int height;
            int width;

            double cubic_interpolation(double p[4], double x);
            double bicubic_interpolation(double p[4][4], double x, double y);
            void basic_construct(Color fill, int height, int width);
    };
}
#endif
