#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>
#include <utility>
#include <stdlib.h>

namespace HelloEngine
{
	class Image {
	public:
		Image() : m_Width{ 0 }, m_Height{ 0 }, m_Size{ 0 }, m_Data{ nullptr } {}
		Image(uint32_t width, uint32_t height, size_t size) :
			m_Width{width},
			m_Height{height},
			m_Size{size}, 
			m_Data{ new unsigned char[size] }
		{};

		Image(Image&& other)
		{
			*this = std::move(other);
		}

		Image& operator = (Image&& other)
		{
			if (this != &other) {
				m_Width = other.m_Width;
				m_Height = other.m_Height;
				m_Data = other.m_Data;
				m_Size = other.m_Size;
				other.m_Data = nullptr;
			}
			return *this;
		}

		unsigned char* GetData() const
		{
			return m_Data;
		}

		uint32_t GetWidth() const
		{
			return m_Width;
		}

		uint32_t GetHeight() const
		{
			return m_Height;
		}

		uint32_t GetSize() const
		{
			return m_Size;
		}

		bool HasData() const
		{
			return m_Data != nullptr && m_Width > 0 && m_Height > 0 && m_Size > 0;
		}

		~Image()
		{
			if (m_Data != nullptr)
			{
				delete[] m_Data;
			}
		}

	private:
		uint32_t m_Width, m_Height;
		size_t m_Size;
		unsigned char* m_Data;
	};
}
#endif
