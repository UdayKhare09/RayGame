#pragma once
#include "Types.h"
#include <vector>

struct Sphere {
    Vec3 center;
    float radius;
    Vec3 color;
    float roughness;
};

struct PointLight {
    Vec3 position;
    float intensity;
    Vec3 color;
    float padding;
};

struct SpotLight {
    Vec3 position;
    float intensity;
    Vec3 direction;
    float cutOff; // Cosine of angle
    Vec3 color;
    float outerCutOff;
};

struct Scene {
    std::vector<Sphere> spheres;
    PointLight pointLight;
    SpotLight spotLight;
    bool sunEnabled = true;
    Vec3 sunDirection = {0.5f, 1.0f, -0.5f};
    
    Scene() {
        // Default scene
        spheres.push_back({{0.0f, 0.0f, 0.0f}, 1.0f, {1.0f, 0.0f, 0.0f}, 0.5f}); // Red sphere
        spheres.push_back({{2.0f, 0.0f, 0.0f}, 1.0f, {0.0f, 1.0f, 0.0f}, 0.2f}); // Green sphere
        spheres.push_back({{-2.0f, 0.0f, 0.0f}, 1.0f, {0.0f, 0.0f, 1.0f}, 0.8f}); // Blue sphere
        spheres.push_back({{0.0f, -101.0f, 0.0f}, 100.0f, {0.5f, 0.5f, 0.5f}, 1.0f}); // Floor

        pointLight = {{0.0f, 5.0f, 0.0f}, 1.0f, {1.0f, 1.0f, 1.0f}, 0.0f};
        spotLight = {{0.0f, 5.0f, 2.0f}, 2.0f, {0.0f, -1.0f, 0.0f}, 0.9f, {1.0f, 1.0f, 0.0f}, 0.8f};
    }
};
