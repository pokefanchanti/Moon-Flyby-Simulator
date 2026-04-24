#include "../include/Physics.h"

void updatePhysics(std::vector<Planet>& solarSystem, float dt, float G) {
    // applying gravity
    for (size_t i = 0; i < solarSystem.size(); i++){
        if(i==2) continue; //skip the sun
        for (size_t j = 0; j < solarSystem.size(); j++){
            if(i==j) continue; // skip if same planet
            if(j==2) continue; //skip the sun
            if(i==0 && j==1) continue; //skip earthgetting pulled by moon
            
            glm::vec3 direction = solarSystem[j].position - solarSystem[i].position;
            float distance = glm::length(direction);

            // prevent divide by zero
            if (distance > 0.5f) {
                glm::vec3 normalizedDir = glm::normalize(direction);
                float force = (G * solarSystem[i].mass * solarSystem[j].mass) / (distance * distance);
                glm::vec3 acceleration = (normalizedDir * force) / solarSystem[i].mass;
                solarSystem[i].velocity += acceleration * dt;
            }
        }
    }

    // applying velocity to position
    // and udpating trails 
    for (size_t i = 0; i < solarSystem.size(); i++){
        solarSystem[i].position += solarSystem[i].velocity * dt;
        solarSystem[i].trail.push_back(solarSystem[i].position);
    }
}