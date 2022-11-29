#include <stdio.h>
#include <stdlib.h>
#include "../utils/bitmap_image.hpp"
#include <sys/time.h>
#include <pthread.h>

#define N 1
#define ENOUGH 100

rgb_t** inImage;
bitmap_image image;
bitmap_image out;
int height, width;

using namespace std;

void loadPixelsToArray(){
    inImage = (rgb_t**) malloc(image.height()*sizeof(rgb_t*));
    
    for (int i = 0; i < image.width(); i++) {
        inImage[i] = (rgb_t*) malloc(image.width()*sizeof(rgb_t));
    }
    
    for (int y = 0; y < image.height(); y++){
        for (int x = 0; x < image.width(); x++){
            image.get_pixel(y, x, inImage[y][x]);
        }
    }
}

int main() {
    time_t time;
    struct timeval t0, t1, t2, t3, t4, t5, t6, start, end;
    float delta;    
    char file_name[ENOUGH], out_file[ENOUGH];

    gettimeofday(&start, NULL);

    for (int pic = 1; pic <= N; pic++) {

        // timp 0
        gettimeofday(&t0, NULL);

        sprintf(file_name, "../img/%d.bmp", pic);
        // citire imagine
        image = bitmap_image(file_name);

        loadPixelsToArray();

        height = image.height();
        width = image.width();
        
        out = bitmap_image(width, height);

        // timp 1
        gettimeofday(&t1, NULL);
        delta = ((t1.tv_sec * 1000000 + t1.tv_usec) -
        (t0.tv_sec * 1000000 + t0.tv_usec));
        printf("citirea imaginii: %f\n", delta);
        

        // filtru 1 = alb-negru
        for (unsigned int i = 1; i < height - 1; ++i) {
            for (unsigned int j = 1; j < width - 1; ++j) {
                unsigned int gray = 0.2126 * inImage[i][j].red +
                                    0.7152 * inImage[i][j].green +
                                    0.0722 * inImage[i][j].blue;
                gray = (gray > 255) ? 255 : gray;
                inImage[i][j].red = gray;
                inImage[i][j].green = gray;
                inImage[i][j].blue = gray;
            }
        }

        // timp 2
        gettimeofday(&t2, NULL);
        delta = ((t2.tv_sec * 1000000 + t2.tv_usec) -
        (t1.tv_sec * 1000000 + t1.tv_usec));
        printf("filtru 1: %f\n", delta);

        // filtru 2 = contrast
        float contrast = 0.5;
        float factor = (259. * (contrast + 255.)) / (255. * (259. - contrast));
        for (unsigned int i = 1; i < height - 1; ++i) {
            for (unsigned int j = 1; j < width - 1; ++j) {
                float tempColor;

                tempColor = factor * (inImage[i][j].red - 128) + 128;
                tempColor = (tempColor < 0) ? 0 : tempColor;
                inImage[i][j].red = (tempColor > 255) ? 255 : tempColor;
                tempColor = factor * (inImage[i][j].green - 128) + 128;
                tempColor = (tempColor < 0) ? 0 : tempColor;
                inImage[i][j].green = (tempColor > 255) ? 255 : tempColor;
                tempColor = factor * (inImage[i][j].blue - 128) + 128;
                tempColor = (tempColor < 0) ? 0 : tempColor;
                inImage[i][j].blue = (tempColor > 255) ? 255 : tempColor;
            }
        }

        // timp 3
        gettimeofday(&t3, NULL);
        delta = ((t3.tv_sec * 1000000 + t3.tv_usec) -
        (t2.tv_sec * 1000000 + t2.tv_usec));
        printf("filtru 2: %f\n", delta);
        

        // filtru 3 = sharpness
        static const float kernel[3][3] = {{0, -2. / 3., 0},
                                        {-2. / 3., 11. / 3., -2. / 3.},
                                        {0, -2. / 3., 0}};

        for ( int i = 1; i < height - 1; ++i) {
            for ( int j = 1; j < width - 1; ++j) {
                float red, green, blue;
                float tempColor;
                red = green = blue = 0;
                
                for (int ki = -1; ki <= 1; ++ki) {
                    for (int kj = -1; kj <= 1; ++kj) {
                        red   += static_cast<float>(inImage[i + ki][j + kj].red) * kernel[ki + 1][kj + 1];
                        green += static_cast<float>(inImage[i + ki][j + kj].green) * kernel[ki + 1][kj + 1];
                        blue  += static_cast<float>(inImage[i + ki][j + kj].blue) * kernel[ki + 1][kj + 1];
                    }
                }

                red = (red < 0) ? 0 : red;
                green = (green < 0) ? 0 : green;
                blue = (blue < 0) ? 0 : blue;
                inImage[i][j].red = (red > 255) ? 255 : red;
                inImage[i][j].green = (green > 255) ? 255 : green;
                inImage[i][j].blue = (blue > 255) ? 255 : blue;
            }
        }

         // timp 4
        gettimeofday(&t4, NULL);
        delta = ((t4.tv_sec * 1000000 + t4.tv_usec) -
        (t3.tv_sec * 1000000 + t3.tv_usec));
        printf("filtru 3: %f\n", delta);

        // filtru 4 = blur
        for (int y = 0; y < height; y++){
            for (int x = 0; x < width; x++){
                int red = 0;
                int green = 0;
                int blue = 0;
                int neighbors = 0;
                rgb_t colour;
                
                for (int i = x - 1; i <= x + 1; i++) {
                    for (int j = y - 1; j <= y + 1; j++) {
                        if (i >= 0 && j >= 0 && i < width && j < height) {
                            neighbors++;
                            colour = inImage[i][j];
                            
                            red   += colour.red;
                            green += colour.green;
                            blue  += colour.blue;
                        }
                    }
                }
                red   /= neighbors;
                green /= neighbors;
                blue  /= neighbors;
                colour = make_colour(red, green, blue);
                out.set_pixel(x, y, colour);
            }
        }

        // timp 5
        gettimeofday(&t5, NULL);
        delta = ((t5.tv_sec * 1000000 + t5.tv_usec) -
        (t4.tv_sec * 1000000 + t4.tv_usec));
        printf("filtru 4: %f\n", delta);

        sprintf(out_file, "../img/out/out_%d.bmp", pic);
        // scriere imagine
        out.save_image(out_file);

        // timp 6
        gettimeofday(&t6, NULL);
        delta = ((t6.tv_sec * 1000000 + t6.tv_usec) -
        (t5.tv_sec * 1000000 + t5.tv_usec));
        // printf("scriere: %f\n", delta);

    }

    gettimeofday(&end, NULL);
    delta = ((end.tv_sec * 1000000 + end.tv_usec) -
    (start.tv_sec * 1000000 + start.tv_usec));
    printf("%f\n", delta);

    return 0;
}
