#pragma once
#include <vector>
#include <array>

namespace HelloEngine
{	
	std::vector<char> GetBinaryFileContents(std::string const &filename);
	std::array<float, 16> GetPerspectiveProjectionMatrix(float const aspect_ratio, float const field_of_view, float const near_clip, float const far_clip);
	std::array<float, 16> GetOrthographicProjectionMatrix(float const left_plane, float const right_plane, float const top_plane, float const bottom_plane, float const near_plane, float const far_plane);
}