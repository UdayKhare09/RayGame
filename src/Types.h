#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <VkBootstrap.h>

struct Uniforms {
    float resolution[2];
    float time;
    float sunEnabled;
    float cameraPos[3];
    float padding2;
    float cameraDir[3];
    float padding3;
};
