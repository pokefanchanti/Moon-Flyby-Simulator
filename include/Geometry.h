#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <vector>

void generateSphere(float radius, int sectors, int stacks, std::vector<float> &vertices, std::vector<unsigned int> &indices);

void generateRocketGeometry(float radius, float bodyHeight, float coneHeight, int sectors, 
                            std::vector<float> &vertices, std::vector<unsigned int> &indices, 
                            int &bodyIndexCount, int &coneIndexCount);

#endif