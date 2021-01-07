/***************************************************************************
 *
 * krt - Kens Raytracer - Coursework Edition. (C) Copyright 1997-2019.
 *
 * Do what you like with this code as long as you retain this comment.
 */

#ifndef _VECTOR_H_
#define _VECTOR_H_

#include <math.h>
#include <cstdio>
#include "vertex.h"
#include <utility>
#include <algorithm>

class Vector {
public:
	float x;
	float y;
	float z;

	Vector(float px, float py, float pz){
		x = px;
		y = py;
		z = pz;
	}

	Vector(){
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
	}

	void normalise(){
		float len = (float)sqrt((double)(x*x + y*y + z*z));
		x = x / len;
		y = y / len;
		z = z / len;
	}

	float len_sqr(){
	  return(x*x + y*y + z*z);
	}

	float length(){
	  return(float)sqrt((double)(x*x + y*y + z*z));
	}

	// float dot(Vector &other){
	// 	return x*other.x + y*other.y + z*other.z;
	// }

	float dot(Vector other){
		return (x*other.x + y*other.y + z*other.z);
	}

	void negate(){
		x = -x;
		y = -y;
		z = -z;
	}

	// Reflect an incoming vector perfectly about this vector which is the normal and write to another vector
	void reflection(Vector initial, Vector &reflect){
		float d;

		d = dot(initial);
		d = d * 2.0f;

		reflect.x = initial.x - d * x;
		reflect.y = initial.y - d * y;
		reflect.z = initial.z - d * z;
	}

	// Perform refraction calculations and apply to fresnel formula given in lecture slides
	float fresnel(Vector normal, float ior){
		Vector n = normal;
		float cosi = dot(n);
		float etai = 1.0f;
		float etat = ior;

		if(cosi < 0){
			cosi = -cosi;
		} else{
			// std::swap(etai, etat);
			n.negate();
		}

		float sint = etai / etat * sqrtf(fmax(0.f, 1 - pow(cosi, 2)));
		if(sint >= 1){
			return 1.0f;
		}

		float eta = etai / etat;
		float cost = sqrtf(fmax(0.f, 1 - sint * sint));

		float rPar = ((eta * cosi) - cost) / ((eta * cosi) + cost);
		float rPer = (cosi - (eta * cost)) / (cosi + (eta * cost));

		float val = (pow(rPar, 2) + pow(rPer, 2)) / 2.0f;

		if(val > 1.0f){
			val = 1.0f;
		}
		else if(val < 0.0f){
			val = 0.0f;
		}

		return val;
	}

	// Refract using formula given in slides
	Vector refraction(Vector normal, float ior){
		float cosi = dot(normal);
    float etai = 1, etat = ior;
    Vector n = normal;
		// Check normal direction and flip if necessary
    if (cosi < 0) {
        cosi = -cosi;
    } else {
        std::swap(etai, etat);
        n.negate();
    }

    float eta = etai / etat;
    float k = 1 - eta * eta * (1 - cosi * cosi);
    return k < 0 ? Vector(0,0,0) : eta * (*this) + (eta * cosi - sqrtf(k)) * n;
  }

	void cross(Vector &other, Vector &result){
	  result.x = y*other.z - z*other.y;
	  result.y = z*other.x - x*other.z;
	  result.z = x*other.y - y*other.x;
	}

	void cross(Vector &other){
	  Vector result;
	  result.x = y*other.z - z*other.y;
	  result.y = z*other.x - x*other.z;
	  result.z = x*other.y - y*other.x;
	  x = result.x;
	  y = result.y;
	  z = result.z;
	}

	void add(Vector &other){
	  x += other.x;
	  y += other.y;
	  z += other.z;
	}

	void sub(Vector &other){
	  x -= other.x;
	  y -= other.y;
	  z -= other.z;
	}

	Vector& operator=(Vector other){
		x = other.x;
		y = other.y;
		z = other.z;
		return *this;
	}

	friend Vector operator+(const Vector &a, const Vector &b){
	  Vector t;
	  t.x = a.x + b.x;
	  t.y = a.y + b.y;
	  t.z = a.z + b.z;
	  return t;
	}

	friend Vector operator-(const Vector &a, const Vector &b){
	  Vector t;
	  t.x = a.x - b.x;
	  t.y = a.y - b.y;
	  t.z = a.z - b.z;
	  return t;
	}

  friend Vector operator*(const Vector &a, const Vector &b){
	  Vector t;
	  t.x = a.x * b.x;
	  t.y = a.y * b.y;
	  t.z = a.z * b.z;
	  return t;
	}

	friend Vector operator*(const float a, const Vector &b){
	  Vector t;
	  t.x = a * b.x;
	  t.y = a * b.y;
	  t.z = a * b.z;
	  return t;
	}

	friend bool operator==(const Vector &a, const Vector &b){
		if(a.x != b.x) return false;
		if(a.y != b.y) return false;
		if(a.z != b.z) return false;
		return true;
	}

	friend bool operator!=(const Vector &a, const Vector &b){
		return !(a == b);
	}

};

#endif
