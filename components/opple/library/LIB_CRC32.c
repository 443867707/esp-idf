#include "LIB_CRC32.h"


static unsigned int crc32table[256];

void init_crc32(void)
{
    unsigned int c;
    unsigned int i, j;

    for (i = 0; i < 256; i++)
    {
        c = i;
        for (j = 0; j < 8; j++)
        {
            if (c & 1)
            {
                c = 0xedb88320L ^ (c >> 1);  // CRC32-IEEE
            }
            else
            {
                c = c >> 1;
            }
        }
        crc32table[i] = c;
    }
}

unsigned int getCrc32(const unsigned char* data,unsigned int len)
{
    unsigned int i;
    unsigned int crc = 0xFFFFFFFF;

    for (i = 0; i < len; i++)
    {
        crc = crc32table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc;
}


