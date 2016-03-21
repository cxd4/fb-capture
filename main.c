#include <stdlib.h>
#include <stdio.h>

#include "config.h"
static const char file_path[] = FILEPATH;
static const unsigned short data_width = WIDTH;
static const unsigned short data_height = HEIGHT;
#undef FILEPATH
#undef WIDTH
#undef HEIGHT

static unsigned long capture_ID = 0;

int main(int argc, char* argv[])
{
    int error_code;

    error_code = system(NULL);
    if (error_code == 0) {
        fputs("It seems that we lack a command interpreter.\n", stderr);
        return 1;
    }

    error_code = dump_framebuffer(0);
    if (error_code != 0) {
        fprintf(stderr, "Error %i dumping Linux frame buffer.\n", error_code);
        return (error_code);
    }

    puts("Correcting frame buffer byte order to 24-bit R8G8B8...");
    return correct_framebuffer(capture_ID);
}

static int dump_framebuffer(int device_ID)
{
    char dump_command[32 + sizeof(file_path)];
    FILE* stream;

    do {
        sprintf(&dump_command[0], "%s%08lX.raw", &file_path[0], capture_ID);
        stream = fopen(&dump_command[0], "rb");
        if (stream == NULL)
            break;
        fclose(stream);
        capture_ID = (capture_ID + 1) & 0xFFFFFFFFul;
    } while (capture_ID != 0);

    sprintf(
        &dump_command[0],
        "cat /dev/fb%i > %s%08lX.raw",
        device_ID, &file_path[0], capture_ID
    );

    printf("\nNow executing:\n%s\n\n", &dump_command[0]);
    return system(&dump_command[0]);
}

static int correct_framebuffer(unsigned long screen_ID)
{
    char location_to_pixels[sizeof(file_path)];
    int characters[4];
    FILE * input, * output;
    unsigned char * pixels;
    long file_size;
    register long i;
    static unsigned char bitmap_header[] = {
        'B', 'M',
        (0 >> 0), (0 >> 8), (0 >> 16), (0 >> 24), /* total file size in bytes */
        0x00, 0x00, /* reserved */
        0x00, 0x00, /* reserved */
        14 + 12, (0 >> 8), (0 >> 16), (0 >> 24), /* pixel map offset */

        12, (0 >> 8), (0 >> 16), (0 >> 24), /* BMP version header size */
        (data_width  >> 0) & 0xFFu, (data_width  >> 8) & 0xFFu,
        (data_height >> 0) & 0xFFu, (data_height >> 8) & 0xFFu,
        1, (0x00 >> 8), /* number of color planes = 1 */
        24, (0 >> 8), /* R8G8B8 24 bits per pixel */
    };

    sprintf(&location_to_pixels[0], "%s%08lX.raw", &file_path[0], screen_ID);
    input  = fopen(&location_to_pixels[0], "rb");
    sprintf(&location_to_pixels[0], "%s%08lX.bmp", &file_path[0], screen_ID);
    output = fopen(&location_to_pixels[0], "wb");
    if (input == NULL || output == NULL) {
        fprintf(stderr,
            "Failed to re-open for reading:  %s\n", &location_to_pixels[0]
        );
        return 1;
    }

    file_size = -1;
    do {
        ++file_size;
    } while (fgetc(input) >= 0);
    fseek(input, 0, SEEK_SET);
    if (file_size % 4 != 0) {
        fputs("This does not seem to be a 32-bpp frame buffer dump.\n", stderr);
        return 1;
    }

    file_size  = (file_size / 4) * 3; /* R8G8B8 is 75% of R8G8B8A8. */
    pixels = (unsigned char *)malloc(file_size);
    file_size += sizeof(bitmap_header);

    bitmap_header[2] = (unsigned char)((file_size >>  0) & 0xFF);
    bitmap_header[3] = (unsigned char)((file_size >>  8) & 0xFF);
    bitmap_header[4] = (unsigned char)((file_size >> 16) & 0xFF);
    bitmap_header[5] = (unsigned char)((file_size >> 24) & 0xFF);
    if (fwrite(&bitmap_header[0], sizeof(bitmap_header), 1, output) != 1) {
        fputs("Failed to initialize BMP with header.\n", stderr);
        return 1;
    }
    if (pixels == NULL) {
        fprintf(stderr, "Unable to allocate %li bytes of pixels.\n", file_size);
        return 1;
    }

    for (i = 0; i < file_size; i += 4 - 1) {
        characters[0] = fgetc(input);
        characters[1] = fgetc(input);
        characters[2] = fgetc(input);
        characters[3] = fgetc(input);

        if (characters[3] < 0)
            break
        ;
        pixels[i + 0] = (unsigned char)characters[0];
        pixels[i + 1] = (unsigned char)characters[1];
        pixels[i + 2] = (unsigned char)characters[2];
    }
    fclose(input);

    for (i = data_height - 1; i >= 0; i--) {
        size_t elements;

        elements = fwrite(pixels + i*3*data_width, 3*data_width, 1, output);
        if (elements != 1)
            fprintf(stderr, "Failed to export scanline %li.\n", i)
        ;
    }
    fclose(output);
    return 0;
}
