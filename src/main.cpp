#include <iostream>
#include <vector>
#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "../include/Shader.h"
#include "../include/Camera.h"
#include "../include/Planet.h"
#include "../include/Geometry.h"
#include "../include/Physics.h"

int windowWidth = 1920;
int windowHeight = 1080;
bool showOrbit = true;
bool oKeyPressed = false;
bool tabKeyPressed = false; 
bool cKeyPressed = false;
bool switched = false;

// mouse tracking
bool firstMouse = true;
float lastX = 800.0f / 2.0f;
float lastY = 600.0f / 2.0f;

// timing(so camera moves at the same speed regardless of framerate)
float deltaTime = 0.0f;
float lastFrame = 0.0f;

//global camera object
Camera camera(glm::vec3(0.0f, 5.0f, 10.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -20.5f);

// prototypes
GLFWwindow *StartGLFW();
unsigned int loadTexture(char const * path);
void processInput(GLFWwindow *window, std::vector<Planet> &solarSystem);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

int main()
{
    GLFWwindow *window = StartGLFW();
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // resize(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // locks the mouse to the window & hides the cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);
    //enable transparency blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    generateSphere(1.0f, 72, 36, sphereVertices, sphereIndices);

    // sphere vertex objects
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);
    // tell VAO how to read the VBO
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6*sizeof(float)));
    glEnableVertexAttribArray(2);

    //rocket vertex objects
    std::vector<float> rocketVertices;
    std::vector<unsigned int> rocketIndices;
    int rocketBodyCount = 0;
    int rocketConeCount = 0;
    // radius: 1.0f, body height: 4.0f, cone height: 2.0f, sectors: 36
    generateRocketGeometry(1.0f, 4.0f, 2.0f, 36, rocketVertices, rocketIndices, rocketBodyCount, rocketConeCount);

    GLuint rocketVAO, rocketVBO, rocketEBO;
    glGenVertexArrays(1, &rocketVAO);
    glGenBuffers(1, &rocketVBO);
    glGenBuffers(1, &rocketEBO);

    glBindVertexArray(rocketVAO);

    glBindBuffer(GL_ARRAY_BUFFER, rocketVBO);
    glBufferData(GL_ARRAY_BUFFER, rocketVertices.size() * sizeof(float), rocketVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rocketEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, rocketIndices.size() * sizeof(unsigned int), rocketIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    //trail vertex objects
    GLuint trailVAO, trailVBO;
    glGenVertexArrays(1, &trailVAO);
    glGenBuffers(1, &trailVBO);

    glBindVertexArray(trailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, trailVBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    Shader lightingShader("../assets/shaders/vertex_core.glsl", "../assets/shaders/fragment_core.glsl");

    lightingShader.use();
    lightingShader.setInt("planetTexture", 0); 
    lightingShader.setInt("nightTexture", 1);  
    lightingShader.setInt("specularMap", 2);

    std::vector<Planet> solarSystem;

    // --- NORMALIZED UNITS ---
    // 1 Distance Unit = 1 Earth Radius
    // 1 Mass Unit = 1 Earth Mass

    Planet earth;
    earth.position = glm::vec3(0.0f, 0.0f, 0.0f);
    earth.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    earth.mass = 1.0f;
    earth.radius = 1.0f;
    earth.axialTilt = 23.4f;
    earth.rotationSpeedFactor = 1.0f;
    earth.color = glm::vec3(0.2f, 0.5f, 1.0f);
    earth.textureID = loadTexture("../assets/textures/earth_8k.jpg");
    earth.cloudTextureID = loadTexture("../assets/textures/clouds_8k.png");
    earth.nightTextureID = loadTexture("../assets/textures/night_8k.jpg");
    earth.specularMapID = loadTexture("../assets/textures/earth_specular.png");
    solarSystem.push_back(earth);
    
    Planet moon;
    moon.position = glm::vec3(0.0f, 0.0f, -60.3f);
    moon.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    moon.axialTilt = 6.63f;
    moon.rotationSpeedFactor = 27.3f;
    moon.mass = 0.0123f;
    moon.radius = 0.273f;
    moon.color = glm::vec3(0.749f, 0.764f, 0.800f);
    moon.textureID = loadTexture("../assets/textures/moon.jpg");
    solarSystem.push_back(moon);
    
    Planet sun;
    sun.position = glm::vec3(0.0f, 0.0f, -23455.63f);
    sun.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    sun.axialTilt=7.0f;
    sun.rotationSpeedFactor=25.67f;
    sun.mass = 333000.0f; 
    sun.radius = 109.0f;
    sun.color = glm::vec3(1.0f, 0.8f, 0.0f);
    sun.textureID = loadTexture("../assets/textures/sun.jpg");
    solarSystem.push_back(sun);

    unsigned int skyboxTexture = loadTexture("../assets/textures/milkyway_8k.jpg");

    //rocket
    Planet rocket;
    rocket.position = glm::vec3(0.0f, 0.0f, 1.05f); 
    rocket.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    rocket.axialTilt = 0.0f;
    rocket.mass = 0.000000001f; 
    rocket.radius = 0.00625f;  
    rocket.color = glm::vec3(0.0f, 1.0f, 0.0f);
    solarSystem.push_back(rocket);

    // float G = 6.674f*pow(10,-11);
    float G = 10.0f;
    float dt = 0.016f;

    float moonOrbitVel = sqrt((G * earth.mass) / abs(moon.position.z));
    solarSystem[1].velocity = glm::vec3(moonOrbitVel, 0.0f, 0.0f);
    // calculate orbital velocity for the Rocket in LEO
    float rocketOrbitVel = sqrt((G * earth.mass) / abs(rocket.position.z));
    solarSystem[3].velocity = glm::vec3(-rocketOrbitVel, 0.0f, 0.0f);


    // ---RENDER LOOP---
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, solarSystem);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        lightingShader.use();
        glBindVertexArray(VAO);

        // ---UPDATE SIMULATION PHYSICS---
        // i.e update the gravity and velocities for each planet
        updatePhysics(solarSystem, dt, G);

        // ---CAMERA POSITIONING MATH---
        glm::mat4 view = camera.GetViewMatrix(solarSystem[camera.targetIndex].position, solarSystem[camera.targetIndex].radius);
        
        float aspect = (float)windowWidth / (float)windowHeight;
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), aspect, 0.1f, 10000000.0f);

        lightingShader.setMat4("view", view);
        lightingShader.setMat4("projection", projection);
        lightingShader.setVec3("viewPos", camera.Position);

        // Send Camera and Sun position to the shader (Sun is index 2)
        lightingShader.setVec3("lightPos", solarSystem[2].position);

        // ---MILKYWAY RENDERING---
        // turn off Depth Writing i.e "draw this, but pretend it is infinitely far away, so everything else draws on top of it"
        glDepthMask(GL_FALSE); 
        
        glBindVertexArray(VAO);
        glm::mat4 skyboxModel = glm::mat4(1.0f);
        
        // Lock the sphere to the Camera's exact position so we can never reach the edge
        skyboxModel = glm::translate(skyboxModel, camera.Position);
        skyboxModel = glm::scale(skyboxModel, glm::vec3(500000.0f)); 

        lightingShader.setMat4("model", skyboxModel);

        // configure the shader toggles
        lightingShader.setInt("isSun", 1);        // Treat it like the Sun (skip shadows, draw full brightness)
        lightingShader.setInt("useTexture", 1);        // Use an image
        lightingShader.setInt("isCloud", 0);        // No transparent cloud math
        lightingShader.setInt("hasNightTexture", 0);        // No city lights

        // Bind the Milky Way texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skyboxTexture);
        glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

        glDepthMask(GL_TRUE);


        // ---DRAWING LOOP---
        for (size_t i = 0; i < solarSystem.size(); i++){
            //drawing trail
            if (showOrbit && !solarSystem[i].trail.empty()){
                glBindVertexArray(trailVAO);    

                glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
                glBufferData(GL_ARRAY_BUFFER, solarSystem[i].trail.size() * sizeof(glm::vec3), solarSystem[i].trail.data(), GL_DYNAMIC_DRAW);
                
                glm::mat4 trailModel = glm::mat4(1.0f);
                lightingShader.setMat4("model", trailModel);
                lightingShader.setVec3("pColor", solarSystem[i].color);

                lightingShader.setInt("isSun", 1);
                lightingShader.setInt("useTexture", 0);
                lightingShader.setInt("isCloud", 0);

                glPointSize(2.0f);
                glDrawArrays(GL_LINE_STRIP, 0, solarSystem[i].trail.size());
                // line instead of dots = GL_POINTS to GL_LINE_STRIP
            }

            glBindVertexArray(VAO);
            glm::mat4 model = glm::mat4(1.0f);

            if(i==2){
                lightingShader.setInt("isSun", 1); 
            } 
            else{
                lightingShader.setInt("isSun", 0); 
            }

            // MVP Transforms
            model = glm::translate(model, solarSystem[i].position);
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(90.0f + solarSystem[i].axialTilt), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, ((float)glfwGetTime() / 2.0f) / solarSystem[i].rotationSpeedFactor, glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(solarSystem[i].radius));

            lightingShader.setMat4("model", model);
            lightingShader.setVec3("pColor", solarSystem[i].color);

            //---TEXTURE BINDING---
            if (solarSystem[i].textureID != 0){
                lightingShader.setInt("useTexture", 1);
                
                // Bind Day Texture to Slot 0
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, solarSystem[i].textureID);

                // Bind Night Texture to Slot 1 
                if (solarSystem[i].nightTextureID != 0) {
                    lightingShader.setInt("hasNightTexture", 1);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, solarSystem[i].nightTextureID);
                } else {
                    lightingShader.setInt("hasNightTexture", 0);
                }
                //Bind Specular Texture to Slot 2
                if (solarSystem[i].specularMapID != 0) {
                    lightingShader.setInt("hasSpecularMap", 1);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, solarSystem[i].specularMapID);
                } else {
                    lightingShader.setInt("hasSpecularMap", 0);
                }

            } else {
                lightingShader.setInt("useTexture", 0);
                lightingShader.setInt("hasNightTexture", 0);
                lightingShader.setInt("hasSpecularMap", 0);     
            }

            lightingShader.setInt("isCloud", 0);
            
            // ---ROCKET RENDERING---
            if (i == 3) {
                glBindVertexArray(rocketVAO);
                
                glm::mat4 rocketModel = glm::mat4(1.0f);
                rocketModel = glm::translate(rocketModel, solarSystem[i].position);
                
                // make rocekt face the direction its moving i.e its velocity
                if (glm::length(solarSystem[i].velocity) > 0.0001f) {
                    glm::vec3 velDir = glm::normalize(solarSystem[i].velocity);
                    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
                    
                    // Calculate axis and angle to rotate 'up' towards 'velDir'
                    glm::vec3 axis = glm::cross(up, velDir);
                    float angle = acos(glm::clamp(glm::dot(up, velDir), -1.0f, 1.0f));
                    
                    if (glm::length(axis) > 0.0001f) {
                        rocketModel = glm::rotate(rocketModel, angle, glm::normalize(axis));
                    } else if (glm::dot(up, velDir) < 0.0f) {
                        // Edge case: moving straight down, flip 180 degrees
                        rocketModel = glm::rotate(rocketModel, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                    }
                }
                
                // Scale the rocket so its center is balanced and sized correctly
                rocketModel = glm::scale(rocketModel, glm::vec3(solarSystem[i].radius));
                // Shift it down slightly so the "center of mass" isn't at the very bottom base
                rocketModel = glm::translate(rocketModel, glm::vec3(0.0f, -2.0f, 0.0f)); 

                lightingShader.setMat4("model", rocketModel);
                
                // Disable textures for the rocket to use custom colors
                lightingShader.setInt("useTexture", 0);
                lightingShader.setInt("hasNightTexture", 0);
                lightingShader.setInt("hasSpecularMap", 0);

                // 1. Draw Body (Dark Gray)
                lightingShader.setVec3("pColor", 0.4f, 0.4f, 0.4f);
                glDrawElements(GL_TRIANGLES, rocketBodyCount, GL_UNSIGNED_INT, 0);
                
                // 2. Draw Cone (Light Gray / Silver)
                lightingShader.setVec3("pColor", 0.8f, 0.8f, 0.8f);
                // Use an offset pointer to start drawing where the body indices ended
                glDrawElements(GL_TRIANGLES, rocketConeCount, GL_UNSIGNED_INT, (void*)(rocketBodyCount * sizeof(unsigned int)));
                
                //rebind the standard planet VAO for the next iterations
                glBindVertexArray(VAO);
            } else {
                // ---STANDARD PLANET RENDERING---
                glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
            }

            // ---SUN CORONA DRAWING---
            if(i==2){  //check for sun
                glm::mat4 coronaModel = glm::mat4(1.0f);
                coronaModel = glm::translate(coronaModel, solarSystem[i].position);
                
                // Scale it up so it acts like an atmosphere extending into space
                coronaModel = glm::scale(coronaModel, glm::vec3(solarSystem[i].radius * 1.6f)); 

                lightingShader.setMat4("model", coronaModel);
                
                // Turn OFF standard textures and turn ON the Corona math
                lightingShader.setInt("useTexture", 0);
                lightingShader.setInt("isSun", 0); 
                lightingShader.setInt("isCorona", 1);
                
                // Set the glow to a bright, hot fiery orange
                lightingShader.setVec3("pColor", 1.0f, 0.6f, 0.0f);

                // CRITICAL: Disable depth writing so the transparent glow 
                // doesn't block out the stars or trails behind it!
                glDepthMask(GL_FALSE);
                glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
                glDepthMask(GL_TRUE); // Turn it back on for the next planet
                
                // Reset the corona switch
                lightingShader.setInt("isCorona", 0);
            }

            // ---CLOUD DRAWING---
            if(solarSystem[i].cloudTextureID != 0) {
                glm::mat4 cloudModel = glm::mat4(1.0f);
                
                // move to the exact same planet position
                cloudModel = glm::translate(cloudModel, solarSystem[i].position);
                cloudModel = glm::rotate(cloudModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                cloudModel = glm::rotate(cloudModel, glm::radians(90.0f + 23.4f), glm::vec3(1.0f, 0.0f, 0.0f));
                float cloudSpeed = 1.2f; 
                cloudModel = glm::rotate(cloudModel, ((float)glfwGetTime() / 2.0f) * cloudSpeed, glm::vec3(0.0f, 0.0f, 1.0f));
                cloudModel = glm::scale(cloudModel, glm::vec3(solarSystem[i].radius * 1.00313f));

                lightingShader.setMat4("model", cloudModel);
                
                lightingShader.setInt("useTexture", 1);
                lightingShader.setInt("isCloud", 1); 
                lightingShader.setInt("hasNightTexture", 0); 
                lightingShader.setInt("hasSpecularMap", 0); 

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, solarSystem[i].cloudTextureID);

                glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}

/* HELPER FUNCTIONS */
// start function
GLFWwindow *StartGLFW()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(windowWidth, windowHeight, "Artemis II", NULL, NULL);
    glfwMakeContextCurrent(window);
    return window;
}

// input handling
void processInput(GLFWwindow *window, std::vector<Planet> &solarSystem)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    //orbit trail toggle
    if(glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS){
        if (!oKeyPressed){
            oKeyPressed = true;
            showOrbit = !showOrbit;
        }
    } else { oKeyPressed = false; }

    //cam toggle
    if(glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if (!cKeyPressed){
            camera.ToggleOrbitalMode();
            cKeyPressed = true;
        }
    } else { cKeyPressed = false; }

    //orb cam : planet toggle
    if(glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        if (!tabKeyPressed){
            camera.CycleTarget(solarSystem.size());
            tabKeyPressed = true;
        }
    } else { tabKeyPressed = false; }

    // Rocket controls
    if(solarSystem.size() > 3){
        glm::vec3 direction = glm::normalize(solarSystem[3].velocity);
        glm::vec3 dir2 = camera.Front; 
        
        float thrust2 = 2.0f * deltaTime; 
        if(glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) solarSystem[3].velocity += dir2 * thrust2;
        if(glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) solarSystem[3].velocity -= dir2 * thrust2;

        float thrust = 2.0f * deltaTime;
        if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) solarSystem[3].velocity += direction * thrust;
        if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) solarSystem[3].velocity -= direction * thrust;
    }

    // Free-fly camera controls
    if(!camera.isOrbitalMode){
        bool isAccelerating = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS);
        
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(FORWARD, deltaTime, isAccelerating);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(BACKWARD, deltaTime, isAccelerating);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(LEFT, deltaTime, isAccelerating);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(RIGHT, deltaTime, isAccelerating);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.ProcessKeyboard(UP, deltaTime, isAccelerating);
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
            camera.ProcessKeyboard(DOWN, deltaTime, isAccelerating);
    }
}

// resizing
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);

    windowWidth = width;
    windowHeight = height;
}

//texture loading function
unsigned int loadTexture(char const * path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(true); 
    
    // Notice the '4' at the end! This forces stb_image to output RGBA every time.
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 4); 
    
    if (data) {
        glBindTexture(GL_TEXTURE_2D, textureID);
        // We now safely assume GL_RGBA for everything
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    return textureID;
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {   
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}