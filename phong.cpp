/***************************************************************************
 *
 * krt - Kens Raytracer - Coursework Edition. (C) Copyright 1997-2018.
 *
 * Do what you like with this code as long as you retain this comment.
 */

#include "phong.h"

#include <math.h>

// A simple Phong based lighting model

void Phong::computeBaseColour(Colour &result){
	result.r += ambient.r;
	result.g += ambient.g;
	result.b += ambient.b;
}

void Phong::computeLightColour(Vector &viewer, Vector &normal, Vector &ldir, Colour &result){
	float diff;

	Vector tolight;
	Vector toviewer;

	result.r = 0.0f;
	result.g = 0.0f;
	result.b = 0.0f;

	tolight = ldir;
	tolight.negate();

	toviewer = viewer;
	toviewer.negate();

	diff = normal.dot(tolight);

	if (diff < 0.0f) // light is behind surface
	{
		return;
	}

	// diffuse

	result.r += diffuse.r * diff;
	result.g += diffuse.g * diff;
	result.b += diffuse.b * diff;

	// the specular component

	Vector r;

	normal.reflection(tolight, r);
	r.normalise();

	float h;

	h = r.dot(toviewer);

	if (h > 0.0f){
		float p = (float)pow(h, power);

		result.r += specular.r * p;
		result.g += specular.g * p;
		result.b += specular.b * p;
	}
}

void Phong::computeDiffuseColour(Vector &viewer, Vector &normal, Vector &ldir, Colour &result){

	float diff;

	Vector tolight;
	Vector toviewer;

	result.r = 0.0f;
	result.g = 0.0f;
	result.b = 0.0f;

	tolight = ldir;
	tolight.negate();

	toviewer = viewer;
	toviewer.negate();

	diff = normal.dot(tolight);

	if (diff < 0.0f) // light is behind surface
	{
		return;
	}

	// diffuse

	result.r += diffuse.r * diff;
	result.g += diffuse.g * diff;
	result.b += diffuse.b * diff;
}

// Return the specular or diffuse colours
Colour Phong::getSpecular(){
	return specular;
}

Colour Phong::getDiffuse(){
	return diffuse;
}

float Phong::returnN(){
	return n;
}

// Max function for probabilities
float Phong::max(){
	return std::max(std::max(diffuse.r + specular.r, diffuse.g + specular.g), diffuse.b + specular.b);
}

// Sum of all diffuse and specular components
float Phong::sum(){
	return diffuse.r + diffuse.g + diffuse.b + specular.r + specular.g + specular.b;
}

// Return the probability of diffuse reflection
float Phong::Pd(){
	return (diffuse.r + diffuse.g + diffuse.b) == 0 ? 0.0f : ((diffuse.r + diffuse.g + diffuse.b)/sum())*max();
}

// Return true if material has any specular values greater than zero
bool Phong::SpecularBool(){
	if(specular.r != 0.0f || specular.g != 0.0f || specular.b != 0.0f){
		return true;
	}
	return false;
}
