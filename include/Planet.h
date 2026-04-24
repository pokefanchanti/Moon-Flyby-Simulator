#ifndef PLANET_H
#define PLANET_H

#include <glm/glm.hpp>
#include <vector>

struct Planet {   
    glm::vec3 position;
    glm::vec3 velocity;
    float mass;
    float radius;
    glm::vec3 color;
    float axialTilt;
    float rotationSpeedFactor;
    std::vector<glm::vec3> trail;
    unsigned int textureID = 0;
    unsigned int cloudTextureID = 0;
    unsigned int nightTextureID = 0;
    unsigned int specularMapID = 0;
};

#endif