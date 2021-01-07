/***************************************************************************
 *
 * krt - Kens Raytracer - Coursework Edition. (C) Copyright 1997-2019.
 *
 * Do what you like with this code as long as you retain this comment.
 */

#pragma once

#include "vector.h"

class Vertex {
public:
	float x;
	float y;
	float z;
	float w;

	Vertex()
	{
		x = 0.0;
		y = 0.0;
		z = 0.0;
		w = 1.0f;
	}

	Vertex(float px, float py, float pz, float pw)
	{
		x = px;
		y = py;
		z = pz;
		w = pw;
	}

	Vertex(float px, float py, float pz)
	{
		x = px;
		y = py;
		z = pz;
		w = 1.0f;
	}

	// Return distance squared from given point to this point
	float dist(Vertex &b){
		return pow(b.x - x, 2) + pow(b.y - y, 2) + pow(b.z - z, 2);
	}
};
