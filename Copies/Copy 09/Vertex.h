#ifndef VERTEX_H
#define VERTEX_H

#include "stdfax.h"

struct Vertex
{
	Vertex(){}

	Vertex(float x, float y, float z, float r, float g, float b, float a): position(x, y, z), color(r, g, b, z) {}
	DirectX::XMFLOAT3 position{};
	DirectX::XMFLOAT4 color{};
};
#endif
