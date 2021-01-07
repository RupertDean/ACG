/***************************************************************************
 *
 * krt - Kens Raytracer - Coursework Edition. (C) Copyright 1997-2018.
 *
 * Do what you like with this code as long as you retain this comment.
 */

#include "directionalLight.h"
DirectionalLight::DirectionalLight(Vector dir, Colour col){
	next = (Light *)0;

	direction = dir;
	direction.normalise();
	intensity = col;
}

bool DirectionalLight::getDirection(Vertex &surface, Vector &dir){
	dir = direction;

	return true;
}

bool DirectionalLight::returnDirection(Vector &dir){
	dir = direction;

	return true;
}

void DirectionalLight::getIntensity(Vertex &surface, Colour &level){
	level = intensity;
}
