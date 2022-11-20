#include "Texture.h"
#include "Vector2.h"
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ (uint32_t*)pSurface->pixels }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		//Load SDL_Surface using IMG_LOAD
		return new Texture{ IMG_Load(path.c_str()) };
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//Set x & y for later usage
		const int x = static_cast<int>(uv.x * m_pSurface->w);
		const int y = static_cast<int>(uv.y * m_pSurface->h);

		//Prepare color get & calculate pixel index on texture
		Uint32 pixel = m_pSurfacePixels[x + y * m_pSurface->w];
		Uint8 r{}, g{}, b{};

		//Get RGB color from texture
		SDL_GetRGB(pixel, m_pSurface->format, &r, &g, &b);

		//Convert to colorRGB
		const float maxColorValue = 255.f;
		return ColorRGB{ r / maxColorValue, g / maxColorValue, b / maxColorValue };
	}
}