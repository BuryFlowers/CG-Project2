#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int stbi_write_jpg(char const* filename, int w, int h, int comp, const void* data, int quality);