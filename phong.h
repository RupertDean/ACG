/***************************************************************************
 *
 * krt - Kens Raytracer - Coursework Edition. (C) Copyright 1997-2018.
 *
 * Do what you like with this code as long as you retain this comment.
 */

// Phong is a child class of Material and implement the simple Phong
// surface illumination model.

#pragma once

#include "material.h"
#include "colour.h"
#include "vector.h"

class Phong : public Material {
public:
	Colour ambient;
	Colour diffuse;
	Colour specular;
	float  power;
	float  n;

	void computeBaseColour(Colour &result);
	void computeLightColour(Vector &viewer, Vector &normal, Vector &ldir, Colour &result);
	void computeDiffuseColour(Vector &viewer, Vector &normal, Vector &ldir, Colour &result);
	Colour getSpecular();
	Colour getDiffuse();
	float returnN();
	float max();
	float sum();
	float Pd();
	bool SpecularBool();
};
