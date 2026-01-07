#pragma once

#include "Renderer.h"
#include "Math.h"
#include "vector"

namespace dae {
	class Container;

	struct Vertex {
		Vector3 Position;
		ColorRGB Color;
		Vector2 Uv{};
		Vector3 Normal{};
		Vector3 Tangent{};
	};

	namespace soft {

		struct Vertex_Out
		{
			Vector3 worldPosition{};
			Vector4 position{};
			Vector3 positionScaled{};
			Vector2 xyNorm{};
			ColorRGB color{ colors::White };
			Vector2 uv{};
			Vector3 normal{};
			Vector3 tangent{};
			Vector3 viewDirection{};

			Vertex_Out& operator=(const Vertex& other) {
				color = other.Color;
				uv = other.Uv;
				normal = other.Normal;
				tangent = other.Tangent;
				worldPosition = other.Position;

				return *this;
			}
		};

		// Makes passing around triangle information easier
		struct Triangle {
			Vertex_Out v0;
			Vertex_Out v1;
			Vertex_Out v2;

			Vector3 triangleNormal;
		};

		enum class PrimitiveTopology
		{
			TriangleList,
			TriangleStrip
		};

		struct VS_OUT {
			// Flat
			const Container& container;
			// Varried
			Vector2 vTexCoord;
			Vector3 vNormal;
			Vector3 vTangent;
			Vector3 vViewDirection;
		};

		struct RenderSettings {
			enum ShadingMode {
				ObservedArea, Diffuse, Specular, Combined
			}; // len 4

			enum SamplingState {
				Point, Linear, Anisotropic
			};

			enum CullMode {
				Front, Back, None
			};

			bool hasRotation{ true };
			bool useNormalMap{ true };
			bool useDepth{ true };
			bool visualizeDepth{ false };
			bool showTriangleBounds{ false };
			bool useUniformClearColor{ false };
			bool drawFireFx{ true };
			SamplingState samplingState{ Linear };
			CullMode cullMode{ Back };

			ShadingMode shadingMode{ Combined };
		};
	}
}