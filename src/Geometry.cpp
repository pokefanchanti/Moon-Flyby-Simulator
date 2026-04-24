#include "../include/Geometry.h"
#include <cmath>

// Safety fallback for strict compilers (windows)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void generateSphere(float radius, int sectors, int stacks, std::vector<float> &vertices, std::vector<unsigned int> &indices) {
    // vertices
    for (int i = 0; i <= stacks; ++i)
    {
        float stackAngle = M_PI / 2 - i * M_PI / stacks;
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);

        for (int j = 0; j <= sectors; ++j)
        {
            float sectorAngle = j * 2 * M_PI / sectors;
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);

            //vertices
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            //normals
            vertices.push_back(x / radius);
            vertices.push_back(y / radius);
            vertices.push_back(z / radius);

            //texture UVs
            // U goes from 0 to 1 across the sectors (longitude)
            // V goes from 0 to 1 across the stacks (latitude)
            float u = 1.0f - (float)j / sectors;
            float v = (float)i / stacks;
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    // indices
    for (int i = 0; i < stacks; ++i)
    {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; ++j, ++k1, ++k2)
        {
            // 2 triangles per sector (minus the exact poles)
            if (i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if (i != (stacks - 1))
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

void generateRocketGeometry(float radius, float bodyHeight, float coneHeight, int sectors, 
                            std::vector<float> &vertices, std::vector<unsigned int> &indices, 
                            int &bodyIndexCount, int &coneIndexCount) {
    vertices.clear();
    indices.clear();

    //cylinder sides
    for(int i = 0; i <= sectors; i++) {
        float angle = (float)i / sectors * 2.0f * M_PI;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        float nx = cos(angle), nz = sin(angle); 
        
        vertices.push_back(x); vertices.push_back(0.0f); vertices.push_back(z);
        vertices.push_back(nx); vertices.push_back(0.0f); vertices.push_back(nz);
        vertices.push_back(0.0f); vertices.push_back(0.0f); 
        
        vertices.push_back(x); vertices.push_back(bodyHeight); vertices.push_back(z);
        vertices.push_back(nx); vertices.push_back(0.0f); vertices.push_back(nz);
        vertices.push_back(0.0f); vertices.push_back(1.0f); 
    }
    
    for(int i = 0; i < sectors; i++) {
        int bl = i * 2;         
        int tl = i * 2 + 1;     
        int br = (i + 1) * 2;   
        int tr = (i + 1) * 2 + 1; 
        indices.push_back(bl); indices.push_back(br); indices.push_back(tl);
        indices.push_back(tl); indices.push_back(br); indices.push_back(tr);
    }
    
    //cylinder base
    int baseCenter = vertices.size() / 8;
    vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
    vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f); 
    vertices.push_back(0.5f); vertices.push_back(0.5f);
    
    int baseEdge = vertices.size() / 8;
    for(int i = 0; i <= sectors; i++) {
        float angle = (float)i / sectors * 2.0f * M_PI;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        vertices.push_back(x); vertices.push_back(0.0f); vertices.push_back(z);
        vertices.push_back(0.0f); vertices.push_back(-1.0f); vertices.push_back(0.0f);
        vertices.push_back(0.0f); vertices.push_back(0.0f);
    }
    for(int i = 0; i < sectors; i++) {
        indices.push_back(baseCenter);
        indices.push_back(baseEdge + i + 1);
        indices.push_back(baseEdge + i);
    }
    
    bodyIndexCount = indices.size(); 

    //cone nose
    int coneApex = vertices.size() / 8;
    vertices.push_back(0.0f); vertices.push_back(bodyHeight + coneHeight); vertices.push_back(0.0f);
    vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f); 
    vertices.push_back(0.5f); vertices.push_back(1.0f);
    
    int coneBase = vertices.size() / 8;
    float coneNy = radius / coneHeight;
    for(int i = 0; i <= sectors; i++) {
        float angle = (float)i / sectors * 2.0f * M_PI;
        float x = cos(angle) * radius;
        float z = sin(angle) * radius;
        float nx = cos(angle);
        float nz = sin(angle);
        float len = sqrt(nx*nx + coneNy*coneNy + nz*nz);
        
        vertices.push_back(x); vertices.push_back(bodyHeight); vertices.push_back(z);
        vertices.push_back(nx/len); vertices.push_back(coneNy/len); vertices.push_back(nz/len);
        vertices.push_back(0.0f); vertices.push_back(0.0f);
    }
    
    for(int i = 0; i < sectors; i++) {
        indices.push_back(coneApex);
        indices.push_back(coneBase + i);
        indices.push_back(coneBase + i + 1);
    }
    
    coneIndexCount = indices.size() - bodyIndexCount;
}