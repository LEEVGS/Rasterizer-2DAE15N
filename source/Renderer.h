#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include "Camera.h"
#include "DataTypes.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render();
		void ToggleDepthBuffer();

		bool SaveBufferToImage() const;

	private:
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};

		float* m_pDepthBufferPixels{};

		Camera m_Camera{};

		Texture* m_pTexture{ nullptr };

		int m_Width{};
		int m_Height{};
		float m_AspectRatio{};

		bool m_IsDepthBuffer{ false };

		std::vector<Mesh> m_MeshesWorld{};

		size_t m_VerticesCount{};
		Vertex* m_VerticesWorld;
		Vertex* m_VerticesNDC;
		Vector2* m_VerticesScreenSpace;
		//std::vector<Vertex> m_VerticesWorld{};
		//std::vector<Vertex> m_VerticesNDC{};
		//std::vector<Vector2> m_VerticesScreenSpace{};

		//Create meshes
		void CreateMeshes();
		void LoadMesh(const std::string& path);

		//Function that transforms the vertices from the mesh from World space to Screen space
		void VertexTransformationWorldToNDC();
		void VertexTransformationWorldToNDCNew(Mesh& mesh);

		bool IsVerticesInFrustrum(const Vertex_Out& vertex);

		//Draw traingles by using the index
		void DrawTriangle(int index, bool swapVertices, const Mesh& mesh);

		//Find size to reserve
		size_t FindReserveSize();
	};
}
