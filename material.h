/***************************************************************************
 *
 * krt - Kens Raytracer - Coursework Edition. (C) Copyright 1997-2018.
 *
 * Do what you like with this code as long as you retain this comment.
 */

// Material is the base class for materials.

#pragma once

#include "vector.h"
#include "colour.h"

class Material{
public:
	bool reflection;
	bool refraction;
	float n;

	virtual void computeBaseColour(Colour &result){
		result.r = 0.0f;
		result.g = 0.0f;
		result.b = 0.0f;
	}

	virtual void computeLightColour(Vector &viewer, Vector &normal, Vector &ldir, Colour &result){

		result.r = 0.0f;
		result.g = 0.0f;
		result.b = 0.0f;
	}

	// Diffuse light component used for photon mapping
	virtual void computeDiffuseColour(Vector &viewer, Vector &normal, Vector &ldir, Colour &result){

		result.r = 0.0f;
		result.g = 0.0f;
		result.b = 0.0f;
	}

	// Functions to return properties of the material
	virtual Colour getSpecular(){
		return Colour(0, 0, 0);
	}

	virtual Colour getDiffuse(){
		return Colour(0, 0, 0);
	}

	virtual float returnN(){
		return n;
	}

	virtual float max(){
		return 0.0f;
	}

	virtual float sum(){
		return 0.0f;
	}

	virtual float Pd(){
		return 0.0f;
	}

	// Boolean to show if material has specular values
	virtual bool SpecularBool(){
		return false;
	}

};
