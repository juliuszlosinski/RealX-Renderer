#ifndef VERTEX_H
#define VERTEX_H

#include "stdfax.h"

struct Vertex
{
	Vertex(){}
	Vertex(float x, float y, float z, float u, float v): position(x, y, z), textureCordinates{u, v}{}
	DirectX::XMFLOAT3 position{};
	DirectX::XMFLOAT2 textureCordinates{};
};
#endif
