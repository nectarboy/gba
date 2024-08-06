#define BPP 4
#define SCALE 2

unsigned char frameBuffer[SCALE * SW * SCALE * SH * BPP];
int BUFFER_BYTES_PER_ROW = SCALE * SW * BPP;
void framebufferPutPx(int x, int y, u32 color) {
    x *= SCALE;
    y *= SCALE;

    for (int i = 0; i < SCALE; i++) {
        for (int ii = 0; ii < SCALE; ii++) {
            int index = BPP * ((y + i) * SCALE * SW + (x + ii));

            frameBuffer[index++] = 0xff;
            frameBuffer[index++] = (u8)(color >> 16);
            frameBuffer[index++] = (u8)(color >> 8);
            frameBuffer[index]   = (u8)(color >> 0);
        }
    }
}