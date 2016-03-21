#include <stdlib.h>
#include <stdio.h>

/*
 * Pass -DFILEPATH="/home/myusername/" or the like in GCC flags.
 * Otherwise, the below test has the current directory as the default.
 */
#ifndef FILEPATH
#define FILEPATH        "./"
#endif
#ifndef WIDTH
#define WIDTH           1024
#endif
#ifndef HEIGHT
#define HEIGHT          768
#endif

static const char file_path[] = FILEPATH;
static const unsigned short data_width = WIDTH;
static const unsigned short data_height = HEIGHT;
#undef FILEPATH
#undef WIDTH
#undef HEIGHT

static unsigned long capture_ID = 0;

static int dump_framebuffer(int device_ID);
static int correct_framebuffer(unsigned long screen_ID);

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

    fseek(input, 0, SEEK_END);
    file_size  = ftell(input); /* quick hack that assumes POSIX support */
    fseek(input, 0, SEEK_SET);
    pixels = (unsigned char *)malloc(file_size * 3 / 4);

    file_size += sizeof(bitmap_header);
    file_size  = (file_size * 3) / 4; /* R8G8B8 is 75% of R8G8B8A8. */

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

    if (fwrite(pixels, file_size - sizeof(bitmap_header), 1, output) != 1) {
        fputs("Failed to export BMP pixel array.\n", stderr);
        fclose(output);
        return 1;
    }
    fclose(output);
    return 0;
}
