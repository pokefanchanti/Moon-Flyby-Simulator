#include<iostream>
#include<vector>
#include<cmath>
#include<glad/glad.h>
#include<GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
out vec4 FragColor;
void main() {
    FragColor = vec4(0.0, 0.0, 1.0, 1.0);
}
)";

int windowWidth = 800;
int windowHeight = 600;

//prototypes
GLFWwindow* StartGLFW();
unsigned int compileShader();
void processInput(GLFWwindow* window); 
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void generateSphere(float radius, int sectors, int stacks, std::vector<float>& vertices, std::vector<unsigned int>& indices);

int main(){
    GLFWwindow* window = StartGLFW();
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // resize(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glEnable(GL_DEPTH_TEST);
    
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    generateSphere(1.0f, 36, 18, sphereVertices, sphereIndices);

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);

    // Tell VAO how to read the VBO
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    unsigned int shaderProgram = compileShader();

    float moveY=0.0f;
    float moveX=0.0f;

    while(!glfwWindowShouldClose(window)){
        processInput(window);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        // glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);

        // 2. Setup the Model: Let's push the planet a little to the right
        // model = glm::translate(model, glm::vec3(width/2.0f, yPos, 0.0f));


        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
            moveY+=0.05f;
        }
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
            moveY-=0.05f;
        }
        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
            moveX-=0.05f;
        }
        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
            moveX+=0.05f;
        }
        
        model = glm::translate(model, glm::vec3(moveX, moveY, 0.0f));
        model = glm::rotate(model, ((float)glfwGetTime())/2.0f, glm::vec3(1.0f, 1.0f, 1.0f));

        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -5.0f));

        float aspect = (float)windowWidth / (float)windowHeight;
        projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

        // 5. Send the matrices to your Vertex Shader
        int modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        int viewLoc = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        int projLoc = glGetUniformLocation(shaderProgram, "projection");
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Draw the shape
        glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
        
        // glm::vec4 vec(1.0f, 0.0f, 0.0f, 1.0f);
        // glm::mat4 trans = glm::mat4(1.0f);
        // trans = glm::translate(trans, glm::vec3(1.0f, 1.0f, 0.0f));
        // vec = trans * vec;
        // std::cout << vec.x << vec.y << vec.z << std::endl;

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
//esc to close
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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