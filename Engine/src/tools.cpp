#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include "tools.h"

namespace HelloEngine
{
	std::vector<char> GetBinaryFileContents(std::string const &filename) {

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

	std::array<float, 16> GetPerspectiveProjectionMatrix(float const aspect_ratio, float const field_of_view, float const near_clip, float const far_clip) {
		float f = 1.0f / tan(field_of_view * 0.5f * 0.01745329251994329576923690768489f);

		return{
			f / aspect_ratio,
			0.0f,
			0.0f,
			0.0f,

			0.0f,
			f,
			0.0f,
			0.0f,

			0.0f,
			0.0f,
			(near_clip + far_clip) / (near_clip - far_clip),
			-1.0f,

			0.0f,
			0.0f,
			(2.0f * near_clip * far_clip) / (near_clip - far_clip),
			0.0f
		};
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
}
