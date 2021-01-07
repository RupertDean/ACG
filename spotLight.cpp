// The spotlight object is a light which has a position and direction as opposed
// to directional lights which do not have a position

#include "spotLight.h"

SpotLight::SpotLight(Vector dir, Colour col, Vertex pos){
	next = (Light *)0;

  position = pos;
	direction = dir;
	direction.normalise();
	intensity = col;
}

// Direction from light to point
bool SpotLight::getDirection(Vertex &surface, Vector &dir){
	dir.x = surface.x - position.x;
	dir.y = surface.y - position.y;
	dir.z = surface.z - position.z;

	return true;
}

// Light direction
Vector SpotLight::returnDirection(){
	return direction;
}

// Light colour
void SpotLight::getIntensity(Vertex &surface, Colour &level){
	level = intensity;
}

// Light position
Vertex SpotLight::getPosition(){
	return position;
}
