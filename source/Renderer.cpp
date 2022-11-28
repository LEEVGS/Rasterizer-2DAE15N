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

//#define STRIP

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow)
	:m_pWindow(pWindow)
	, m_pTexture{ Texture::LoadFromFile("Resources/tuktuk.png") }
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_AspectRatio = (float)m_Width / (float)m_Height;
	m_Camera.Initialize(m_AspectRatio,60.f, { .0f,.0f,-10.f });

	//CreateMeshes();
	LoadMesh("Resources/tuktuk.obj");
	//m_MeshesWorld.push_back(Mesh{ {},{}, PrimitiveTopology::TriangleList });
	//Utils::ParseOBJ("Resources/tuktuk.obj", m_MeshesWorld[0].vertices, m_MeshesWorld[0].indices);
	//m_MeshesWorld[0].worldMatrix = Matrix::CreateScale({ 0.5f,0.5f,0.5f }) * Matrix::CreateTranslation(0.f, -3.f, 15.f);

	//Reserve max size
	const int reserveSize = FindReserveSize();
	m_VerticesCount = reserveSize;
	m_VerticesNDC = new Vertex[reserveSize];
	m_VerticesWorld = new Vertex[reserveSize];
	m_VerticesScreenSpace = new Vector2[reserveSize];
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete[] m_VerticesNDC;
	delete[] m_VerticesWorld;
	delete[] m_VerticesScreenSpace;
	delete m_pTexture;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	const float rotationSpeed = 1.f;
	for (Mesh& mesh : m_MeshesWorld)
	{
		mesh.worldMatrix = Matrix::CreateRotationY((rotationSpeed * pTimer->GetElapsed())) * mesh.worldMatrix;
	}
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

	//Loop over every mesh
	for (Mesh& mesh : m_MeshesWorld)
	{
		//Create vertices from indices
		for (size_t i = 0; i < mesh.indices.size(); ++i)
		{
			m_VerticesWorld[i] = mesh.vertices[mesh.indices[i]];
		}

		//Set maximum read count
		m_VerticesCount = mesh.indices.size();

		//VertexTransformationWorldToNDC();
		VertexTransformationWorldToNDCNew(mesh);

		//Convert ndc's to screenspace
		//for (size_t i = 0; i < m_VerticesCount; i++)
		//{
		//	Vector2 temp{};
		//	temp.x = (m_VerticesNDC[i].position.x + 1) / 2 * m_Width;
		//	temp.y = (1 - m_VerticesNDC[i].position.y) / 2 * m_Height;
		//	m_VerticesScreenSpace[i] = temp;
		//}
		for (size_t i = 0; i < m_VerticesCount; i++)
		{
			Vector2 temp{};
			temp.x = (mesh.vertices_out[i].position.x + 1) / 2 * m_Width;
			temp.y = (1 - mesh.vertices_out[i].position.y) / 2 * m_Height;
			m_VerticesScreenSpace[i] = temp;
		}


		//RENDER LOGIC
		switch (mesh.primitiveTopology)
		{
		case PrimitiveTopology::TriangleList:
		{
			for (int i = 0; i < m_VerticesCount; i += 3)
			{
				DrawTriangle(i, false, mesh);
			}
		}
		break;
		case PrimitiveTopology::TriangleStrip:
		{
			for (int i = 0; i < m_VerticesCount - 2; ++i)
			{
				DrawTriangle(i, i % 2, mesh);
			}
		}
		break;
		}
	}
	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}

void dae::Renderer::ToggleDepthBuffer()
{
	m_IsDepthBuffer = !m_IsDepthBuffer;
}

void dae::Renderer::CreateMeshes()
{
#ifdef STRIP
	m_MeshesWorld =
	{
		Mesh{
			{
				Vertex{{ -3.f, 3.f, -2.f }, {colors::White} , {0.0f, 0.0f}},
				Vertex{{ 0.f, 3.f, -2.f },{colors::White} , { 0.5f, 0.0f }},
				Vertex{{ 3.f, 3.f, -2.f },{colors::White} , { 1.0f, 0.0f }},
				Vertex{{ -3.f, 0.f, -2.f },{colors::White} , { 0.0f, 0.5f }},
				Vertex{{ 0.f, 0.f, -2.f }, {colors::White} ,{ 0.5f, 0.5f }},
				Vertex{{ 3.f, 0.f, -2.f },{colors::White} , { 1.0f, 0.5f }},
				Vertex{{ -3.f, -3.f, -2.f }, {colors::White} ,{ 0.0f, 1.0f }},
				Vertex{{ 0.f, -3.f, -2.f },{colors::White} , { 0.5f, 1.0f }},
				Vertex{{ 3.f, -3.f, -2.f },{colors::White} , { 1.0f, 1.0f }},
			},
			{
				3,0,4,1,5,2,
				2,6,
				6,3,7,4,8,5
			},
			PrimitiveTopology::TriangleStrip
		}
	};
#else
	m_MeshesWorld =
	{
		Mesh{
			{
				Vertex{{ -3.f, 3.f, -2.f }},
				Vertex{{ 0.f, 3.f, -2.f }},
				Vertex{{ 3.f, 3.f, -2.f }},
				Vertex{{ -3.f, 0.f, -2.f }},
				Vertex{{ 0.f, 0.f, -2.f }},
				Vertex{{ 3.f, 0.f, -2.f }},
				Vertex{{ -3.f, -3.f, -2.f }},
				Vertex{{ 0.f, -3.f, -2.f }},
				Vertex{{ 3.f, -3.f, -2.f }},
			},
			{
				3,0,1,        1,4,3,        4,1,2,
				2,5,4,        6,3,4,        4,7,6,
				7,4,5,        5,8,7
			},
			PrimitiveTopology::TriangleList
		}
	};
#endif
}

void Renderer::LoadMesh(const std::string& path)
{
	//Create empty mesh
	m_MeshesWorld.push_back(Mesh{ {},{}, PrimitiveTopology::TriangleList });

	//Load mesh
	Utils::ParseOBJ(path, m_MeshesWorld[m_MeshesWorld.size()-1].vertices, m_MeshesWorld[m_MeshesWorld.size() - 1].indices);

	//Set values for matrix
	const Vector3 translation = { m_Camera.origin + Vector3{ 0.0f, -3.f, 15.f } };
	const Vector3 rotation = { 0.f, 0.f, 0.f };
	const Vector3 scale = { 0.5f, 0.5f, 0.5f };

	//Generate and apply matrix
	m_MeshesWorld[m_MeshesWorld.size() - 1].worldMatrix = Matrix::CreateScale(scale) * Matrix::CreateRotation(rotation) * Matrix::CreateTranslation(translation);
}

void Renderer::VertexTransformationWorldToNDC()
{
	Vertex v{};
	for (int i = 0; i < m_VerticesCount; i++)
	{
		v = m_VerticesWorld[i];

		//Transfrom to camera matrix
		v.position = m_Camera.invViewMatrix.TransformPoint(v.position);

		//Perspective devide
		v.position.x = v.position.x / (m_AspectRatio * m_Camera.fov) / v.position.z;
		v.position.y = v.position.y / m_Camera.fov / v.position.z;

		m_VerticesNDC[i] = v;
	}
}

void dae::Renderer::VertexTransformationWorldToNDCNew(Mesh& mesh)
{
	const Matrix matrix = mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix;

	Vertex_Out v{};
	mesh.vertices_out.clear();
	mesh.vertices_out.reserve(m_VerticesCount);
	for (int i = 0; i < m_VerticesCount; i++)
	{
		v = { Vector4{}, m_VerticesWorld[i].color, m_VerticesWorld[i].uv, m_VerticesWorld[i].normal, m_VerticesWorld[i].tangent };

		//Transfrom to camera matrix
		v.position = matrix.TransformPoint({ m_VerticesWorld[i].position, 1 });

		//Perspective devide
		v.position.x /= v.position.w;
		v.position.y /= v.position.w;
		v.position.z /= v.position.w;

		mesh.vertices_out.emplace_back(v);
	}
}

bool dae::Renderer::IsVerticesInFrustrum(const Vertex_Out& vertex)
{
	if (vertex.position.x < -1.f || vertex.position.x > 1.f)
	{
		return false;
	}
	if (vertex.position.y < -1.f || vertex.position.y > 1.f)
	{
		return false;
	}
	if (vertex.position.z < 0.f || vertex.position.z > 1.f)
	{
		return false;
	}
	return true;
}

void dae::Renderer::DrawTriangle(int i, bool swapVertices, const Mesh& mesh)
{
	//Predefine indexes
	const uint32_t vertexIndex0 = i;
	const uint32_t vertexIndex1 = i + 1 * !swapVertices + 2 * swapVertices;
	const uint32_t vertexIndex2 = i + 2 * !swapVertices + 1 * swapVertices;

	//Calculate edges
	const Vector2 edgeV0V1 = m_VerticesScreenSpace[vertexIndex1] - m_VerticesScreenSpace[vertexIndex0];
	const Vector2 edgeV1V2 = m_VerticesScreenSpace[vertexIndex2] - m_VerticesScreenSpace[vertexIndex1];
	const Vector2 edgeV2V0 = m_VerticesScreenSpace[vertexIndex0] - m_VerticesScreenSpace[vertexIndex2];

	//Check if triangle is valid
	if (edgeV0V1.SqrMagnitude() < FLT_EPSILON || edgeV1V2.SqrMagnitude() < FLT_EPSILON || edgeV2V0.SqrMagnitude() < FLT_EPSILON)
	{
		return;
	}

	if (IsVerticesInFrustrum(mesh.vertices_out[vertexIndex0]) == false) { return; }
	if (IsVerticesInFrustrum(mesh.vertices_out[vertexIndex1]) == false) { return; }
	if (IsVerticesInFrustrum(mesh.vertices_out[vertexIndex2]) == false) { return; }

	const float fullTriangleArea = Vector2::Cross(edgeV0V1, edgeV1V2);

	//Create bounding box for optimized rendering
	Vector2 minBoundingBox{ Vector2::Min(m_VerticesScreenSpace[vertexIndex0], Vector2::Min(m_VerticesScreenSpace[vertexIndex1], m_VerticesScreenSpace[vertexIndex2])) };
	Vector2 maxBoundingBox{ Vector2::Max(m_VerticesScreenSpace[vertexIndex0], Vector2::Max(m_VerticesScreenSpace[vertexIndex1], m_VerticesScreenSpace[vertexIndex2])) };
	minBoundingBox.Clamp(m_Width, m_Height);
	maxBoundingBox.Clamp(m_Width, m_Height);
	const int offset = 1;
	int maxX = (int)maxBoundingBox.x + offset;
	int maxY = (int)maxBoundingBox.y + offset;

	//Loop over every pixel that matches the bounding box
	for (int px{ (int)minBoundingBox.x }; px < maxX; ++px)
	{
		for (int py{ (int)minBoundingBox.y }; py < maxY; ++py)
		{
			ColorRGB finalColor{ .0f, .0f, .0f };
			const int index = px + py * m_Width;

			const Vector2 pointToSide = Vector2{ (float)px, (float)py };
			const Vector2 pointV0 = pointToSide - m_VerticesScreenSpace[vertexIndex0];
			const Vector2 pointV1 = pointToSide - m_VerticesScreenSpace[vertexIndex1];
			const Vector2 pointV2 = pointToSide - m_VerticesScreenSpace[vertexIndex2];

			//Check if pixel is in triangle
			float edge0 = Vector2::Cross(edgeV0V1, pointV0);
			if (edge0 <= 0)
			{
				continue;
			}
			float edge1 = Vector2::Cross(edgeV1V2, pointV1);
			if (edge1 <= 0)
			{
				continue;
			}
			float edge2 = Vector2::Cross(edgeV2V0, pointV2);
			if (edge2 <= 0)
			{
				continue;
			}

			//Calculate the barycentric weight
			const float weightV0 = edge1 / fullTriangleArea;
			const float weightV1 = edge2 / fullTriangleArea;
			const float weightV2 = edge0 / fullTriangleArea;

			const float depthV0 = mesh.vertices_out[vertexIndex0].position.z;
			const float depthV1 = mesh.vertices_out[vertexIndex1].position.z;
			const float depthV2 = mesh.vertices_out[vertexIndex2].position.z;

			const float interpolatedDepth
			{
				1.0f /
				(weightV0 / depthV0 +
				weightV1 / depthV1 +
				weightV2 / depthV2)
			};

			if (m_pDepthBufferPixels[index] < interpolatedDepth)
			{
				continue;
			}

			m_pDepthBufferPixels[index] = interpolatedDepth;

			if (m_IsDepthBuffer)
			{
				
			}
			else
			{
				//Texture check?
				const float invInterpolatedDepthV0{ 1.f / mesh.vertices_out[vertexIndex0].position.w };
				const float invInterpolatedDepthV1{ 1.f / mesh.vertices_out[vertexIndex1].position.w };
				const float invInterpolatedDepthV2{ 1.f / mesh.vertices_out[vertexIndex2].position.w };

				const float interpolatedPixelDepth
				{
					1.f /
					(
						weightV0 * invInterpolatedDepthV0 +
						weightV1 * invInterpolatedDepthV1 +
						weightV2 * invInterpolatedDepthV2
					)
				};

				const Vector2 pixelUV
				{
					(weightV0 * mesh.vertices_out[vertexIndex0].uv * invInterpolatedDepthV0 +
					weightV1 * mesh.vertices_out[vertexIndex1].uv * invInterpolatedDepthV1 +
					weightV2 * mesh.vertices_out[vertexIndex2].uv * invInterpolatedDepthV2)
						* interpolatedPixelDepth
				};

				finalColor = { m_pTexture->Sample(pixelUV) };
			}
			//const Vector2 pixelUV
			//{
			//	(weightV0 * m_VerticesWorld[vertexIndex0].uv / depthV0 +
			//	weightV1 * m_VerticesWorld[vertexIndex1].uv / depthV1 +
			//	weightV2 * m_VerticesWorld[vertexIndex2].uv / depthV2)
			//		* interpolatedDepth
			//};

			//Calculate color
			//finalColor = { m_pTexture->Sample(pixelUV) };

			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}

size_t dae::Renderer::FindReserveSize()
{
	size_t max{};
	for (const Mesh& mesh : m_MeshesWorld)
	{
		max = std::max(max, mesh.indices.size());
	}
	return max;
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}