#include "image.h"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <png.h>
#include <pngconf.h>

#define POSITION(x, y, width, offset)  ((y) * (width) * 3 + ((x) + (offset)))
#define POSITION_X(x, y, width, offset)  ((y) * (width) * 3 + ((x) * 3 + (offset)))
#define POSITION_CHECK(x, y, height, width)  ((y >= height || x >= width || y < 0 || x < 0))

namespace ImageScaler {
    Image::Image(int height, int width) {
        this->basic_construct(DEFAULT_COLOR, height, width);
    }

    Image::Image(Color fill, int height, int width) {
        this->basic_construct(fill, height, width);
    }

    Image::Image(const char* path){
        int imageSuccess = this->load_image(path);
        if (imageSuccess != 0) {
            this->basic_construct(DEFAULT_COLOR, 1, 1);
        }
    }

    void Image::basic_construct(Color fill, int height, int width) {
        data = (double*)malloc(sizeof(double) * height * width * 3);

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width * 3; x += 3) {
                data[POSITION(x, y, width, 0)] = fill.r;
                data[POSITION(x, y, width, 1)] = fill.g;
                data[POSITION(x, y, width, 2)] = fill.b;
            }
        }

        this->height = height;
        this->width = width;
    }

    /**
     * @brief Loads a PNG file into a empty canvas.
     * 1 = not a valid path
     * 2 = can't read the file
     * 3 = not a png file.
     * 
     * @param path The file path for the PNG image.
     * @return an int code of whether it successfully loaded.
     */
    int Image::load_image(const char* path) {
        FILE* fd;

        fd = fopen(path, "rb");
        if (fd == nullptr) {
            return 1;
        }
        unsigned char header[8];

        if (fread(header, 1, 8, fd) != 8) {
            return 2;
        }
        
        if (png_sig_cmp(header, 0, 8) != 0) {
            return 3;
        }

        png_structp imagep = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop infop = png_create_info_struct(imagep);
        png_init_io(imagep, fd);
        png_set_sig_bytes(imagep, 8);
        png_read_info(imagep, infop);

        width = png_get_image_width(imagep, infop);
        height = png_get_image_height(imagep, infop);

        int rowbytes = png_get_rowbytes(imagep, infop);

        png_bytepp row_ptr = (png_bytepp)malloc(sizeof(png_bytep) * height);
        for (int y = 0; y < height; y++) {
            row_ptr[y] = (png_bytep)malloc(rowbytes);
        }

        png_read_image(imagep, row_ptr);

        data = (double*)malloc(sizeof(double) * height * width * 3);
        for (int y = 0; y < height; y++) {
            png_bytep row = row_ptr[y];
            for (int x = 0; x < width * 3; x += 3) {
                double r = (double)(row[x])/255;
                double g = (double)(row[x + 1])/255;
                double b = (double)(row[x + 2])/255;

                data[POSITION(x, y, width, 0)] = r;
                data[POSITION(x, y, width, 1)] = g;
                data[POSITION(x, y, width, 2)] = b;
            }
        }

        free(row_ptr);
        fclose(fd);
        return 0;
    }

    int Image::save_image(const char* path) {
       FILE *fd = fopen(path, "wb");
       if (fd == nullptr) {
           return 1;
       }

       png_structp imagep = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
       if (!imagep)
           return 2;

       png_infop info_ptr = png_create_info_struct(imagep);
       if (!info_ptr) {
           png_destroy_write_struct(&imagep, (png_infopp)NULL);
           return 3;
       }

       png_init_io(imagep, fd);

       png_set_IHDR(imagep, info_ptr, (png_uint_32)width, (png_uint_32)height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
       // Load up the image data.

       png_bytepp row_ptr = (png_bytepp)malloc(sizeof(png_bytep) * height);
       for (int y = 0; y < height; y++) {
           png_bytep row = (png_bytep)malloc(sizeof(unsigned char) * 3 * width);
           for (int x = 0; x < width * 3; x += 3) {
               double r = data[POSITION(x, y, width, 0)];
               double g = data[POSITION(x, y, width, 1)];
               double b = data[POSITION(x, y, width, 2)];

               unsigned char converted_r = (unsigned char)(static_cast<int>(std::floor(r * 255.0)));
               unsigned char converted_g = (unsigned char)(static_cast<int>(std::floor(g * 255.0)));
               unsigned char converted_b = (unsigned char)(static_cast<int>(std::floor(b * 255.0)));

               row[x + 0] = converted_r;
               row[x + 1] = converted_g;
               row[x + 2] = converted_b;
           }

           row_ptr[y] = row;
       }
       png_set_rows(imagep, info_ptr, row_ptr);
       png_write_png(imagep, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
       png_write_end(imagep, info_ptr);

       png_destroy_write_struct(&imagep, &info_ptr);
    }

    Color Image::get_pixel(int x, int y) {
        if (POSITION_CHECK(x, y, height, width)) {
            return DEFAULT_COLOR;
        } else {
            Color out_color;
            out_color.r = data[POSITION_X(x, y, width, 0)];
            out_color.g = data[POSITION_X(x, y, width, 1)];
            out_color.b = data[POSITION_X(x, y, width, 2)];

            return out_color;
        }
    }

    Color Image::get_pixel_bicubic(double x, double y) {
        int nearest_x = (int)(floor(x));
        int nearest_y = (int)(floor(y));

        double local_x = x - (double)(nearest_x);
        double local_y = y - (double)(nearest_y);

        // This use of alloca is acceptable (I don't think a 48 element 2d array 
        // is going to cause a stack overflow.), though not ideal...
        // Ideally, this would be statically allocated.

        // Really avoid pointers unless you have to use them.
        // I am only using them because I am an idiot.

        // Anyways, this creates a 4x4 matrix.
        double** rp = (double **)alloca(sizeof(double *) * 4);
        double** gp = (double **)alloca(sizeof(double *) * 4);
        double** bp = (double **)alloca(sizeof(double *) * 4);

        for (int i = 0; i < 4; i++) {
            rp[i] = (double *)alloca(sizeof(double) * 4);
            gp[i] = (double *)alloca(sizeof(double) * 4);
            bp[i] = (double *)alloca(sizeof(double) * 4);
        }

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                int test_x = nearest_x+(i-1);
                int test_y = nearest_y+(j-1);

                if (POSITION_CHECK(test_x, test_y, height, width)) {
                    rp[i][j] = 0;
                    gp[i][j] = 0;
                    bp[i][j] = 0;
                } else {
                    rp[i][j] = data[POSITION_X(test_x, test_y, width, 0)];
                    gp[i][j] = data[POSITION_X(test_x, test_y, width, 1)];
                    bp[i][j] = data[POSITION_X(test_x, test_y, width, 2)];
                }
            }
        }

        // This is a hack, I should really make a clamp function. Not use fmax fmin.
        // This was used to fix a bug.
        double r = fmax(fmin(bicubic_interpolation(rp, local_x, local_y), 1.0), 0.0);
        double g = fmax(fmin(bicubic_interpolation(gp, local_x, local_y), 1.0), 0.0);
        double b = fmax(fmin(bicubic_interpolation(bp, local_x, local_y), 1.0), 0.0);

        Color out_color;
        out_color.r = r;
        out_color.g = g;
        out_color.b = b;

        return out_color;
    }

    double Image::cubic_interpolation(double* p, double x) {
        double a = (-0.5 * p[0]) + (1.5 * p[1]) + (-1.5 * p[2]) + (0.5 * p[3]);
        double b = p[0] + (-2.5 * p[1]) + (2 * p[2]) + (-0.5 * p[3]);
        double c = -0.5 * p[0] + 0.5 * p[2];
        double d = p[1];

        return (a * (x * x * x)) + (b * (x * x)) + (c * (x)) + d;
    }

    double Image::bicubic_interpolation(double** p, double x, double y) {
        double results[4];

        
        results[0] = cubic_interpolation(p[0], y);
        results[1] = cubic_interpolation(p[1], y);
        results[2] = cubic_interpolation(p[2], y);
        results[3] = cubic_interpolation(p[3], y);

        return cubic_interpolation(results, x);
    }

    int Image::set_pixel(Color value, int x, int y) {
        if (POSITION_CHECK(x, y, height, width)) {
            return 1; // Error
        } else {
            data[y * width * 3 + ((x * 3))] = value.r;
            data[y * width * 3 + ((x * 3) + 1)] = value.g;
            data[y * width * 3 + ((x * 3) + 2)] = value.b;

            return 0;
        }
    }

    unsigned char* Image::get_raw_data() {
        unsigned char* data = (unsigned char*)malloc(sizeof(unsigned char) * height * width * 3);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width * 3; x += 3) {
                data[y * width * 3 + (x)] = (unsigned char)(this->data[y * width * 3 + (x)] * 255);
                data[y * width * 3 + (x + 1)] = (unsigned char)(this->data[y * width * 3 + (x + 1)] * 255);
                data[y * width * 3 + (x + 2)] = (unsigned char)(this->data[y * width * 3 + (x + 2)] * 255);
            }
        }

        return data;
    }

    int Image::get_height() {
        return height;
    }

    int Image::get_width() {
        return width;
    }
}
