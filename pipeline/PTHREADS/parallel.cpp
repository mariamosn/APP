#include <stdio.h>
#include <stdlib.h>
#include "bitmap_image.hpp"
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define N 10
#define ENOUGH 100

rgb_t **inImage1, **inImage2;
bitmap_image image1, image2;
bitmap_image out1, out2;

using namespace std;

sem_t sem_read_a;
sem_t sem_read_b;
sem_t sem_write_b;
sem_t sem_write_a;
sem_t sem_op;

int ctr_read;
int ctr_write;

pthread_cond_t cond_read;
pthread_cond_t cond_write;

int finished = 0, finished2 = 0;
int read_allowed = 0, write_allowed = 0;
int signal_flag_write, signal_flag_read;
int read_run = 1, write_run = 1;
int written_a = 1, written_b = 1;
int a = 0, b = 0;

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
    //  return inImage;
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

        read_allowed = 1 - read_allowed;

        if (read_allowed)
        {
            sem_wait(&sem_read_a);
            sprintf(file_name1, "./img/%d.bmp", i);
            image1 = bitmap_image(file_name1);
            inImage1 = (rgb_t **)malloc(image1.height() * sizeof(rgb_t *));
            loadPixelsToArray(inImage1, image1);
            i++;
            sem_post(&sem_write_a);
        }
        else
        {
            sem_wait(&sem_read_b);
            sprintf(file_name2, "./img/%d.bmp", i);
            image2 = bitmap_image(file_name2);
            inImage2 = (rgb_t **)malloc(image2.height() * sizeof(rgb_t *));
            loadPixelsToArray(inImage2, image2);
            i++;
            sem_post(&sem_write_b);
        }

        if (i == N)
        {
            finished = 1;
        }
        write_run = 1;
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
        if (write_allowed == 1)
        {
            sem_wait(&sem_write_a);
            height = image1.height();
            width = image1.width();
            sprintf(out_file, "./img/out/out_%d.bmp", i);
            out1 = bitmap_image(width, height);
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    out1.set_pixel(x, y, inImage1[x][y]);
                }
            }
            out1.save_image(out_file);
            i++;
            sem_post(&sem_read_a);
        }
        else
        {
            sem_wait(&sem_write_b);
            height = image2.height();
            width = image2.width();
            sprintf(out_file, "./img/out/out_%d.bmp", i);
            out2 = bitmap_image(width, height);
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    out2.set_pixel(x, y, inImage2[x][y]);
                }
            }
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

int main()
{
    time_t time;
    struct timeval t0, t1, t2, t3, t4, t5, t6, start, end;
    float delta;
    char file_name[ENOUGH], out_file[ENOUGH];

    int NUM_THREADS = 2;
    pthread_t threads[NUM_THREADS];
    int r;
    long id;
    void *status;
    long arguments[NUM_THREADS];

    // init
    pthread_cond_init(&cond_read, NULL);
    pthread_cond_init(&cond_write, NULL);

    sem_init(&sem_read_a, 0, 1);
    sem_init(&sem_read_b, 0, 1);
    sem_init(&sem_write_a, 0, 0);
    sem_init(&sem_write_b, 0, 0);
    gettimeofday(&start, NULL);

    arguments[0] = 0;
    r = pthread_create(&threads[0], NULL, input_task, (void *)&arguments[0]);

    if (r)
    {
        printf("Eroare la crearea thread-ului %d\n", 0);
        exit(-1);
    }

    arguments[1] = 1;
    r = pthread_create(&threads[1], NULL, output_task, (void *)&arguments[1]);

    if (r)
    {
        printf("Eroare la crearea thread-ului %d\n", 1);
        exit(-1);
    }

    for (id = 0; id < NUM_THREADS; id++)
    {
        r = pthread_join(threads[id], &status);

        if (r)
        {
            printf("Eroare la asteptarea thread-ului %d\n", 0);
            exit(-1);
        }
    }

    pthread_cond_destroy(&cond_read);
    pthread_cond_destroy(&cond_write);
    sem_destroy(&sem_read_a);
    sem_destroy(&sem_read_b);
    sem_destroy(&sem_write_a);
    sem_destroy(&sem_write_b);
    return 0;
}
