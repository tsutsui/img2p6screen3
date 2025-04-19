#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_LINEAR
#include "stb_image.h"

#define P6_XSIZE	128
#define P6_YSIZE	192
#define P6_STRIDE	(P6_XSIZE / 4)

#define IMG_XSIZE	(P6_XSIZE * 2)
#define IMG_YSIZE	P6_YSIZE

const char progname[] = "img2p6screen3";

/* 固定の4色パレット（PC-6001 SCREEN 3） */
typedef struct {
    uint8_t colors[4][3];
} p6palettedef;

static const p6palettedef p6palette[2] = {
    {
        {
            {   0, 255,   0 }, // 緑
            { 255, 255,   0 }, // 黄
            {   0,   0, 255 }, // 青
            { 255,   0,   0 }, // 赤
        }
    },
    {
        {
            { 255, 255, 255 }, // 白
            {   0, 255, 255 }, // シアン
            { 255,   0, 255 }, // マゼンタ
            { 255, 128,   0 }, // 橙
        }
    }
};

static void
usage(void)
{
    fprintf(stderr, "使い方: %s [-c 1|2] 入力画像ファイル 出力バイナリファイル\n", progname);
    fprintf(stderr, "  -c 1    color,,1 パレット（緑・黄・青・赤）※デフォルト\n");
    fprintf(stderr, "  -c 2    color,,2 パレット（白・シアン・マゼンタ・橙）\n");
    exit(EXIT_FAILURE);
}

/* 最近傍色インデックスを求める */
static unsigned int
nearest_color(const p6palettedef *palette, uint8_t r, uint8_t g, uint8_t b)
{
    unsigned int min_dist = UINT_MAX;
    unsigned int index = 0;
    unsigned int i;
    for (i = 0; i < 4; ++i) {
        int dr = (int)r - (int)palette->colors[i][0];
        int dg = (int)g - (int)palette->colors[i][1];
        int db = (int)b - (int)palette->colors[i][2];
        unsigned int dist = (dr * dr) + (dg * dg) + (db * db);
        if (dist < min_dist) {
            min_dist = dist;
            index = i;
        }
    }
    return index;
}

int
main(int argc, char *argv[])
{
    int color_type = 1;
    int width, height, channels;
    int c, i, x, y, x_byte;
    uint8_t *img;
    const char *ifname, *ofname;
    const p6palettedef *palette;
    FILE *ofp;

    while ((c = getopt(argc, argv, "c:")) != -1) {
        switch (c) {
        case 'c':
            color_type = atoi(optarg);
            if (color_type < 1 || color_type > 2) {
                usage();
            }
            break;
        default:
            usage();
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 2)
        usage();

    ifname = argv[0];
    ofname = argv[1];
    palette = &p6palette[color_type - 1];

    img = stbi_load(ifname, &width, &height, &channels, 3); /* RGB固定 */
    if (img == NULL) {
        fprintf(stderr, "画像を読み込めませんでした: %s\n", ifname);
        exit(EXIT_FAILURE);
    }

    if (width != IMG_XSIZE || height != IMG_YSIZE) {
        fprintf(stderr, "エラー: 入力画像のサイズは %dx%d である必要があります（入力画像サイズ: %dx%d）\n",
          IMG_XSIZE, IMG_YSIZE, width, height);
        stbi_image_free(img);
        exit(EXIT_FAILURE);
    }

    ofp = fopen(ofname, "wb");
    if (ofp == NULL) {
        fprintf(stderr, "出力ファイルを開けませんでした: %s\n", ofname);
        stbi_image_free(img);
        exit(EXIT_FAILURE);
    }

    /* 変換: 256x192 を 128x192 にリサイズ（横2ドットを1ドットに） */
    for (y = 0; y < P6_YSIZE; y++) {
        for (x_byte = 0; x_byte < P6_STRIDE; x_byte++) {
            uint8_t out_byte = 0;
            for (i = 0; i < 4; ++i) {
                /* 2ドットを1ドットに平均化 */
                int x = (x_byte * 4 + i) * 2;
                int idx1 = (y * IMG_XSIZE + x) * 3;
                int idx2 = (y * IMG_XSIZE + x + 1) * 3;
                uint8_t r = (img[idx1 + 0] + img[idx2 + 0]) / 2;
                uint8_t g = (img[idx1 + 1] + img[idx2 + 1]) / 2;
                uint8_t b = (img[idx1 + 2] + img[idx2 + 2]) / 2;
                unsigned int color = nearest_color(palette, r, g, b);
                out_byte |= (color & 0x03U) << ((3 - i) * 2);
            }
            fwrite(&out_byte, 1, 1, ofp);
        }
    }

    fclose(ofp);
    stbi_image_free(img);
    exit(EXIT_SUCCESS);
}
