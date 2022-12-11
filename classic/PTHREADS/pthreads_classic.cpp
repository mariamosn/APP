#include "../../utils/bitmap_image.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string>

#define NUM_THREADS 4
#define N 10
#define ENOUGH 100

struct args
{
    int height;
    int width;
    int thread_id;
};

pthread_barrier_t barrier;

bitmap_image image = bitmap_image("../../img/1.bmp");
bitmap_image out;
rgb_t **in_image;

void loadPixelsToArray()
{
    in_image = (rgb_t **)malloc(image.height() * sizeof(rgb_t *));

    for (int i = 0; i < image.width(); i++)
    {
        in_image[i] = (rgb_t *)malloc(image.width() * sizeof(rgb_t));
    }

    for (int y = 0; y < image.height(); y++)
    {
        for (int x = 0; x < image.width(); x++)
        {
            image.get_pixel(y, x, in_image[y][x]);
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

    u_int64_t slice = (params.height - 2) / NUM_THREADS;
    u_int64_t start = thread_max(1, params.thread_id * slice);
    u_int64_t stop = (params.thread_id + 1) * slice;

    if (params.thread_id + 1 == NUM_THREADS)
    {
        stop = thread_max((params.thread_id + 1) * slice, (params.height - 1));
    }

    for (unsigned int i = start; i < stop; ++i)
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

    u_int64_t slice = (params.height - 2) / NUM_THREADS;
    u_int64_t start = thread_max(1, params.thread_id * slice);
    u_int64_t stop = (params.thread_id + 1) * slice;

    if (params.thread_id + 1 == NUM_THREADS)
    {
        stop = thread_max((params.thread_id + 1) * slice, (params.height - 1));
    }

    float contrast = 0.5;
    float factor = (259. * (contrast + 255.)) / (255. * (259. - contrast));

    for (unsigned int i = start; i < stop; ++i)
    {
        for (unsigned int j = 1; j < params.width - 1; ++j)
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

void filter_blur(void *args)
{
    struct args params = *((struct args *)args);
    u_int64_t slice = (params.height - 2) / NUM_THREADS;
    u_int64_t start = thread_max(1, params.thread_id * slice);
    u_int64_t stop = (params.thread_id + 1) * slice;

    if (params.thread_id + 1 == NUM_THREADS)
    {
        stop = thread_max((params.thread_id + 1) * slice, (params.height - 1));
    }

    for (int y = 0; y < params.height; y++)
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
                    if (i >= 0 && j >= 0 && i < params.width && j < params.height)
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

void filter_black_white(void *args)
{
    struct args params = *((struct args *)args);
    u_int64_t slice = (params.height - 2) / NUM_THREADS;
    u_int64_t start = thread_max(1, params.thread_id * slice);
    u_int64_t stop = (params.thread_id + 1) * slice;

    if (params.thread_id + 1 == NUM_THREADS)
    {
        stop = thread_max((params.thread_id + 1) * slice, (params.height - 1));
    }

    for (unsigned int i = start; i < stop; ++i)
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
    return NULL;
}

int main(int argc, const char *argv[])
{
    pthread_t thread[NUM_THREADS];
    pthread_attr_t attr;
    char file_name[ENOUGH], out_file[ENOUGH];

    image = bitmap_image(file_name);
    loadPixelsToArray();

    if (!in_image)
    {
        printf("Error - Failed to open in_image\n");
        return 1;
    }

    int height = image.height();
    int width = image.width();

    pthread_barrier_init(&barrier, NULL, NUM_THREADS);

    out = bitmap_image(width, height);

    for (int i = 0; i < NUM_THREADS; i++)
    {
        struct args *params = (struct args *)malloc(sizeof(struct args));

        params->thread_id = i;
        params->height = height;
        params->width = width;

        pthread_create(&thread[i], NULL, &apply_filters, (void *)params);
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(thread[i], NULL);
    }

    out.save_image("../../img/out.bmp");

    return 0;
}
