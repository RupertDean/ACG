/***************************************************************************
 *
 * krt - Kens Raytracer - Coursework Edition. (C) Copyright 1997-2019.
 *
 * Do what you like with this code as long as you retain this comment.
 */

/* This is the entry point function for the program you need to create for lab two.
 * You should not need to modify this code.
 * It creates a framebuffer, loads an triangle mesh object, calls the drawing function to render the object and then outputs the framebuffer as a ppm file.
 *
 * On linux.bath.ac.uk:
 *
 * Compile the code using g++ -o lab4executable main_lab4.cpp framebuffer.cpp polymesh.cpp sphere.cpp spotLight.cpp phong.cpp -lm
 *
 * Execute the code using ./lab4executable
 *
 * This will produce an image file called test.ppm. You can convert this a png file for viewing using
 *
 * pbmropng test.ppm > test.png
 *
 * You are expected to fill in the missing code in polymesh.cpp.
 */

#include "framebuffer.h"
#include "ray.h"
#include "hit.h"
#include "polymesh.h"
#include "sphere.h"
#include "light.h"
#include "directionalLight.h"
#include "spotLight.h"
#include "material.h"
#include "phong.h"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <random>
#include <memory>

//kd-tree by Xavier Holt, available at github.com/xavierholt/kd
#include "kdTree/src/tree.h"

using namespace std;

class Photon {
  public:
    Vertex position;
    Vector incoming;
    Object* what;
    Vector normal;
    Colour intensity;
    char type;
};

//setup kd-tree
struct CORE {
    typedef std::shared_ptr<Photon> Item; // The item stored is a shared photon pointer
    typedef Vertex Point; // The type we read coordinates from.
    typedef float Coord; // The type of individual coordinates.

    static const int DIMENSIONS =  3; // We're in a three-dimensional space.
    static const int MAX_DEPTH  = 10; // The tree will reach at most ten levels.
    static const int STORAGE    =  1; //8; // Leaves can hold eight items before splitting.

    // Get the distance of a point along the given axis.
    static Coord coordinate(const Point& point, int axis) {
        if(axis == 0) return point.x;
        if(axis == 1) return point.y;
        if(axis == 2) return point.z;
        return point.x;
    }

    // Get the location of an item
    static const Point& point(const Item& item) {
        return item->position;
    }
};

// Random number distributions and generator seeds
std::mt19937 randomGenerator(0);
std::mt19937 rouletteGenerator(1);
std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
std::uniform_real_distribution<float> roulette(0.0, 1.0f);

// KD tree setup
Vertex minPoint(-30,-20,-10);
Vertex maxPoint(30,30,40);
KD::Tree<struct CORE> causticTree(minPoint, maxPoint);
KD::Tree<struct CORE> globalTree(minPoint, maxPoint);

void objectTest(Ray ray, Object *objects, Hit &bestHit) {
  Object *obj = objects;

  bestHit.flag = false;

  while(obj != 0) {
    Hit objHit;
    objHit.flag=false;

    obj->intersection(ray, objHit);

    if (objHit.flag) {
      if (objHit.t > 0.0f) {
        if (bestHit.flag == false) {
          bestHit = objHit;
        }
        else if (objHit.t < bestHit.t) {
          bestHit = objHit;
        }
      }
    }

    obj = obj->next;
  }

  return;
}

void traceGlobalPhotons(Ray ray, Hit& bestHit, Object* objects, std::shared_ptr<Photon>& photon, char type) {
  // Check if given ray intersects with anything
  objectTest(ray, objects, bestHit);

  if(bestHit.flag){
    //If there is a hit, update the photon's fields and add to tree
    photon->what = bestHit.what;
    photon->position = bestHit.position;
    photon->incoming = ray.direction;
    photon->type = type;
    photon->normal = bestHit.normal;

    globalTree.insert(photon);

    // Generate random number between 0 and 1
    float random = roulette(rouletteGenerator);

    // Calculate probabilities based on material properties
    float pReflection = bestHit.what->material->max();
    float pDiffuse = bestHit.what->material->Pd();
    pDiffuse *= pReflection;
    float pSpecular = pReflection - pDiffuse;
    float pAbsorption = 1 - pReflection;

    // Cast shadow photon
    Ray shadowRay;
    Hit shadowHit;
    shadowRay.direction = ray.direction;
    shadowRay.direction.normalise();
    shadowRay.position.x = bestHit.position.x + (1e-4f * shadowRay.direction.x);
    shadowRay.position.y = bestHit.position.y + (1e-4f * shadowRay.direction.y);
    shadowRay.position.z = bestHit.position.z + (1e-4f * shadowRay.direction.z);
    objectTest(shadowRay, objects, shadowHit);
    //If the shadow hits something that isn't transparent, create and store
    if(shadowHit.flag && !shadowHit.what->material->refraction) {
        std::shared_ptr<Photon> shadowPhoton = std::make_shared<Photon>();
        shadowPhoton->position = shadowHit.position;
        shadowPhoton->incoming = shadowRay.direction;
        shadowPhoton->what = shadowHit.what;
        shadowPhoton->type = 'S';
        shadowPhoton->intensity = Colour(0,0,0,0);
        globalTree.insert(shadowPhoton);
    }

    // If photon is reflected
    if(random < pReflection){
      // If photon is diffusely reflected
      if(random < pDiffuse){
        // Create new ray
        Ray reflectedRay;
        Hit reflectedHit;

        // Generate random direction in the hemisphere in front of the light direction
        do{
          reflectedRay.direction.x = dis(randomGenerator);
          reflectedRay.direction.y = dis(randomGenerator);
          reflectedRay.direction.z = dis(randomGenerator);
        } while (reflectedRay.direction.dot(bestHit.normal) < 0.5f);
        reflectedRay.direction.normalise();

        // Update position with new direction
        reflectedRay.position.x = bestHit.position.x + (1e-4f * reflectedRay.direction.x);
        reflectedRay.position.y = bestHit.position.y + (1e-4f * reflectedRay.direction.y);
        reflectedRay.position.z = bestHit.position.z + (1e-4f * reflectedRay.direction.z);

        // Create new photon and scale colour
        std::shared_ptr<Photon> reflectedPhoton = std::make_shared<Photon>();
        Colour hitColour = bestHit.what->material->getDiffuse();
        reflectedPhoton->intensity = photon->intensity;
        reflectedPhoton->intensity.scale(hitColour);
        reflectedPhoton->intensity.scale(1.0f/pDiffuse);

        // Recursively trace photon
        traceGlobalPhotons(reflectedRay, reflectedHit, objects, reflectedPhoton, 'I');
        return;
      }
      // Else if photon is reflected specularly and material supports is
      else if(random < pSpecular + pDiffuse && bestHit.what->material->SpecularBool()){
        // Create reflected ray
        Ray reflectedRay;
        Hit reflectedHit;
        bestHit.normal.reflection(ray.direction, reflectedRay.direction);
        reflectedRay.direction.normalise();

        // Update position with reflected direction
        reflectedRay.position.x = bestHit.position.x + (1e-4f * reflectedRay.direction.x);
        reflectedRay.position.y = bestHit.position.y + (1e-4f * reflectedRay.direction.y);
        reflectedRay.position.z = bestHit.position.z + (1e-4f * reflectedRay.direction.z);

        // Create new photon and scale colour
        std::shared_ptr<Photon> reflectedPhoton = std::make_shared<Photon>();
        Colour hitColour = bestHit.what->material->getSpecular();
        reflectedPhoton->intensity = photon->intensity;
        reflectedPhoton->intensity.scale(hitColour);
        reflectedPhoton->intensity.scale(1.0f/pSpecular);

        // Recursively trace photon
        traceGlobalPhotons(reflectedRay, reflectedHit, objects, reflectedPhoton, 'I');
        return;
      }
    }
    // Else if the photon is not absorbed
    else if(random < pAbsorption){
      // If photon is refracted
      if(bestHit.what->material->refraction){
        //Create refracted ray
        Ray refractedRay;
        Hit refractedHit;
        // Refract assuming glass, could be changed to use bestHit.what->material->returnN()
        refractedRay.direction = ray.direction.refraction(bestHit.normal, 1.52f);
        refractedRay.direction.normalise();

        // If TIR then return
        if(refractedRay.direction.x == 0.0f and refractedRay.direction.y == 0.0f and refractedRay.direction.z == 0.0f) return;

        // Update position with refracted ray
        refractedRay.position.x = bestHit.position.x + (1e-4f * refractedRay.direction.x);
        refractedRay.position.y = bestHit.position.y + (1e-4f * refractedRay.direction.y);
        refractedRay.position.z = bestHit.position.z + (1e-4f * refractedRay.direction.z);

        // Create photon and copy intensity from previous photon
        std::shared_ptr<Photon> refractedPhoton = std::make_shared<Photon>();
        refractedPhoton->intensity = photon->intensity;

        // Recursively trace photon
        traceGlobalPhotons(refractedRay, refractedHit, objects, refractedPhoton, 'I');
        return;
      }
    }
    // Else if photon is not refracted
    else{
      return;
    }
  }
  // Else if photon is absorbed
  else{
    return;
  }
}

void castGlobalPhotons(Object* objects, Light* lights, int photons) {
    printf("Creating Global Photon Map (%d photons)\n", photons);

    for(int i = 0; i < photons; i++) {
      if(i % (photons / 10) == 0) printf("-");
      Light* light = lights;
      while(light != (Light *)0){
        Hit bestHit;
        Ray ray;

        // Get position and direction of light
        ray.position = light->getPosition();
        Vector ldir = light->returnDirection();
        ldir.normalise();

        // Generate random direction in the hemisphere in front of the light
        do {
          ray.direction.x = dis(randomGenerator);
          ray.direction.y = dis(randomGenerator);
          ray.direction.z = dis(randomGenerator);
          ray.direction.normalise();
        } while(ray.direction.dot(ldir) < 0.0f);


        // Create photon and set intensity
        std::shared_ptr<Photon> photon = std::make_shared<Photon>();
        photon->what = nullptr;

        // Set power and create colour
        float power = 1200.0f / photons;
        photon->intensity = Colour(power, power, power, power);

        // Trace photon until absorbed or miss
        traceGlobalPhotons(ray, bestHit, objects, photon, 'D');

        light = light->next;
      }
    }
    // Print size of the tree
    printf("\nGlobal Photon Map Created!\n");
    auto nearest = globalTree.within(Vertex(0, 0, 0), 100.0f);
    printf("%d\n", nearest.size());
}

void traceCausticPhotons(Ray ray, Hit& bestHit, Object* objects, std::shared_ptr<Photon>& photon, char type) {
  // Check if given ray intersects with anything
  objectTest(ray, objects, bestHit);

  if(bestHit.flag){
    //If there is a hit, update the photon's fields and add to tree
    photon->what = bestHit.what;
    photon->position = bestHit.position;
    photon->incoming = ray.direction;
    photon->type = type;
    photon->normal = bestHit.normal;

    // Generate random number between 0 and 1
    float p = roulette(rouletteGenerator);

    // Calculate probabilities based on material properties
    float pReflection = bestHit.what->material->max();
    float pDiffuse = bestHit.what->material->Pd();
    pDiffuse *= pReflection;
    float pSpecular = pReflection - pDiffuse;

    // If material supports refraction
    if(bestHit.what->material->refraction){
      // Create refracted ray
      Ray refractedRay;
      Hit refractedHit;
      refractedRay.direction = ray.direction.refraction(bestHit.normal, 1.52f);
      refractedRay.direction.normalise();

      // If TIR then return
      if(refractedRay.direction.x == 0.0f and refractedRay.direction.y == 0.0f and refractedRay.direction.z == 0.0f) return;

      // Update position with refracted ray
      refractedRay.position.x = bestHit.position.x + (1e-4f * refractedRay.direction.x);
      refractedRay.position.y = bestHit.position.y + (1e-4f * refractedRay.direction.y);
      refractedRay.position.z = bestHit.position.z + (1e-4f * refractedRay.direction.z);

      // Create new photon and set intensity
      std::shared_ptr<Photon> refractedPhoton = std::make_shared<Photon>();
      refractedPhoton->intensity = photon->intensity;

      // Recursively trace photon
      traceCausticPhotons(refractedRay, refractedHit, objects, refractedPhoton, 'I');
    }
    // Else if the photon should be reflected
    else if(bestHit.what->material->refraction && bestHit.what->material->SpecularBool()){
      // Create reflected ray
      Ray reflectedRay;
      Hit reflectedHit;
      bestHit.normal.reflection(ray.direction, reflectedRay.direction);
      reflectedRay.direction.normalise();

      // Update position with reflected ray
      reflectedRay.position.x = bestHit.position.x + (1e-4f * reflectedRay.direction.x);
      reflectedRay.position.y = bestHit.position.y + (1e-4f * reflectedRay.direction.y);
      reflectedRay.position.z = bestHit.position.z + (1e-4f * reflectedRay.direction.z);

      // Create new photon and scale colour
      std::shared_ptr<Photon> reflectedPhoton = std::make_shared<Photon>();
      Colour hitColour = bestHit.what->material->getSpecular();
      reflectedPhoton->intensity = photon->intensity;
      reflectedPhoton->intensity.scale(hitColour);

      // Trace recursively
      traceCausticPhotons(reflectedRay, reflectedHit, objects, reflectedPhoton, 'I');
    }
    // If photon has refracted or reflected at least once before
    else if(type != 'D'){
      // If material is diffuse
      if(!bestHit.what->material->SpecularBool()){
        // Add to caustic tree
        causticTree.insert(photon);
      }
    }
  }
  return;
}

void castCausticPhotons(Object* objects, Light* lights, int photons) {
  printf("Creating Caustic Photon Map (%d photons)\n", photons);

  Light* light = lights;
  while(light != (Light *)0) {
    Object* object = objects;
    for(int i = 0; i < photons; i++) {
      if(light == nullptr) break;
      if(i % (photons / 10) == 0) printf("-");

      Hit hit;
      Hit bestHit;
      Ray ray;

      ray.position = light->getPosition();
      Vector ldir = light->returnDirection();
      ldir.normalise();

      bool causticIntersection = false;

      // Generate random direction
      do {
        ray.direction.x = dis(randomGenerator);
        ray.direction.y = dis(randomGenerator);
        ray.direction.z = dis(randomGenerator);
        ray.direction.normalise();

        objectTest(ray, objects, hit);
        if(hit.flag){
          if(hit.what->material->reflection || hit.what->material->refraction){
            causticIntersection = true;
          }
        }

        // Only break loop if ray hits a specular or transparent surface in front of the light
        if(ray.direction.dot(ldir) > 0.707f && causticIntersection){
          break;
        }
      } while(1);

      //Create new photon
      std::shared_ptr<Photon> photon = std::make_shared<Photon>();
      photon->what = nullptr;

      // Set photon colour
      float power = 500.0f / photons;
      photon->intensity = Colour(power, power, power, power);

      // Trace caustic photon until miss
      traceCausticPhotons(ray, bestHit, objects, photon, 'D');
    }
    light = light->next;
  }

  // Print caustic tree size
  printf("\nCaustic Photon Map Created!\n");
  auto nearest = causticTree.within(Vertex(0, 0, 0), 100.0f);
  printf("%d\n", nearest.size());
}

void raytrace(Ray ray, Object *objects, Light *lights, Colour &colour, int recursions) {
  // If recursions is zero
  if(recursions < 1){
    return;
  }

  // Create hit and test for intersection
  Hit bestHit;
  objectTest(ray, objects, bestHit);

  // If intersection
  if(bestHit.flag){
    // Add ambient colour
    bestHit.what->material->computeBaseColour(colour);
    // Set depth
    colour.d = bestHit.t;
    Light *light = lights;

    while (light != (Light *)0){
      // Get direction of light and direction to camera
      Vector viewer;
      Vector ldir = light->returnDirection();

      viewer.x = -bestHit.position.x;
      viewer.y = -bestHit.position.y;
      viewer.z = -bestHit.position.z;
      viewer.normalise();

      // Create lit and set to true
      bool lit = true;

      if(lit){
        // Check if point is in shadow
        Hit shadowHit;
        Ray shadowRay;
        Vertex lightPosition = light->getPosition();

        shadowRay.direction.x = lightPosition.x - bestHit.position.x;
        shadowRay.direction.y = lightPosition.y - bestHit.position.y;
        shadowRay.direction.z = lightPosition.z - bestHit.position.z;

        shadowRay.position.x = bestHit.position.x + (0.0001f * shadowRay.direction.x);
        shadowRay.position.y = bestHit.position.y + (0.0001f * shadowRay.direction.y);
        shadowRay.position.z = bestHit.position.z + (0.0001f * shadowRay.direction.z);

        objectTest(shadowRay, objects, shadowHit);

        // If shadow hit
        if(shadowHit.flag == true){
          // If shadow is in front of the camera
          if (shadowHit.t > 0.0f){
            // Point is in shadow
            lit = false;
          }
        }
        else{
          lit = true;
        }
      }

      // If point is still lit
      if (lit) {
        Colour intensity;
        Colour lightColour;

        // Get light colour from light and intensity from BDRF
        light->getIntensity(bestHit.position, lightColour);
        bestHit.what->material->computeLightColour(viewer, bestHit.normal, ldir, intensity);

        // Scale and add light colour
        intensity.scale(lightColour);
        colour.add(intensity);
      }

      // Global photon radiance
      // If material at point is not transparent
      if(!bestHit.what->material->refraction) {
        // Find nearest 200 photons
        auto nearest = globalTree.nearest(bestHit.position, 200);
        Colour colourToAdd;
        float radius2 = 0.0f;

        if(nearest.size() > 0) {
          // For each photon
          for(auto& photon : nearest) {
            // If photons have hit the same object as the ray and their normals are similar
            if(photon->what == bestHit.what && photon->normal.dot(bestHit.normal) > 0.707f){
              // Calculate radius of the bounding sphere
              radius2 = std::max(radius2, photon->position.dist(bestHit.position));
              Colour col;

              // Calculate colour of photon from diffuse colour at best hit point
              bestHit.what->material->computeDiffuseColour(viewer, bestHit.normal, photon->incoming, col);

              // Scale and add to total so far
              col.scale(photon->intensity);
              colourToAdd.add(col);
            }
          }

          // Scale by area
          float area = M_PI * radius2;
          Colour scale(1.0f/area, 1.0f/area, 1.0f/area, 1.0f/area);
          colourToAdd.scale(scale);

          // Restrict values to between 0 and 1
          if (colourToAdd.r < 0.0f) { colourToAdd.r = 0.0f; }
          else if (colourToAdd.r > 1.0f) { colourToAdd.r = 1.0f; }
          if (colourToAdd.g < 0.0f) { colourToAdd.g = 0.0f; }
          else if (colourToAdd.g > 1.0f) { colourToAdd.g = 1.0f; }
          if (colourToAdd.b < 0.0f) { colourToAdd.b = 0.0f; }
          else if (colourToAdd.b > 1.0f) { colourToAdd.b = 1.0f; }

          // Add to colour
          colour.add(colourToAdd);
        }
      }

      // Caustic photon radiance
      // Find nearest 50 caustic photons
      auto nearest = causticTree.nearest(bestHit.position, 50);

      Colour colourToAdd;
      float radius2 = 0.0f;
      if(nearest.size() > 0) {
        // For each photon
        for(auto& photon : nearest) {
          // If photon hits the same object and their normals are similar
          if(photon->what == bestHit.what && photon->normal.dot(bestHit.normal) > 0.707f){
            // If material is not transparent
            if(!bestHit.what->material->refraction){
              // Calculate radius of bounding sphere
              radius2 = std::max(radius2, photon->position.dist(bestHit.position));
              Colour col;

              // Get colour of hit material
              photon->what->material->computeBaseColour(col);

              // Scale and add to colour total
              col.scale(photon->intensity);
              colourToAdd.add(col);
            }
          }
        }

        // Scale by area
        float area = M_PI * radius2;
        Colour scale(1.0f/area, 1.0f/area, 1.0f/area, 1.0f/area);
        colourToAdd.scale(scale);

        // Restrict colour to between 0 and 1
        if (colourToAdd.r < 0.0f) { colourToAdd.r = 0.0f; }
        else if (colourToAdd.r > 1.0f) { colourToAdd.r = 1.0f; }
        if (colourToAdd.g < 0.0f) { colourToAdd.g = 0.0f; }
        else if (colourToAdd.g > 1.0f) { colourToAdd.g = 1.0f; }
        if (colourToAdd.b < 0.0f) { colourToAdd.b = 0.0f; }
        else if (colourToAdd.b > 1.0f) { colourToAdd.b = 1.0f; }

        // Add total to colour
        colour.add(colourToAdd);
      }

      // If material is reflective
      if(bestHit.what->material->reflection) {
        // Reflect ray
        Ray reflectedRay;
        // Calculate fresnel value
        float kr = ray.direction.fresnel(bestHit.normal, 1.52f);
        // Reflect direction
        bestHit.normal.reflection(ray.direction, reflectedRay.direction);
        reflectedRay.direction.normalise();

        // Update position based on reflected direction
        reflectedRay.position.x = bestHit.position.x + (1e-4f * reflectedRay.direction.x);
        reflectedRay.position.y = bestHit.position.y + (1e-4f * reflectedRay.direction.y);
        reflectedRay.position.z = bestHit.position.z + (1e-4f * reflectedRay.direction.z);

        // Create new colour
        Colour refColour;

        // Recursively trace ray
        raytrace(reflectedRay, objects, lights, refColour, recursions-1);

        // Scale and add colour
        refColour.scale(kr);
        colour.add(refColour);
      }

      // If material is refractive
      if(bestHit.what->material->refraction) {
        // Refract ray
        Ray refractedRay;
        // Calculate fresnel value
        float kt = 1 - ray.direction.fresnel(bestHit.normal, 1.52f);
        // Refract direction
        refractedRay.direction = ray.direction.refraction(bestHit.normal, 1.52f);
        refractedRay.direction.normalise();

        // If TIR then return
        if(refractedRay.direction.x == 0.0f and refractedRay.direction.y == 0.0f and refractedRay.direction.z == 0.0f) return;

        // Update position based on direction
        refractedRay.position.x = bestHit.position.x + (1e-4f * refractedRay.direction.x);
        refractedRay.position.y = bestHit.position.y + (1e-4f * refractedRay.direction.y);
        refractedRay.position.z = bestHit.position.z + (1e-4f * refractedRay.direction.z);

        // Create new colour
        Colour refColour;

        // Recursively trace ray
        raytrace(refractedRay, objects, lights, refColour, recursions-1);

        // Scale and add colour
        refColour.scale(kt);
        colour.add(refColour);
      }

      light = light->next;
    }
  }
  // If ray doesnt hit then set colour to black and depth to 100
  else {
    colour.d = 100.0f;
    colour.r = 0.0f;
    colour.g = 0.0f;
    colour.b = 0.0f;
  }
}

int main(int argc, char *argv[]){
  // Rendering settings
  bool blurring = true;
  int recursions = 10;
  int photons = 50000;
  int causticPhotons = 10000;
  int width = 2048;
  int height = 2048;

  // Create framebuffer to write to
  FrameBuffer *fb = new FrameBuffer(width, height);

  // Transform for teapot
  Transform *transform = new Transform(0.707f*2.5f, 0.0f, 0.707f, 4.0f,
                                       0.0f, 2.5f, 0.0f, -10.0f,
                                       -0.707f, 0.0f, 0.707f*2.5f, 16.0f,
                                       0.0f, 0.0f, 0.0f, 1.0f);

  // Transform for other objects
  Transform *transform2 = new Transform(1.0f, 0.0f, 0.0f, 0.0f,
                                        0.0f, 1.0f, 0.0f, -10.0f,
                                        0.0f, 0.0f, 1.0f, 11.0f,
                                        0.0f, 0.0f, 0.0f, 1.0f);

  // Read in polymeshes and transform
  PolyMesh *pm = new PolyMesh((char *)"teapot.ply", transform);
  PolyMesh *pm2 = new PolyMesh((char *)"whiteWalls.ply", transform2);
  PolyMesh *pm3 = new PolyMesh((char *)"leftWall.ply", transform2);
  PolyMesh *pm4 = new PolyMesh((char *)"rightWall.ply", transform2);

  // Create Spheres
  Vertex vR;
  vR.x = -5.5f;  vR.y = 5.0f;  vR.z = 13.5f;
  Sphere *sphereR = new Sphere(vR, 1.5f);

  Vertex vG;
  vG.x = -2.6f;  vG.y = 5.0f;  vG.z = 13.5f;
  Sphere *sphereG = new Sphere(vG, 1.5f);

  Vertex vB;
  vB.x = -4.0f;  vB.y = 3.3f;  vB.z = 13.5f;
  Sphere *sphereB = new Sphere(vB, 1.5f);

  Vertex v;
  v.x = 6.0f;  v.y = 0.0f;  v.z = 16.0f;
  Sphere *sphere = new Sphere(v, 2.5f);

  Vertex v2;
  v2.x = -7.5f;  v2.y = -6.0f;  v2.z = 12.5f;
  Sphere *sphere2 = new Sphere(v2, 2.0f);

  Vertex v4;
  v4.x = -7.5f;  v4.y = -7.0f;  v4.z = 16.0f;
  Sphere *sphere4 = new Sphere(v4, 1.5f);

  // Create ray
  Ray ray;
  ray.position.x = 0.0001f;
  ray.position.y = 0.0f;
  ray.position.z = 0.0f;

  // Create spotlight
  SpotLight *pl = new SpotLight(Vector(0.0f, -1.0f, 1.0f),
  Colour(1.0f, 1.0f, 1.0f, 0.0f), Vertex(0.0f, 10.0f, 0.0f));
  pl->direction.normalise();

  // Define materials
  Phong bp1;
  bp1.ambient.r = 0.0f;  bp1.ambient.g = 0.0f;  bp1.ambient.b = 0.0f;
  bp1.diffuse.r = 0.0f;  bp1.diffuse.g = 0.0f;  bp1.diffuse.b = 0.0f;
  bp1.specular.r = 1.0f;  bp1.specular.g = 1.0f;  bp1.specular.b = 1.0f;
  bp1.power = 40.0f;  bp1.n = 1.0f;
  bp1.refraction = false;  bp1.reflection = true;

  Phong bpR;
  bpR.ambient.r = 1.0f;  bpR.ambient.g = 0.0f;  bpR.ambient.b = 0.0f;
  bpR.diffuse.r = 1.0f;  bpR.diffuse.g = 0.0f;  bpR.diffuse.b = 0.0f;
  bpR.specular.r = 1.0f;  bpR.specular.g = 1.0f;  bpR.specular.b = 1.0f;
  bpR.power = 40.0f;  bpR.n = 1.52f;
  bpR.refraction = false; bpR.reflection = true;

  Phong bpG;
  bpG.ambient.r = 0.0f;  bpG.ambient.g = 1.0f;  bpG.ambient.b = 0.0f;
  bpG.diffuse.r = 0.0f;  bpG.diffuse.g = 1.0f;  bpG.diffuse.b = 0.0f;
  bpG.specular.r = 1.0f;  bpG.specular.g = 1.0f;  bpG.specular.b = 1.0f;
  bpG.power = 40.0f;  bpG.n = 1.52f;
  bpG.refraction = false; bpG.reflection = true;

  Phong bpB;
  bpB.ambient.r = 0.0f;  bpB.ambient.g = 0.0f;  bpB.ambient.b = 1.0f;
  bpB.diffuse.r = 0.0f;  bpB.diffuse.g = 0.0f;  bpB.diffuse.b = 1.0f;
  bpB.specular.r = 1.0f;  bpB.specular.g = 1.0f;  bpB.specular.b = 1.0f;
  bpB.power = 40.0f;  bpB.n = 1.52f;
  bpB.refraction = false; bpB.reflection = true;

  Phong bp3;
  bp3.ambient.r = 1.0f;  bp3.ambient.g = 1.0f;  bp3.ambient.b = 1.0f;
  bp3.diffuse.r = 1.0f;  bp3.diffuse.g = 1.0f;  bp3.diffuse.b = 1.0f;
  bp3.specular.r = 0.0f;  bp3.specular.g = 0.0f;  bp3.specular.b = 0.0f;
  bp3.power = 40.0f;  bp3.n = 1.0f;
  bp3.refraction = false;  bp3.reflection = false;

  Phong bp4;
  bp4.ambient.r = 0.0f;  bp4.ambient.g = 0.0f;  bp4.ambient.b = 0.0f;
  bp4.diffuse.r = 0.0f;  bp4.diffuse.g = 0.0f;  bp4.diffuse.b = 0.0f;
  bp4.specular.r = 1.0f;  bp4.specular.g = 1.0f;  bp4.specular.b = 1.0f;
  bp4.power = 40.0f;  bp4.n = 1.3f;
  bp4.refraction = true;  bp4.reflection = true;

  Phong bp5;
  bp5.ambient.r = 0.001f;  bp5.ambient.g = 0.001f;  bp5.ambient.b = 0.001f;
  bp5.diffuse.r = 0.001f;  bp5.diffuse.g = 0.001f;  bp5.diffuse.b = 0.001f;
  bp5.specular.r = 0.9f;  bp5.specular.g = 0.9f;  bp5.specular.b = 0.9f;
  bp5.power = 70.0f;  bp5.n = 1.0f;
  bp5.refraction = false;  bp5.reflection = true;

  Phong bpRed;
  bpRed.ambient.r = 1.0f;  bpRed.ambient.g = 0.8f;  bpRed.ambient.b = 0.07f;
  bpRed.diffuse.r = 1.0f;  bpRed.diffuse.g = 0.8f;  bpRed.diffuse.b = 0.07f;
  bpRed.specular.r = 0.0f;  bpRed.specular.g = 0.0f;  bpRed.specular.b = 0.0f;
  bpRed.power = 40.0f;  bpRed.n = 1.0f;
  bpRed.refraction = false;  bpRed.reflection = false;

  Phong bpGreen;
  bpGreen.ambient.r = 0.93f;  bpGreen.ambient.g = 0.48f;  bpGreen.ambient.b = 0.91f;
  bpGreen.diffuse.r = 0.93f;  bpGreen.diffuse.g = 0.48f;  bpGreen.diffuse.b = 0.91f;
  bpGreen.specular.r = 0.0f;  bpGreen.specular.g = 0.0f;  bpGreen.specular.b = 0.0f;
  bpGreen.power = 40.0f;  bpGreen.n = 1.0f;
  bpGreen.refraction = false;  bpGreen.reflection = false;

  // Set object materials
  sphere->material = &bp5;
  sphere2->material = &bp4;
  sphereR->material = &bpR;
  sphereG->material = &bpG;
  sphereB->material = &bpB;
  sphere4->material = &bp1;
  pm->material = &bp4;
  pm2->material = &bp3;
  pm3->material = &bpRed;
  pm4->material = &bpGreen;

  // Link objects together
  pm->next = pm2;
  pm2->next = pm3;
  pm3->next = pm4;
  pm4->next = sphere;
  sphere->next = sphere2;
  sphere2->next = sphereR;
  sphereR->next = sphereG;
  sphereG->next = sphereB;
  sphereB->next = sphere4;

  // Cast global and caustic photons
  castGlobalPhotons(pm, pl, photons);
  castCausticPhotons(pm, pl, causticPhotons);

  int count = 0;
  // For each row of pixels
  for (int y = 0; y < height; y += 1){
    count += 1;
    if(count % 10 == 0){
      printf("Raycast %d\n", count);
    }
    // For each pixel in column
    for (int x = 0; x < width; x += 1){
      // Calculate direction
      float fx = (float)x/(float)width;
      float fy = (float)y/(float)height;

      ray.direction.x = (fx-0.5f);
      ray.direction.y = (0.5f-fy);
      ray.direction.z = 0.5f;
      ray.direction.normalise();

      // Create colour
      Colour colour;

      // Raytrace to get colour
      raytrace(ray, pm, pl, colour, recursions);

      // Plot colour and depth of pixel
      fb->plotPixel(x, y, colour.r, colour.g, colour.b);
      fb->plotDepth(x,y, colour.d);
    }
  }


  // If blurring
  if(blurring){
    // Create another framebuffer
    FrameBuffer *fb2 = new FrameBuffer(width, height);
    // Blur the image
    fb2->blur(fb);
    printf("Blurred\n");
    // Output to ppm
    fb2->writeRGBFile((char *)"test.ppm");
  }
  else{
    // Output to ppm
    fb->writeRGBFile((char *)"test.ppm");
  }
  // fb->writeDepthFile((char *)"depth.ppm");
  // Close safely
  return 0;
}
