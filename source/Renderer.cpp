//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Texture.h"
#include "Utils.h"


#include <future>
#include <ppl.h>

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
	m_AspectRatio = (float)m_Width / (float)m_Height;
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START
	//Clears background
	SDL_FillRect(m_pBackBuffer, NULL, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100)); 
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	const int nrPixels{ m_Width * m_Height };
	std::fill_n(m_pDepthBufferPixels, nrPixels, FLT_MAX);

	std::vector<Vertex> vertices_world
	{
		
		//Triangle 0
		{{0.f,2.f,0.f}, {1,0,0}},
		{{1.5f,-1.f,0.f}, {1,0,0}},
		{{-1.5f,-1.f,0.f}, {1,0,0}},
		//Triangle 1
		{{0.f,4.f,2.f}, {1,0,0}},
		{{3.f,-2.f,2.f}, {0,1,0}},
		{{-3.f,-2.f,2.f}, {0,0,1}},
		
	};

	std::vector<Vertex> vertices_ndc{};

	VertexTransformationFunction(vertices_world, vertices_ndc);

	std::vector<Vector2> vertices_screenSpace;

	//Convert ndc's to screenspace
	for (Vertex& vertices : vertices_ndc)
	{
		Vector2 temp{};
		temp.x = (vertices.position.x +1) / 2 * m_Width;
		temp.y = (1 - vertices.position.y) / 2 * m_Height;
		vertices_screenSpace.push_back(temp);
	}

	for (size_t i = 0; i < vertices_screenSpace.size(); i += 3)
	{
		const Vector2 edgeV0V1 = vertices_screenSpace[i + 1] - vertices_screenSpace[i + 0];
		const Vector2 edgeV1V2 = vertices_screenSpace[i + 2] - vertices_screenSpace[i + 1];
		const Vector2 edgeV2V0 = vertices_screenSpace[i + 0] - vertices_screenSpace[i + 2];
		const float fullTriangleArea = Vector2::Cross(edgeV0V1, edgeV1V2);

		Vector2 minBoundingBox{ Vector2::Min(vertices_screenSpace[i + 0], Vector2::Min(vertices_screenSpace[i + 1], vertices_screenSpace[i + 2]))};
		Vector2 maxBoundingBox{ Vector2::Max(vertices_screenSpace[i + 0], Vector2::Max(vertices_screenSpace[i + 1], vertices_screenSpace[i + 2]))};
		minBoundingBox.Clamp(m_Width, m_Height);
		maxBoundingBox.Clamp(m_Width, m_Height);

		int maxX = (int)maxBoundingBox.x;
		int maxY = (int)maxBoundingBox.y;

		for (int px{ (int)minBoundingBox.x}; px < maxX; ++px)
		{
			for (int py{(int)minBoundingBox.y}; py < maxY; ++py)
			{
				ColorRGB finalColor{ .0f, .0f, .0f };
				const int index = px + py * m_Width;

				const Vector2 pointToSide = Vector2{ (float)px, (float)py };
				const Vector2 pointV0 = pointToSide - vertices_screenSpace[i + 0];
				const Vector2 pointV1 = pointToSide - vertices_screenSpace[i + 1];
				const Vector2 pointV2 = pointToSide - vertices_screenSpace[i + 2];

				//Check if pixel is in triangle
				float edge0 = Vector2::Cross(edgeV0V1, pointV0);
				if (edge0 < 0)
				{
					continue;
				}
				float edge1 = Vector2::Cross(edgeV1V2, pointV1);
				if (edge1 < 0)
				{
					continue;
				}
				float edge2 = Vector2::Cross(edgeV2V0, pointV2);
				if (edge2 < 0)
				{
					continue;
				}

				const float weightV0 = edge1 / fullTriangleArea;
				const float weightV1 = edge2 / fullTriangleArea;
				const float weightV2 = edge0 / fullTriangleArea;

				const float depth
				{
					weightV0 * (vertices_world[i + 0].position.z - m_Camera.origin.z) +
					weightV1 * (vertices_world[i + 1].position.z - m_Camera.origin.z) +
					weightV2 * (vertices_world[i + 2].position.z - m_Camera.origin.z)
				};

				if (m_pDepthBufferPixels[index] < depth)
				{
					continue;
				}
				m_pDepthBufferPixels[index] = depth;

				//Calculate color
				finalColor = { weightV0 * vertices_world[i + 0].color + weightV1 * vertices_world[i + 1].color + weightV2 * vertices_world[i + 2].color };

				//Update Color in Buffer
				finalColor.MaxToOne();

				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
		}
	}

	//RENDER LOGIC
	

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	vertices_out.reserve(vertices_in.size());
	for (Vertex v : vertices_in)
	{
		//Transfrom to camera matrix
		v.position = m_Camera.invViewMatrix.TransformPoint(v.position);

		//Perspective devide
		v.position.x = v.position.x / (m_AspectRatio * m_Camera.fov) / v.position.z;
		v.position.y = v.position.y / m_Camera.fov / v.position.z;

		vertices_out.emplace_back(v);
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}