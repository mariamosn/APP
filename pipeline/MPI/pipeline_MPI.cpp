#include <stdio.h>
#include <stdlib.h>
#include "../../utils/bitmap_image.hpp"
#include "mpi.h"

#define N 10
#define ENOUGH 100

using namespace std;

rgb_t **get_original(char file_name[], int *height, int *width)
{
    rgb_t **inImage;
    bitmap_image image;

    image = bitmap_image(file_name);
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

    *height = image.height();
    *width = image.width();

    return inImage;
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
}

// Resource: https://stackoverflow.com/questions/66622459/sending-array-of-structs-in-mpi
void define_struct(MPI_Datatype *stype) {
    int cnt = 3;
    int blocklens[cnt] = {1, 1, 1};
    MPI_Datatype types[cnt] = {
                            MPI_UNSIGNED_CHAR,
                            MPI_UNSIGNED_CHAR,
                            MPI_UNSIGNED_CHAR
                            };
    MPI_Aint disps[cnt] = {
                        offsetof(rgb_t, red),
                        offsetof(rgb_t, green),
                        offsetof(rgb_t, blue)
                        };
    MPI_Type_create_struct(cnt, blocklens, disps, types, stype);
    MPI_Type_commit(stype);
}

int main(int argc, char *argv[])
{
    int numtasks, rank;
    MPI_Datatype stype;
    
    rgb_t **inImage = NULL;
    int height, width;
    bitmap_image out;

    char file_name[ENOUGH], out_file[ENOUGH];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    define_struct(&stype);

    for (int pic = 1; pic <= N; pic++) {
        if (rank == 0) {
            sprintf(file_name, "../../img/%d.bmp", pic);

            // citire imagine
            inImage = get_original(file_name, &height, &width);

            // filtru 1 = alb-negru
            filter_black_white(inImage, height, width);

            // filtru 2 = contrast
            filter_contrast(inImage, height, width);

            // trimite catre 1
            // height
            MPI_Send(&height, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            // width
            MPI_Send(&width, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            // img
            for (int i = 0; i < height; i++) {
                MPI_Send(&(inImage[i][0]), width, stype, rank + 1, 0, MPI_COMM_WORLD);
            }
            /*
            MPI_Send(&(inImage[0][0]),
                    height * width,
                    stype, rank + 2, 0, MPI_COMM_WORLD);
            */

            // free img
            for (int i = 0; i < width; i++) {
                free(inImage[i]);
            }
            free(inImage);

        } else if (rank == 1) {
            // primeste de la 0
            // height
            MPI_Recv(&height, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // width
            MPI_Recv(&width, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // aloca img
            inImage = (rgb_t **)malloc(height * sizeof(rgb_t *));
            for (int i = 0; i < width; i++) {
                inImage[i] = (rgb_t *)malloc(width * sizeof(rgb_t));
            }
            // img
            for (int i = 0; i < height; i++) {
                MPI_Recv(&(inImage[i][0]), width, stype, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }

            // filtru 3 = sharpness
            filter_sharpness(inImage, height, width);

            // trimite catre 2
            // height
            MPI_Send(&height, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            // width
            MPI_Send(&width, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
            // img
            for (int i = 0; i < height; i++) {
                MPI_Send(&(inImage[i][0]), width, stype, rank + 1, 0, MPI_COMM_WORLD);
            }

            // free img
            for (int i = 0; i < width; i++) {
                free(inImage[i]);
            }
            free(inImage);

        } else if (rank == 2) {
            // primeste de la 1
            // height
            MPI_Recv(&height, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // width
            MPI_Recv(&width, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            // aloca img
            inImage = (rgb_t **)malloc(height * sizeof(rgb_t *));
            for (int i = 0; i < width; i++) {
                inImage[i] = (rgb_t *)malloc(width * sizeof(rgb_t));
            }
            // img
            for (int i = 0; i < height; i++) {
                MPI_Recv(&(inImage[i][0]), width, stype, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            /*
            MPI_Recv(&(inImage[0][0]),
                    height * width,
                    stype, rank - 2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            */

            out = bitmap_image(width, height);

            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    out.set_pixel(x, y, inImage[x][y]);
                }
            }

            // filtru 4 = blur
            filter_blur(inImage, height, width, out);

            // scriere imagine
            sprintf(out_file, "../../img/out/out_%d.bmp", pic);
            out.save_image(out_file);

            // free img
            for (int i = 0; i < width; i++) {
                free(inImage[i]);
            }
            free(inImage);
        }
    }

    MPI_Finalize();

    return 0;
}
