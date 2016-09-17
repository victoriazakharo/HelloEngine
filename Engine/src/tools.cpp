#include <cmath>
#include <fstream>
#include <iostream>
#include "tools.h"
#include <libpng/png.h>
#include <string.h>

namespace HelloEngine
{
	std::vector<char> GetBinaryFileContents(const char *filename) {

		std::ifstream file(filename, std::ios::binary);
		if (file.fail()) {
			std::cout << "Could not open \"" << filename << "\" file!" << std::endl;
			return std::vector<char>();
		}

		std::streampos begin, end;
		begin = file.tellg();
		file.seekg(0, std::ios::end);
		end = file.tellg();

		std::vector<char> result(static_cast<size_t>(end - begin));
		file.seekg(0, std::ios::beg);
		file.read(&result[0], end - begin);
		file.close();

		return result;
	}	

	std::array<float, 16> GetOrthographicProjectionMatrix(float const left_plane, float const right_plane, float const top_plane, float const bottom_plane, float const near_plane, float const far_plane) {
		return{
			2.0f / (right_plane - left_plane),
			0.0f,
			0.0f,
			0.0f,

			0.0f,
			2.0f / (bottom_plane - top_plane),
			0.0f,
			0.0f,

			0.0f,
			0.0f,
			-2.0f / (far_plane - near_plane),
			0.0f,

			-(right_plane + left_plane) / (right_plane - left_plane),
			-(bottom_plane + top_plane) / (bottom_plane - top_plane),
			-(far_plane + near_plane) / (far_plane - near_plane),
			1.0f
		};
	}	

	Image GetImage(const char *filename)
	{
		FILE* file = fopen(filename, "rb");
		if(file == nullptr)
		{
			printf("Could not open file %s!\n", filename);
			return Image();
		}
		png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		png_infop info = png_create_info_struct(png);
		if (setjmp(png_jmpbuf(png))) {
			png_destroy_read_struct(&png, &info, nullptr);
			fclose(file);
			printf("Could not read image data!\n");
			return Image();
		}
		png_init_io(png, file);
		png_read_info(png, info);

		png_uint_32 width = png_get_image_width(png, info);
		png_uint_32 height = png_get_image_height(png, info);
	
		png_byte color_type = png_get_color_type(png, info);
		png_byte bit_depth = png_get_bit_depth(png, info);

		if (width <= 0 || height <= 0) {
			printf("Could not read image data!\n");
			return Image();
		}
		if (bit_depth == 16)
			png_set_strip_16(png);

		if (color_type == PNG_COLOR_TYPE_PALETTE)
			png_set_palette_to_rgb(png);

		if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
			png_set_expand_gray_1_2_4_to_8(png);

		if (png_get_valid(png, info, PNG_INFO_tRNS))
			png_set_tRNS_to_alpha(png);

		if (color_type == PNG_COLOR_TYPE_RGB ||
			color_type == PNG_COLOR_TYPE_GRAY ||
			color_type == PNG_COLOR_TYPE_PALETTE)
			png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

		if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(png);
		png_read_update_info(png, info);
		png_bytepp row_pointers = static_cast<png_bytepp>(malloc(sizeof(png_bytep) * height));
		for (int y = 0; y < height; y++) {
			row_pointers[y] = static_cast<png_bytep>(malloc(png_get_rowbytes(png, info)));
		}
		png_read_image(png, row_pointers);
		unsigned int row_bytes = png_get_rowbytes(png, info);
		Image image(width, height, row_bytes * height);
		unsigned char * data = image.GetData();
		for (int i = 0; i < height; i++) {
			memcpy(data + row_bytes * i, row_pointers[i], row_bytes);
		}
		fclose(file);
		return image;
	}
}
