#ifndef TOOS_H
#define TOOLS_H
#pragma once
#include "image.h"
#include <vector>
#include <array>

namespace HelloEngine
{	
	std::vector<char> GetBinaryFileContents(const char *filenamefilename);
	std::array<float, 16> GetOrthographicProjectionMatrix(float const left_plane, float const right_plane, float const top_plane, float const bottom_plane, float const near_plane, float const far_plane);	
	Image GetImage(const char *filename);
}
#endif
