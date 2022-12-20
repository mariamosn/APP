#include <stdio.h>
#include <stdlib.h>
#include "../../utils/bitmap_image.hpp"
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define N 500
#define ENOUGH 100

rgb_t **inImage1, **inImage2;
bitmap_image image1, image2;
bitmap_image out1, out2;

using namespace std;

sem_t sem_read_a;
sem_t sem_read_b;
sem_t sem_write_b;
sem_t sem_write_a;
sem_t sem_filter1_a;
sem_t sem_filter1_b;
sem_t sem_filter2_a;
sem_t sem_filter2_b;
sem_t sem_filter3_a;
sem_t sem_filter3_b;
sem_t sem_filter4_a;
sem_t sem_filter4_b;

int finished = 0, finished2 = 0, finished_filter1 = 0, finished_filter2 = 0,
    finished_filter3 = 0, finished_filter4 = 0;
int read_allowed = 0, write_allowed = 0, filter1_allowed = 0,
    filter2_allowed = 0, filter3_allowed = 0, filter4_allowed = 0;

char file_name1[ENOUGH], file_name2[ENOUGH], out_file[ENOUGH];

void loadPixelsToArray(rgb_t **inImage, bitmap_image image)
{

    for (int i = 0; i < image.width(); i++)
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

void filter_black_white(rgb_t **inImage, int height, int width)
{
    for (unsigned int i = 1; i < height - 1; ++i)
    {
        for (unsigned int j = 1; j < width - 1; ++j)
        {
            unsigned int gray = 0.2126 * inImage[i][j].red +
                                0.7152 * inImage[i][j].green +
                                0.0722 * inImage[i][j].blue;
            gray = (gray > 255) ? 255 : gray;
            inImage[i][j].red = gray;
            inImage[i][j].green = gray;
            inImage[i][j].blue = gray;
        }
    }
}

void filter_contrast(rgb_t **inImage, int height, int width)
{
    float contrast = 0.5;
    float factor = (259. * (contrast + 255.)) / (255. * (259. - contrast));
    for (unsigned int i = 1; i < height - 1; ++i)
    {
        for (unsigned int j = 1; j < width - 1; ++j)
        {
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
}

void filter_sharpness(rgb_t **inImage, int height, int width)
{
    static const float kernel[3][3] = {{0, -2. / 3., 0},
                                       {-2. / 3., 11. / 3., -2. / 3.},
                                       {0, -2. / 3., 0}};

    for (int i = 1; i < height - 1; ++i)
    {
        for (int j = 1; j < width - 1; ++j)
        {
            float red, green, blue;
            float tempColor;
            red = green = blue = 0;

            for (int ki = -1; ki <= 1; ++ki)
            {
                for (int kj = -1; kj <= 1; ++kj)
                {
                    red += static_cast<float>(inImage[i + ki][j + kj].red) *
                        kernel[ki + 1][kj + 1];
                    green += static_cast<float>(inImage[i + ki][j + kj].green) *
                        kernel[ki + 1][kj + 1];
                    blue += static_cast<float>(inImage[i + ki][j + kj].blue) *
                        kernel[ki + 1][kj + 1];
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
}

bitmap_image filter_blur(rgb_t **inImage, int height, int width,
    bitmap_image out)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
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
                    if (i >= 0 && j >= 0 && i < width && j < height)
                    {
                        neighbors++;
                        colour = inImage[i][j];

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
    return out;
}

void *input_task(void *args)
{

    int i = 1;

    while (1)
    {
        if (finished)
        {
            break;
        }
        // indica daca actiunea are loc pe matricea A sau matricea B
        read_allowed = 1 - read_allowed;
        // semaforul sem_read indica daca continutul matricei a fost scris deci
        // aceasta poate fi actualizata
        // semaforul sem_filter_1_ va indica filtrului 1 ca poate utiliza
        // continutul matricei respective, deoarece a fost citita
        if (read_allowed)
        {
            sem_wait(&sem_read_a);
            sprintf(file_name1, "../../img/%d.bmp", i);
            image1 = bitmap_image(file_name1);
            inImage1 = (rgb_t **)malloc(image1.height() * sizeof(rgb_t *));
            loadPixelsToArray(inImage1, image1);
            i++;
            sem_post(&sem_filter1_a);
        }
        else
        {
            sem_wait(&sem_read_b);
            sprintf(file_name2, "../../img/%d.bmp", i);
            image2 = bitmap_image(file_name2);
            inImage2 = (rgb_t **)malloc(image2.height() * sizeof(rgb_t *));
            loadPixelsToArray(inImage2, image2);
            i++;
            sem_post(&sem_filter1_b);
        }

        if (i == N)
        {
            finished = 1;
        }
    }

    return NULL;
}

void *output_task(void *args)
{
    int height, width;
    int i = 1;
    while (1)
    {

        if (finished2)
        {
            break;
        }

        write_allowed = 1 - write_allowed;

        // semaforul sem_write indica daca continutul matricei a fost procesat
        // de filtre deci aceasta poate fi scrisa in fisier
        // semaforul sem_read va indica filtrului de read input ca poate citi
        // noi valori in matricea respectiva
        if (write_allowed == 1)
        {
            sem_wait(&sem_write_a);
            sprintf(out_file, "../../img/out/out_%d.bmp", i);
            out1.save_image(out_file);
            i++;
            sem_post(&sem_read_a);
        }
        else
        {
            sem_wait(&sem_write_b);
            sprintf(out_file, "../../img/out/out_%d.bmp", i);
            out2.save_image(out_file);
            i++;
            sem_post(&sem_read_b);
        }
        if (i == N)
        {
            finished2 = 1;
        }
    }

    return NULL;
}

// la functiile ce reprezinta etapele de pipeline de aplicare a filtrelor
// semaforul sem_filterN_ unde N este numarul filtrului va marca starea de
// asteptare a filtrului curent daca nicio matrice nu este disponibila
// semaforul sem_filter(N+1) va indica filtrului urmator ca s-a terminat
// procesoarea actuala si ca acesta poate incepe procesarea matricei

void *filter_contrast_task(void *args)
{

    int i = 1;

    while (1)
    {
        if (finished_filter2)
        {
            break;
        }

        filter2_allowed = 1 - filter2_allowed;
        if (filter2_allowed)
        {
            sem_wait(&sem_filter2_a);
            filter_contrast(inImage1, image1.height(), image1.width());
            i++;
            sem_post(&sem_filter3_a);
        }
        else
        {
            sem_wait(&sem_filter2_b);
            filter_contrast(inImage2, image2.height(), image2.width());
            i++;
            sem_post(&sem_filter3_b);
        }

        if (i == N)
        {
            finished_filter2 = 1;
        }
    }

    return NULL;
}

void *filter_black_white_task(void *args)
{

    int i = 1;

    while (1)
    {
        if (finished_filter1)
        {
            break;
        }

        filter1_allowed = 1 - filter1_allowed;

        if (filter1_allowed)
        {
            sem_wait(&sem_filter1_a);
            filter_black_white(inImage1, image1.height(), image1.width());
            i++;
            sem_post(&sem_filter2_a);
        }
        else
        {
            sem_wait(&sem_filter1_b);
            filter_black_white(inImage2, image2.height(), image2.width());
            i++;
            sem_post(&sem_filter2_b);
        }

        if (i == N)
        {
            finished_filter1 = 1;
        }
    }

    return NULL;
}

void *filter_sharpness_task(void *args)
{

    int i = 1;

    while (1)
    {
        if (finished_filter3)
        {
            break;
        }

        filter3_allowed = 1 - filter3_allowed;

        if (filter3_allowed)
        {
            sem_wait(&sem_filter3_a);
            filter_sharpness(inImage1, image1.height(), image1.width());
            i++;
            sem_post(&sem_filter4_a);
        }
        else
        {
            sem_wait(&sem_filter3_b);
            filter_sharpness(inImage2, image2.height(), image2.width());
            i++;
            sem_post(&sem_filter4_b);
        }

        if (i == N)
        {
            finished_filter3 = 1;
        }
    }

    return NULL;
}

void *filter_blur_task(void *args)
{

    int i = 1;

    while (1)
    {
        if (finished_filter4)
        {
            break;
        }

        filter4_allowed = 1 - filter4_allowed;

        if (filter4_allowed)
        {
            sem_wait(&sem_filter4_a);
            out1 = bitmap_image(image1.height(), image1.width());
            out1 = filter_blur(inImage1, image1.height(), image1.width(), out1);
            i++;
            sem_post(&sem_write_a);
        }
        else
        {
            sem_wait(&sem_filter4_b);
            out2 = bitmap_image(image2.height(), image2.width());
            out2 = filter_blur(inImage2, image2.height(), image2.width(), out2);
            i++;
            sem_post(&sem_write_b);
        }

        if (i == N)
        {
            finished_filter4 = 1;
        }
    }

    return NULL;
}

int main()
{
    int NUM_THREADS = 6;
    pthread_t threads[NUM_THREADS];
    int r1, r2, r3, r4, r5, r6;
    long id;
    void *status;
    long arguments[NUM_THREADS];

    // init
    sem_init(&sem_read_a, 0, 1);
    sem_init(&sem_read_b, 0, 1);
    sem_init(&sem_write_a, 0, 0);
    sem_init(&sem_write_b, 0, 0);
    sem_init(&sem_filter1_a, 0, 0);
    sem_init(&sem_filter1_b, 0, 0);
    sem_init(&sem_filter2_a, 0, 0);
    sem_init(&sem_filter2_b, 0, 0);
    sem_init(&sem_filter3_a, 0, 0);
    sem_init(&sem_filter3_b, 0, 0);
    sem_init(&sem_filter4_a, 0, 0);
    sem_init(&sem_filter4_b, 0, 0);

    arguments[0] = 0;
    r1 = pthread_create(&threads[0], NULL, input_task, (void *)&arguments[0]);

    arguments[1] = 1;
    r2 = pthread_create(&threads[1], NULL, output_task, (void *)&arguments[1]);

    arguments[2] = 2;
    r3 = pthread_create(&threads[2], NULL, filter_black_white_task,
        (void *)&arguments[2]);

    arguments[3] = 3;
    r4 = pthread_create(&threads[3], NULL, filter_contrast_task,
        (void *)&arguments[3]);

    arguments[4] = 4;
    r5 = pthread_create(&threads[4], NULL, filter_sharpness_task,
        (void *)&arguments[4]);

    arguments[5] = 5;
    r6 = pthread_create(&threads[5], NULL, filter_blur_task,
        (void *)&arguments[5]);

    if (r1 || r2 || r3 || r4 || r5 || r6)
    {
        printf("Eroare la crearea thread-ului %d\n", 0);
        exit(-1);
    }

    for (id = 0; id < NUM_THREADS; id++)
    {
        r1 = pthread_join(threads[id], &status);

        if (r1)
        {
            printf("Eroare la asteptarea thread-ului %d\n", 0);
            exit(-1);
        }
    }

    sem_destroy(&sem_read_a);
    sem_destroy(&sem_read_b);
    sem_destroy(&sem_write_a);
    sem_destroy(&sem_write_b);
    sem_destroy(&sem_filter1_a);
    sem_destroy(&sem_filter1_b);
    sem_destroy(&sem_filter2_a);
    sem_destroy(&sem_filter2_b);
    sem_destroy(&sem_filter3_a);
    sem_destroy(&sem_filter3_b);
    sem_destroy(&sem_filter4_a);
    sem_destroy(&sem_filter4_b);

    return 0;
}
