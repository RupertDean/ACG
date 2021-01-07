#pragma once
#include "light.h"

class SpotLight : public Light {
public:

  Vertex position;
	Vector direction;
	Colour intensity;

	SpotLight(Vector dir, Colour col, Vertex pos);
	bool getDirection(Vertex &surface, Vector &dir);
	Vector returnDirection();
	void getIntensity(Vertex &surface, Colour &intensity);
  Vertex getPosition();

};
