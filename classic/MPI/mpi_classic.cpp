#include "../../utils/bitmap_image.hpp"

#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

#define N 1
#define ENOUGH 100

using namespace std;

rgb_t **get_original(char file_name[], int *height, int start, int end, int *width)
{
    rgb_t **in_image;
    bitmap_image image;

    image = bitmap_image(file_name);
    in_image = (rgb_t **)malloc(image.height() * sizeof(rgb_t *));

    for (int i = 0; i < image.height(); i++)
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

    *height = image.height();
    *width = image.width();

    return in_image;
}

void filter_black_white(rgb_t **in_image, int height, int start, int end, int width)
{
    if (end == height)
    {
        end--;
    }

    if (start == 0)
    {
        start++;
    }

    for (unsigned int i = start; i < end; ++i)
    {
        for (unsigned int j = 0; j < width - 1; ++j)
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

void filter_contrast(rgb_t **in_image, int height, int start, int end, int width)
{
    float contrast = 0.5;
    float factor = (259. * (contrast + 255.)) / (255. * (259. - contrast));

    if (end == height)
    {
        end--;
    }

    if (start == 0)
    {
        start++;
    }

    for (unsigned int i = start; i < end; ++i)
    {
        for (unsigned int j = 1; j < width - 1; ++j)
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

void filter_sharpness(rgb_t **in_image, int height, int start, int end, int width)
{
    static const float kernel[3][3] = {{0, -2. / 3., 0},
                                       {-2. / 3., 11. / 3., -2. / 3.},
                                       {0, -2. / 3., 0}};
    if (end == height)
    {
        end--;
    }

    if (start == 0)
    {
        start++;
    }

    for (int i = start; i < end; ++i)
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
                    red += static_cast<float>(in_image[i + ki][j + kj].red) *
                        kernel[ki + 1][kj + 1];
                    green += static_cast<float>(in_image[i + ki][j + kj].green)
                        * kernel[ki + 1][kj + 1];
                    blue += static_cast<float>(in_image[i + ki][j + kj].blue) *
                        kernel[ki + 1][kj + 1];
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

void filter_blur(rgb_t **in_image, int height, int start, int end, int width, bitmap_image &out)
{
    if (end == height)
    {
        end--;
    }

    if (start == 0)
    {
        start++;
    }

    for (int y = start; y < end; y++)
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
                    if (i >= 0 && j >= 0 && i < width && j < end)
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

int main(int argc, char *argv[])
{
    int numtasks, rank, height, start, end, width;
    char file_name[ENOUGH], out_file[ENOUGH];

    rgb_t **in_image = NULL;
    MPI_Datatype stype;
    bitmap_image out;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    define_struct(&stype);

    for (int pic = 1; pic <= N; pic++)
    {
        sprintf(file_name, "../../img/%d.bmp", pic);
        in_image = get_original(file_name, &height, start, end, &width);

        if (!in_image)
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
                int mod = height % (numtasks - 1);
                int start = chunk * (tid - 1);
                int end = tid * chunk;
                end += mod-- > 0 ? 1 : 0;

                // trimit un nr de linii fiecarui thread
                for (int i = start; i < end; ++i)
                {
                    MPI_Send(&(in_image[i][0]), width, stype, tid, 0,
                        MPI_COMM_WORLD);
                }
            }
        }
        else
        {
            int chunk = height / (numtasks - 1);
            int mod = height % (numtasks - 1);
            int start = chunk * (rank - 1);
            int end = rank * chunk;
            end += mod-- > 0 ? 1 : 0;

            for (int i = start; i < end; ++i)
            {
                MPI_Recv(&(in_image[i][0]), width, stype, 0, 0, MPI_COMM_WORLD,
                    MPI_STATUS_IGNORE);
            }

            // filter 1
            filter_black_white(in_image, height, start, end, width);
            // filter 2
            filter_contrast(in_image, height, start, end, width);
            // filter 3
            filter_sharpness(in_image, height, start, end, width);

            out = bitmap_image(width, height);

            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    out.set_pixel(x, y, in_image[x][y]);
                }
            }

            // filter 4
            filter_blur(in_image, height, start, end, width, out);
        }

        MPI_Barrier(MPI_COMM_WORLD);

        sprintf(out_file, "../../img/out/out_%d.bmp", pic);
        out.save_image(out_file);

        for (int i = 0; i < width; i++)
        {
            free(in_image[i]);
        }
        free(in_image);
    }

    MPI_Finalize();

    return 0;
}
