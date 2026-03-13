#ifndef GIFENC_H
#define GIFENC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ge_GIF {
    uint16_t w, h;
    int depth;
    int bgindex;
    int fd;
    int offset;
    int nframes;
    uint8_t *frame, *back;
    uint32_t partial;
    uint8_t buffer[0xFF];
} ge_GIF;

ge_GIF *ge_new_gif(
    const char *fname, uint16_t width, uint16_t height,
    uint8_t *palette, int depth, int bgindex, int loop
);
void ge_add_frame(ge_GIF *gif, uint16_t delay);
/* juice-cpp extension: add a frame at an explicit position with separate pixel data */
void ge_add_frame_at(ge_GIF *gif, uint16_t delay,
                     const uint8_t *pixels, uint16_t x, uint16_t y,
                     uint16_t w, uint16_t h);
void ge_close_gif(ge_GIF* gif);

#ifdef __cplusplus
}
#endif
#endif /* GIFENC_H */
