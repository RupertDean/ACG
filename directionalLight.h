/***************************************************************************
 *
 * krt - Kens Raytracer - Coursework Edition. (C) Copyright 1997-2018.
 *
 * Do what you like with this code as long as you retain this comment.
 */

// DirectionaLight is a child class of Light and implements a light
// with constant value in a given direction. The light has no position
// and can be treated as infinitely far away.

#pragma once
#include "light.h"

class DirectionalLight : public Light {
public:
	Vector direction;
	Colour intensity;

	DirectionalLight(Vector dir, Colour col);

	bool getDirection(Vertex &surface, Vector &dir);
	bool returnDirection(Vector &dir);
	void getIntensity(Vertex &surface, Colour &intensity);

};
