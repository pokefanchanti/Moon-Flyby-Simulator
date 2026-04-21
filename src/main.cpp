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

struct Planet
{   
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

// vertex shader
const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords; //UV coords

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
        //calc location of vertex in world space
    FragPos = vec3(model * vec4(aPos, 1.0));
    
        //correct normal vector if planet is scaled or rotated
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

// fragment shader
const char *fragmentShaderSource = R"(
#version 330 core


uniform sampler2D planetTexture;
uniform sampler2D nightTexture;
uniform sampler2D specularMap;

uniform vec3 pColor;
uniform bool useTexture;
uniform bool isCloud;
uniform bool hasNightTexture;
uniform bool hasSpecularMap;

uniform vec3 lightPos; //position of light source
uniform vec3 viewPos; //position of camera
uniform bool isSun; //is current object being rendered the light source?
uniform bool isCorona; // glow toggle

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

void main(){
        //corona glow rendering
    if (isCorona) {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);
        
            // Intensity is 1.0 at the center, 0.0 at the grazing edges
        float intensity = max(dot(norm, viewDir), 0.0);
        
            // Cube the intensity so the glow fades smoothly but stays tight to the sun
        float alpha = pow(intensity, 3.0);
        
            // Multiply alpha by 0.6 so the center isn't 100% solid, allowing the Sun texture to peek through!
        FragColor = vec4(pColor, alpha * 0.6);
        return;
    }
        //normal rendering
    vec4 texData = texture(planetTexture, TexCoords);
    vec3 objectColor = texData.rgb;
    float alpha = texData.a;
    
    if (useTexture) {
        vec4 texData = texture(planetTexture, TexCoords);
        objectColor = texData.rgb;
        alpha = texData.a;
        
            //cloud hack
        if (isCloud) {
                // Use the redness/brightness of the image as the transparency!
                // Black background = 0.0 (Invisible). White clouds = 1.0 (Solid).
            alpha = texData.r; 
            objectColor = vec3(1.0); // Force the clouds to be pure white
        }
    }else{
        objectColor = pColor;
    }

    if(isSun){
        FragColor = vec4(objectColor * 5.0, alpha);
        return;
    }
    
        //ambient lighting
    float ambientStrength = 0.10;
    vec3 ambient = ambientStrength * vec3(1.0);

        //diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float nDotL = dot(norm, lightDir);
    float diff = max(nDotL, 0.0);
    vec3 diffuse = diff * vec3(1.0);

        //specular lighting
    float specularStrength = 0.2;
    if (hasSpecularMap && !isCloud) {
            // We only need the 'r' (red) channel because the image is black and white
        float specMask = texture(specularMap, TexCoords).r; 
        
            // Multiply our strength by the mask. 
            // Oceans (1.0) stay 0.2 shiny. Land (0.0) becomes 0.0 shiny!
        specularStrength *= specMask; 
    }
    if (isCloud) { specularStrength = 0.0; }
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular = specularStrength * spec * vec3(1.0);
        // Note: The Moon shouldn't be very shiny, but Earth's oceans should! We keep it simple for now.

    vec3 result = (ambient + diffuse + specular) * objectColor;
    if (hasNightTexture && !isCloud) {
        vec3 nightColor = texture(nightTexture, TexCoords).rgb;
        
            // smoothstep creates a soft twilight gradient at the terminator line.
            // When nDotL is 0.1 (twilight), blend starts. When -0.2 (night), blend is 1.0.
        float nightBlend = smoothstep(0.1, -0.2, nDotL); 
        
            // Add the glowing city lights on top of the dark surface!
        result += nightColor * nightBlend;
    }
    FragColor = vec4(result, alpha);
}
)";

int windowWidth = 1920;
int windowHeight = 1080;
bool showOrbit = true;
bool oKeyPressed = false;
// camera stuff
glm::vec3 cameraPos = glm::vec3(0.0f, 15.0f, 30.0f); // start 15 units back
glm::vec3 cameraFront;
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f); // y aaxis is up

// mouse tracking
bool firstMouse = true;
float yaw = -90.0f; // yaw is initialized to -90 degrees so we face the -Z axis
float pitch = -20.5f;
float lastX = 800.0f / 2.0f;
float lastY = 600.0f / 2.0f;
float fov = 45.0f; // this will control our zoom

// timing(so camera moves at the same speed regardless of framerate)
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// prototypes
GLFWwindow *StartGLFW();
unsigned int compileShader();
unsigned int loadTexture(char const * path);
void processInput(GLFWwindow *window, std::vector<Planet> &solarSystem);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void generateSphere(float radius, int sectors, int stacks, std::vector<float> &vertices, std::vector<unsigned int> &indices);
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

    // vertex objects
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

    GLuint trailVAO, trailVBO;
    glGenVertexArrays(1, &trailVAO);
    glGenBuffers(1, &trailVBO);

    glBindVertexArray(trailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, trailVBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    unsigned int shaderProgram = compileShader();

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "planetTexture"), 0); // Slot 0
    glUniform1i(glGetUniformLocation(shaderProgram, "nightTexture"), 1);  // Slot 1
    glUniform1i(glGetUniformLocation(shaderProgram, "specularMap"), 2); //Slot 2

    std::vector<Planet> solarSystem;

    // --- NORMALIZED ENGINE UNITS ---
    // 1 Distance Unit = 1 Earth Radius
    // 1 Mass Unit = 1 Earth Mass

    // 149.6 * 10^6 km
    // 6378 km
    // 23455.63f

    Planet earth;
    earth.position = glm::vec3(0.0f, 0.0f, 0.0f);
    earth.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    earth.mass = 1.0f;
    earth.radius = 1.0f;
    earth.axialTilt = 23.4f;
    earth.rotationSpeedFactor = 1.0f;
    earth.color = glm::vec3(0.2f, 0.5f, 1.0f);
    earth.textureID = loadTexture("../earth_8k.jpg");
    earth.cloudTextureID = loadTexture("../clouds_8k.png");
    earth.nightTextureID = loadTexture("../night_8k.jpg");
    earth.specularMapID = loadTexture("../earth_specular.png");
    solarSystem.push_back(earth);
    
    Planet moon;
    moon.position = glm::vec3(0.0f, 0.0f, -60.3f);
    moon.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    moon.axialTilt = 6.63f;
    // moon.velocity = glm::vec3(0.0f, 0.0f, -1.3f);
    moon.rotationSpeedFactor = 27.3f;
    moon.mass = 0.0123f;
    moon.radius = 0.273f;
    moon.color = glm::vec3(0.749f, 0.764f, 0.800f);
    moon.textureID = loadTexture("../moon.jpg");
    solarSystem.push_back(moon);
    
    Planet sun;
    sun.position = glm::vec3(0.0f, 0.0f, 23455.63f);
    sun.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    // sun.mass = 0.1f;
    sun.axialTilt=7.0f;
    sun.rotationSpeedFactor=25.67f;
    sun.mass = 333000.0f; 
    sun.radius = 109.0f;
    sun.color = glm::vec3(1.0f, 0.8f, 0.0f);
    sun.textureID = loadTexture("../sun.jpg");
    solarSystem.push_back(sun);

    unsigned int skyboxTexture = loadTexture("../milkyway_8k.jpg");

    // 4. THE ROCKET (Artemis II)
    Planet rocket;
    // Start it in Low Earth Orbit (LEO) - slightly above the Earth's surface
    // Earth radius is 1.0, so we put the rocket at 1.05
    rocket.position = glm::vec3(0.0f, 0.0f, 1.05f); 
    rocket.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    rocket.axialTilt = 0.0f;
    rocket.mass = 0.0000001f; // Effectively zero mass
    rocket.radius = 0.05f;    // Exaggerated visual size so we can see it
    rocket.color = glm::vec3(0.0f, 1.0f, 0.0f); // Bright green!
    solarSystem.push_back(rocket);

    // float G = 6.674f*pow(10,-11);
    float G = 10.0f;
    float dt = 0.016f;

    float moonOrbitVel = sqrt((G * earth.mass) / abs(moon.position.z));
    solarSystem[1].velocity = glm::vec3(moonOrbitVel, 0.0f, 0.0f);
    // Calculate orbital velocity for the Rocket in LEO
    float rocketOrbitVel = sqrt((G * earth.mass) / abs(rocket.position.z));
    solarSystem[3].velocity = glm::vec3(rocketOrbitVel, 0.0f, 0.0f);

    // calc cameraFront vector
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, solarSystem);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        // glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        // applying gravity
        for (size_t i = 0; i < solarSystem.size(); i++){
            if(i==2) continue; //skip the sun
            for (size_t j = 0; j < solarSystem.size(); j++){
                if(i==j) continue; // same planet
                if(j==2) continue; //skip the sun
                if(i==0 && j==1) continue; //earth pulled by moon
                glm::vec3 direction = solarSystem[j].position - solarSystem[i].position;
                float distance = glm::length(direction);

                // prevent by zero division
                if (distance > 0.5f)
                {
                    glm::vec3 normalizedDir = glm::normalize(direction);
                    float force = (G * solarSystem[i].mass * solarSystem[j].mass) / (distance * distance);
                    glm::vec3 acceleration = (normalizedDir * force) / solarSystem[i].mass;
                    solarSystem[i].velocity += acceleration * dt;
                }

                // collision
                //  if(distance<=solarSystem[i].radius+solarSystem[j].radius){
                //      solarSystem[i].velocity*=glm::vec3(-1.0f,-1.0f,1.0f);
                //  }
            }
        }

        // applying velocity to position
        for (size_t i = 0; i < solarSystem.size(); i++){
            solarSystem[i].position += solarSystem[i].velocity * dt;
            solarSystem[i].trail.push_back(solarSystem[i].position);
        }

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::mat4(1.0f);

        float aspect = (float)windowWidth / (float)windowHeight;
        projection = glm::perspective(glm::radians(fov), aspect, 0.1f, 1000000.0f);

        // sending matrices to vertex shader
        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        int projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        int modelLoc = glGetUniformLocation(shaderProgram, "model");
        int colorLoc = glGetUniformLocation(shaderProgram, "pColor");
        
        //get the lighting uniform locations
        int lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
        int viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
        int isSunLoc = glGetUniformLocation(shaderProgram, "isSun");

        int useTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");
        int isCloudLoc = glGetUniformLocation(shaderProgram, "isCloud");
        int hasNightLoc = glGetUniformLocation(shaderProgram, "hasNightTexture");
        int isCoronaLoc = glGetUniformLocation(shaderProgram, "isCorona");
        int hasSpecLoc = glGetUniformLocation(shaderProgram, "hasSpecularMap");
        // Send Camera and Sun position to the shader
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(solarSystem[2].position)); // Sun is index 2
        // glm::vec3 customAxis = glm::normalize(glm::vec3(0.4f, 1.0f, 0.1f));

        // ----------milkyway rending-----------------
        // 1. Turn off Depth Writing! 
        // This is a magical graphics trick. It tells OpenGL: "Draw this, but pretend it is infinitely far away, so EVERYTHING else draws on top of it."
        glDepthMask(GL_FALSE); 
        
        glBindVertexArray(VAO);
        glm::mat4 skyboxModel = glm::mat4(1.0f);
        
        // 2. Lock the sphere to the Camera's exact position so we can never reach the edge
        skyboxModel = glm::translate(skyboxModel, cameraPos); 
        
        // 3. Make it ridiculously massive
        skyboxModel = glm::scale(skyboxModel, glm::vec3(50000.0f)); 

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(skyboxModel));

        // 4. Configure the shader toggles
        glUniform1i(isSunLoc, 1);        // Treat it like the Sun (skip shadows, draw full brightness)
        glUniform1i(useTextureLoc, 1);   // Use an image
        glUniform1i(isCloudLoc, 0);      // No transparent cloud math
        glUniform1i(hasNightLoc, 0);     // No city lights

        // 5. Bind the Milky Way texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, skyboxTexture);

        // 6. Draw the giant sphere
        glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

        // 7. Turn Depth Writing back on so the planets render normally!
        glDepthMask(GL_TRUE);

        for (size_t i = 0; i < solarSystem.size(); i++){
            //drawing trail
            if (showOrbit && !solarSystem[i].trail.empty()){
                glBindVertexArray(trailVAO);    

                glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
                glBufferData(GL_ARRAY_BUFFER, solarSystem[i].trail.size() * sizeof(glm::vec3), solarSystem[i].trail.data(), GL_DYNAMIC_DRAW);
                
                glm::mat4 trailModel = glm::mat4(1.0f);
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(trailModel));
                glUniform3fv(colorLoc, 1, glm::value_ptr(solarSystem[i].color));

                glUniform1i(isSunLoc, 1);
                glUniform1i(useTextureLoc, 0);
                glUniform1i(isCloudLoc, 0);

                glPointSize(2.0f);
                glDrawArrays(GL_LINE_STRIP, 0, solarSystem[i].trail.size());
                // line instead of dots = GL_POINTS to GL_LINE_STRIP
            }

            glBindVertexArray(VAO);
            glm::mat4 model = glm::mat4(1.0f);

            if(i==2){
                glUniform1i(isSunLoc, 1); // True
            } 
            else{
                glUniform1i(isSunLoc, 0); // False
            }

            // apply transformations = read backwards: scale -> rotate -> translate
            model = glm::translate(model, solarSystem[i].position);
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(90.0f + solarSystem[i].axialTilt), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, ((float)glfwGetTime() / 2.0f) / solarSystem[i].rotationSpeedFactor, glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(solarSystem[i].radius));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(colorLoc, 1, glm::value_ptr(solarSystem[i].color));

            //texture binding
            if (solarSystem[i].textureID != 0){
                glUniform1i(useTextureLoc, 1); 
                
                // Bind Day Texture to Slot 0
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, solarSystem[i].textureID);

                // Bind Night Texture to Slot 1 (If it exists)
                if (solarSystem[i].nightTextureID != 0) {
                    glUniform1i(hasNightLoc, 1);
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, solarSystem[i].nightTextureID);
                } else {
                    glUniform1i(hasNightLoc, 0);
                }
                //Bind Specular Texture to Slot 2
                if (solarSystem[i].specularMapID != 0) {
                    glUniform1i(hasSpecLoc, 1);
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, solarSystem[i].specularMapID);
                } else {
                    glUniform1i(hasSpecLoc, 0);
                }

            } else {
                glUniform1i(useTextureLoc, 0); 
                glUniform1i(hasNightLoc, 0); 
                glUniform1i(hasSpecLoc, 0); 
            }

            glUniform1i(isCloudLoc, 0);
            glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);

            //corona drawing
            if(i==2){  //check for sun
                glm::mat4 coronaModel = glm::mat4(1.0f);
                coronaModel = glm::translate(coronaModel, solarSystem[i].position);
                
                // Scale it up so it acts like an atmosphere extending into space
                coronaModel = glm::scale(coronaModel, glm::vec3(solarSystem[i].radius * 1.6f)); 

                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(coronaModel));
                
                // Turn OFF standard textures and turn ON the Corona math
                glUniform1i(useTextureLoc, 0); 
                glUniform1i(isSunLoc, 0); 
                glUniform1i(isCoronaLoc, 1);
                
                // Set the glow to a bright, hot fiery orange
                glUniform3f(colorLoc, 1.0f, 0.6f, 0.0f); 

                // CRITICAL: Disable depth writing so the transparent glow 
                // doesn't block out the stars or trails behind it!
                glDepthMask(GL_FALSE);
                glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
                glDepthMask(GL_TRUE); // Turn it back on for the next planet
                
                // Reset the corona switch
                glUniform1i(isCoronaLoc, 0);
            }

            //cloud drawing
            if(solarSystem[i].cloudTextureID != 0) {
                glm::mat4 cloudModel = glm::mat4(1.0f);
                
                // 1. Move to the exact same planet position
                cloudModel = glm::translate(cloudModel, solarSystem[i].position);
                cloudModel = glm::rotate(cloudModel, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                cloudModel = glm::rotate(cloudModel, glm::radians(90.0f + 23.4f), glm::vec3(1.0f, 0.0f, 0.0f));
                float cloudSpeed = 1.2f; 
                cloudModel = glm::rotate(cloudModel, ((float)glfwGetTime() / 2.0f) * cloudSpeed, glm::vec3(0.0f, 0.0f, 1.0f));
                cloudModel = glm::scale(cloudModel, glm::vec3(solarSystem[i].radius * 1.005f));

                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(cloudModel));
                
                glUniform1i(useTextureLoc, 1);
                glUniform1i(isCloudLoc, 1); //  Activate the transparent cloud hack!
                glUniform1i(hasNightLoc, 0); //disable city lights for clouds
                glUniform1i(hasSpecLoc, 0); //disable spec map for clouds

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

    if(glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS){
        if (!oKeyPressed){
            oKeyPressed = true;
            showOrbit = !showOrbit;
        }
    }
    else{
        oKeyPressed = false;
    }

    if(solarSystem.size()>3){
        glm::vec3 direction = glm::normalize(solarSystem[3].velocity);
        float thrust = 2.0f * deltaTime;

        if(glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS){
            solarSystem[3].velocity += direction * thrust;
        }

        if(glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS){
            solarSystem[3].velocity -= direction * thrust;
        }
    }

    float cameraSpeed = 5.0f * deltaTime; // adjust 5.0f to fly faster or slower

    // move front/back
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(glm::normalize(glm::cross(cameraFront, cameraUp)), cameraUp)) * cameraSpeed;
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(glm::normalize(glm::cross(cameraFront, cameraUp)), cameraUp)) * cameraSpeed;
        // cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos -=glm::normalize(glm::cross(glm::normalize(glm::cross(cameraFront, cameraUp)), cameraUp)) * cameraSpeed;
    else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos -=glm::normalize(glm::cross(glm::normalize(glm::cross(cameraFront, cameraUp)), cameraUp)) * cameraSpeed;

    // move left/right
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    // fly up/down
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += 10 * cameraSpeed * cameraUp;
    else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS && glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        cameraPos -= 10 * cameraSpeed * cameraUp;
    else if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraUp;
}
// resizing
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);

    windowWidth = width;
    windowHeight = height;
}

// void resize(GLFWwindow* window){
//     //resizing
//     int framebuffer_width, framebuffer_height;
//     glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
//     glViewport(0, 0, framebuffer_width, framebuffer_height);
//     glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
// }

unsigned int compileShader()
{
    // Compile shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    // Link shader program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
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

void generateSphere(float radius, int sectors, int stacks, std::vector<float> &vertices, std::vector<unsigned int> &indices)
{
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

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coords go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // prevent camera from flipping upside down
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    // trigo stuff to convert 2D mouse movement into 3D direction vector
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    fov -= (float)yoffset; // scroll up reduces FOV (zoom in)
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
}