#ifndef _TGA_H_
#define _TGA_H_

typedef struct
{
    unsigned char imageTypeCode;
    short int imageWidth;
    short int imageHeight;
    unsigned char bitCount;
    unsigned char *imageData;
} TGAFILE;

bool LoadTGAFile(char *filename, TGAFILE *tgaFile);

#endif