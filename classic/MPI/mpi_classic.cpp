#include <stdio.h>
#include <stdlib.h>
#include "../../utils/bitmap_image.hpp"
#include "mpi.h"

#define N 10
#define ENOUGH 100

using namespace std;

MPI_Datatype stype;
bitmap_image image = bitmap_image("../../img/1.bmp");
bitmap_image out;
rgb_t **inImage;

void loadPixelsToArray()
{
    inImage = (rgb_t **)malloc(image.height() * sizeof(rgb_t *));

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
    for (unsigned int i = 0; i < height; ++i)
    {
        for (unsigned int j = 0; j < width - 1; ++j)
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
                    red += static_cast<float>(inImage[i + ki][j + kj].red) * kernel[ki + 1][kj + 1];
                    green += static_cast<float>(inImage[i + ki][j + kj].green) * kernel[ki + 1][kj + 1];
                    blue += static_cast<float>(inImage[i + ki][j + kj].blue) * kernel[ki + 1][kj + 1];
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

void filter_blur(rgb_t **inImage, int height, int width, bitmap_image &out)
{
    for (int y = 1; y < height - 1; y++)
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
                    if (i >= 0 && j >= 0 && i < width && j < height) // poate (end -start)?
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
}

void define_struct(MPI_Datatype *stype)
{
    int blocklens[3] = {1, 1, 1};
    MPI_Datatype types[3] = {
        MPI_UNSIGNED_CHAR,
        MPI_UNSIGNED_CHAR,
        MPI_UNSIGNED_CHAR};
    MPI_Aint disps[3] = {
        offsetof(rgb_t, red),
        offsetof(rgb_t, green),
        offsetof(rgb_t, blue)};
    MPI_Type_create_struct(3, blocklens, disps, types, stype);
    MPI_Type_commit(stype);
}

inline unsigned long thread_max(unsigned long a, unsigned long b)
{
    return (a > b) ? a : b;
}

int main(int argc, char *argv[])
{
    int numtasks, rank;
    char out_file[ENOUGH];

    double timer_start;
    double timer_end;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    define_struct(&stype);

    loadPixelsToArray();
    int height = image.height();
    int width = image.width();

    if (!inImage)
    {
        printf("Error - Failed to open in_image\n");
        return 1;
    }

    if (rank == 0)
    {
        // trimitere imagine de la Master
        for (int tid = 1; tid < numtasks; ++tid)
        {
            int chunk = height / (numtasks - 1);
            int mod = height % numtasks;
            int start = chunk * (tid - 1);
            int end = tid * chunk;
            end += mod-- > 0 ? 1 : 0;

            // trimit un nr de linii fiecarui thread
            for (int i = start; i < end; ++i)
            {
                MPI_Send(&(inImage[i][0]), width, stype, tid, 0, MPI_COMM_WORLD);
            }
            timer_start = MPI_Wtime();
        }
    }
    else
    {
        int chunk = height / (numtasks - 1);
        int mod = height % numtasks;
        int start = chunk * (rank - 1);
        int end = rank * chunk;
        end += mod-- > 0 ? 1 : 0;

        for (int i = start; i < end; ++i)
        {
            MPI_Recv(&(inImage[i][0]), width, stype, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        filter_black_white(inImage, height, width);

        filter_contrast(inImage, height, width);

        filter_sharpness(inImage, height, width);

        out = bitmap_image(width, height);

        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                out.set_pixel(x, y, inImage[x][y]);
            }
        }
        filter_blur(inImage, height, width, out);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0)
    {
        timer_end = MPI_Wtime();
        printf("time: %fs.\n", timer_end - timer_start);
    }
    out.save_image("../../img/out.bmp");

    MPI_Finalize();

    return 0;
}
