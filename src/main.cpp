#include<iostream>
#include<vector>
#include<cmath>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Planet {
    glm::vec3 position;
    glm::vec3 velocity;
    float mass;
    float radius;
    glm::vec3 color;
    std::vector<glm::vec3> trail;
};

//vertex shader
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

//fragment shader
const char* fragmentShaderSource = R"(
#version 330 core

uniform vec3 pColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(pColor, 1.0);
}
)";

int windowWidth = 800;
int windowHeight = 600;
bool showOrbit=true;
bool oKeyPressed=false;
//camera stuff
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 15.0f); // start 15 units back
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); // look at the origin
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f); // y aaxis is up

//mouse tracking
bool firstMouse = true;
float yaw   = -90.0f; // yaw is initialized to -90 degrees so we face the -Z axis
float pitch =  0.0f;
float lastX =  800.0f / 2.0f;
float lastY =  600.0f / 2.0f;
float fov   =  45.0f; //this will control our zoom

//timing(so camera moves at the same speed regardless of framerate)
float deltaTime = 0.0f;	
float lastFrame = 0.0f;

//prototypes
GLFWwindow* StartGLFW();
unsigned int compileShader();
void processInput(GLFWwindow* window); 
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void generateSphere(float radius, int sectors, int stacks, std::vector<float>& vertices, std::vector<unsigned int>& indices);
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);


int main(){
    GLFWwindow* window = StartGLFW();
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // resize(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    //locks the mouse to the window & hides the cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);
    
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    generateSphere(1.0f, 36, 18, sphereVertices, sphereIndices);

    //vertex objects
    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);
    //tell VAO how to read the VBO
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    GLuint trailVAO, trailVBO;
    glGenVertexArrays(1, &trailVAO);
    glGenBuffers(1, &trailVBO);

    glBindVertexArray(trailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, trailVBO);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    unsigned int shaderProgram = compileShader();

    std::vector<Planet> solarSystem; 
    Planet sun;
    sun.position = glm::vec3(0.0f, 0.0f, 0.0f);
    sun.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
    sun.mass = 10000.0f;
    sun.radius = 2.0f;
    sun.color = glm::vec3(1.0f, 0.8f, 0.0f);
    solarSystem.push_back(sun);

    Planet earth;
    earth.position = glm::vec3(6.0f, 0.0f, 0.0f); 
    earth.velocity = glm::vec3(0.0f, 1.3f, 0.0f); 
    earth.mass = 1.0f;
    earth.radius = 0.5f;
    earth.color = glm::vec3(0.2f, 0.5f, 1.0f);
    solarSystem.push_back(earth);

    float G = 0.001f; 
    float dt = 0.016f;

    while(!glfwWindowShouldClose(window)){
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                // glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        //applying gravity
        for(size_t i=0; i<solarSystem.size(); i++){
            for(size_t j=0; j<solarSystem.size(); j++){
                if(i==j) continue; //same planet

                glm::vec3 direction = solarSystem[j].position - solarSystem[i].position;
                float distance = glm::length(direction);

                //prevent by zero division
                if(distance > 0.5f) { 
                    glm::vec3 normalizedDir = glm::normalize(direction);
                    float force = (G * solarSystem[i].mass * solarSystem[j].mass) / (distance * distance);
                    glm::vec3 acceleration = (normalizedDir * force) / solarSystem[i].mass;
                    solarSystem[i].velocity += acceleration * dt;
                }
            }
        }

        //applying velocity to position
        for(size_t i=0; i<solarSystem.size(); i++){
            solarSystem[i].position += solarSystem[i].velocity * dt;
            solarSystem[i].trail.push_back(solarSystem[i].position);
        }

                // if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
                //     moveY+=0.05f;
                // }
                // if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
                //     moveY-=0.05f;
                // }
                // if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
                //     moveX-=0.05f;
                // }
                // if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
                //     moveX+=0.05f;
                // }

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::mat4(1.0f);

        float aspect = (float)windowWidth / (float)windowHeight;
        projection = glm::perspective(glm::radians(fov), aspect, 0.1f, 100.0f);
        
        //sending matrices to vertex shader
        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        
        int projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        int modelLoc = glGetUniformLocation(shaderProgram, "model");
        int colorLoc = glGetUniformLocation(shaderProgram, "pColor");

        for(size_t i = 0; i < solarSystem.size(); i++){
            if(showOrbit && !solarSystem[i].trail.empty()){
                glBindVertexArray(trailVAO);

                glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
                glBufferData(GL_ARRAY_BUFFER, solarSystem[i].trail.size() * sizeof(glm::vec3), solarSystem[i].trail.data(), GL_DYNAMIC_DRAW);

                glm::mat4 trailModel = glm::mat4(1.0f);
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(trailModel));
                glUniform3fv(colorLoc, 1, glm::value_ptr(solarSystem[i].color));

                glPointSize(2.0f); 
                glDrawArrays(GL_LINE_STRIP, 0, solarSystem[i].trail.size());
                // line instead of dots = GL_POINTS to GL_LINE_STRIP
            }

            glBindVertexArray(VAO);
            glm::mat4 model = glm::mat4(1.0f);
            
            //apply transformations = read backwards: scale -> rotate -> translate
            model = glm::translate(model, solarSystem[i].position);
            model = glm::rotate(model, ((float)glfwGetTime())/2.0f, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(solarSystem[i].radius));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(colorLoc, 1, glm::value_ptr(solarSystem[i].color));

            //draw the VBO for this planet
            glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
}



/* HELPER FUNCTIONS */
//start function
GLFWwindow* StartGLFW(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL", NULL, NULL);
    glfwMakeContextCurrent(window);
    return window;
}
//input handling
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if(glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS){
        if(!oKeyPressed){
            oKeyPressed=true;
            showOrbit=!showOrbit;
        }
    }
    else{
        oKeyPressed=false;
    }
    
    float cameraSpeed = 5.0f * deltaTime; // adjust 5.0f to fly faster or slower
    
    // move front/back
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
        
    //move left/right
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        
    //fly up/down 
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraUp;
}
//resizing
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
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

unsigned int compileShader(){
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

void generateSphere(float radius, int sectors, int stacks, std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    //vertices
    for(int i = 0; i <= stacks; ++i) {
        float stackAngle = M_PI / 2 - i * M_PI / stacks;
        float xy = radius * cosf(stackAngle);           
        float z = radius * sinf(stackAngle);            

        for(int j = 0; j <= sectors; ++j) {
            float sectorAngle = j * 2 * M_PI / sectors; 
            float x = xy * cosf(sectorAngle);           
            float y = xy * sinf(sectorAngle);           
            
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    //indices
    for(int i = 0; i < stacks; ++i) {
        int k1 = i * (sectors + 1);     
        int k2 = k1 + sectors + 1;      

        for(int j = 0; j < sectors; ++j, ++k1, ++k2) {
            //2 triangles per sector (minus the exact poles)
            if(i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if(i != (stacks - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
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
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // trigo stuff to convert 2D mouse movement into 3D direction vector
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    fov -= (float)yoffset; // scroll up reduces FOV (zoom in)
    if (fov < 1.0f) fov = 1.0f;
    if (fov > 45.0f) fov = 45.0f;
}