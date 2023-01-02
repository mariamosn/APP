#include "../../utils/bitmap_image.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string>
#include <sys/time.h>

#define NUM_THREADS 3
#define N 500
#define ENOUGH 100

bitmap_image out;
bitmap_image image1;
rgb_t **in_image;

pthread_barrier_t barrier;

struct args
{
    int height;
    int width;
    int thread_id;
    int h_start;
    int h_stop;
};

void loadPixelsToArray(rgb_t **inImage, bitmap_image image)
{

    for (int i = 0; i < image.height(); i++)
    {
        inImage[i] = (rgb_t *)malloc(image.width() * sizeof(rgb_t));
    }

    for (int y = 0; y < image.height(); y++)
    {
        for (int x = 0; x < image.width(); x++)
        {
            image.get_pixel(y, x, inImage[y][x]);
        }
    }
}

inline unsigned long thread_max(unsigned long a, unsigned long b)
{
    return (a > b) ? a : b;
}

void filter_sharpness(void *args)
{
    static const float kernel[3][3] = {{0, -2. / 3., 0},
                                       {-2. / 3., 11. / 3., -2. / 3.},
                                       {0, -2. / 3., 0}};

    struct args params = *((struct args *)args);

    int start = params.h_start;
    int stop = params.h_stop;

    if (params.h_stop == params.height)
        stop--;

    if (params.h_start == 0)
        start++;

    for (int i = start; i < stop; ++i)
    {
        for (unsigned int j = 1; j < params.width - 1; ++j)
        {
            float red, green, blue;
            float tempColor;
            red = green = blue = 0;

            for (int ki = -1; ki <= 1; ++ki)
            {
                for (int kj = -1; kj <= 1; ++kj)
                {
                    red += static_cast<float>(in_image[i + ki][j + kj].red) * kernel[ki + 1][kj + 1];
                    green += static_cast<float>(in_image[i + ki][j + kj].green) * kernel[ki + 1][kj + 1];
                    blue += static_cast<float>(in_image[i + ki][j + kj].blue) * kernel[ki + 1][kj + 1];
                }
            }
            red = (red < 0) ? 0 : red;
            green = (green < 0) ? 0 : green;
            blue = (blue < 0) ? 0 : blue;
            in_image[i][j].red = (red > 255) ? 255 : red;
            in_image[i][j].green = (green > 255) ? 255 : green;
            in_image[i][j].blue = (blue > 255) ? 255 : blue;
        }
    }
}

void filter_contrast(void *args)
{
    struct args params = *((struct args *)args);

    float contrast = 0.5;
    float factor = (259. * (contrast + 255.)) / (255. * (259. - contrast));

    for (unsigned int i = params.h_start; i < params.h_stop; ++i)
    {
        for (unsigned int j = 0; j < params.width; ++j)
        {
            float tempColor;

            tempColor = factor * (in_image[i][j].red - 128) + 128;
            tempColor = (tempColor < 0) ? 0 : tempColor;
            in_image[i][j].red = (tempColor > 255) ? 255 : tempColor;
            tempColor = factor * (in_image[i][j].green - 128) + 128;
            tempColor = (tempColor < 0) ? 0 : tempColor;
            in_image[i][j].green = (tempColor > 255) ? 255 : tempColor;
            tempColor = factor * (in_image[i][j].blue - 128) + 128;
            tempColor = (tempColor < 0) ? 0 : tempColor;
            in_image[i][j].blue = (tempColor > 255) ? 255 : tempColor;
        }
    }
}

void filter_black_white(void *args)
{
    struct args params = *((struct args *)args);

    for (unsigned int i = params.h_start; i < params.h_stop; ++i)
    {
        for (unsigned int j = 1; j < params.width - 1; ++j)
        {
            unsigned int gray = 0.2126 * in_image[i][j].red +
                                0.7152 * in_image[i][j].green +
                                0.0722 * in_image[i][j].blue;
            gray = (gray > 255) ? 255 : gray;
            in_image[i][j].red = gray;
            in_image[i][j].green = gray;
            in_image[i][j].blue = gray;
        }
    }
}

void filter_blur(void *args)
{
    struct args params = *((struct args *)args);
    for (int y = params.h_start; y < params.h_stop; y++)
    {
        for (int x = 0; x < params.width; x++)
        {
            int red = 0;
            int green = 0;
            int blue = 0;
            int neighbors = 0;
            rgb_t colour;

            for (int i = x - 1; i <= x + 1; i++)
            {
                for (int j = y - 1; j <= y + 1; j++)
                {
                    if (i >= 0 && j >= params.h_start && i < params.width && j < params.h_stop)
                    {
                        neighbors++;
                        colour = in_image[i][j];

                        red += colour.red;
                        green += colour.green;
                        blue += colour.blue;
                    }
                }
            }

            red /= neighbors;
            green /= neighbors;
            blue /= neighbors;
            colour = make_colour(red, green, blue);
            out.set_pixel(x, y, colour);
        }
    }
}

void *apply_filters(void *args)
{
    // filter 1
    filter_black_white(args);
    pthread_barrier_wait(&barrier);
    // filter 2
    filter_contrast(args);
    pthread_barrier_wait(&barrier);
    // filter 3
    filter_sharpness(args);
    pthread_barrier_wait(&barrier);
    // filter 4
    filter_blur(args);
    pthread_barrier_wait(&barrier);
    return NULL;
}

int main(int argc, const char *argv[])
{
    pthread_t thread[NUM_THREADS];
    pthread_attr_t attr;
    char file_name[ENOUGH], out_file[ENOUGH];

    struct timeval start_time, end_time;
    double seq_time;
    bitmap_image image;
    int height, width;

    pthread_barrier_init(&barrier, NULL, NUM_THREADS);

    gettimeofday(&start_time, NULL);

    for (int pic = 1; pic <= N; pic++)
    {
        sprintf(file_name, "../../img/%d.bmp", pic);

        // citire imagine
        image = bitmap_image(file_name);
        in_image = (rgb_t **)malloc(image.height() * sizeof(rgb_t *));
        loadPixelsToArray(in_image, image);

        height = image.height();
        width = image.width();

        out = bitmap_image(width, height);

        if (!in_image)
        {
            printf("Error - Failed to open in_image\n");
            return 1;
        }

        struct args params[NUM_THREADS];
        for (int i = 0; i < NUM_THREADS; i++)
        {
            params[i].h_start = (i * height / NUM_THREADS);
            params[i].h_stop = ((i + 1) * height / NUM_THREADS);
            if (params[i].h_stop > height)
            {
                params[i].h_stop = height;
            }

            params[i].thread_id = i;
            params[i].height = height;
            params[i].width = width;
            pthread_create(&thread[i], NULL, &apply_filters, &params[i]);
        }

        for (int i = 0; i < NUM_THREADS; i++)
        {
            pthread_join(thread[i], NULL);
        }

        sprintf(out_file, "../../img/out/out_%d.bmp", pic);
        out.save_image(out_file);
    }

    gettimeofday(&end_time, NULL);
    pthread_barrier_destroy(&barrier);
    seq_time = (double)((end_time.tv_usec - start_time.tv_usec) / 1.0e6 + end_time.tv_sec - start_time.tv_sec);

    printf("time: %fs.\n", seq_time);
    return 0;
}
