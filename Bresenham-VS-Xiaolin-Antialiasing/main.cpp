#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>
#include <cmath>

// Vertex structure for Xiaolin Wu lines
struct Vertex {
    float x, y;
    float alpha; // intensity from 0.0 to 1.0
};

// Bresenham line: basic
std::vector<float> bresenhamLine(int x0, int y0, int x1, int y1, int width, int height) {
    std::vector<float> vertices;

    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) {
        float ndcX = (2.0f * x0) / width - 1.0f;
        float ndcY = (2.0f * y0) / height - 1.0f;
        vertices.push_back(ndcX);
        vertices.push_back(ndcY);

        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }

    return vertices;
}

// Bresenham with Xiaolin Wu antialiasing
std::vector<Vertex> bresenhamWithXiaolin(int x0, int y0, int x1, int y1, int width, int height) {
    std::vector<Vertex> vertices;

    bool steep = abs(y1 - y0) > abs(x1 - x0);

    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }

    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    float dx = float(x1 - x0);
    float dy = float(y1 - y0);
    float gradient = dy / dx;

    float y = y0 + 0.0f;

    for (int x = x0; x <= x1; ++x) {
        int y_int = int(floor(y));
        float frac = y - y_int;

        // Main pixel
        float ndcX = 2.0f * (steep ? y_int : x) / width - 1.0f;
        float ndcY = 2.0f * (steep ? x : y_int) / height - 1.0f;
        vertices.push_back({ ndcX, ndcY, 1.0f - frac });

        // Adjacent pixel
        ndcX = 2.0f * (steep ? y_int + 1 : x) / width - 1.0f;
        ndcY = 2.0f * (steep ? x : y_int + 1) / height - 1.0f;
        vertices.push_back({ ndcX, ndcY, frac });

        y += gradient;
    }

    return vertices;
}

// Shader loader
std::string loadShaderSource(const char* filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "ERROR: Could not open shader file: " << filePath << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// GLFW callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Bresenham Lines", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);

    // Shaders
    std::string vertexCode = loadShaderSource("vertex_shader.glsl");
    std::string fragmentCode = loadShaderSource("fragment_shader.glsl");

    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vShaderCode, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Generate initial vertices
    std::vector<float> verticesBresenham = bresenhamLine(50, 50, 750, 550, SCR_WIDTH, SCR_HEIGHT);
    std::vector<Vertex> verticesWu = bresenhamWithXiaolin(50, 50, 750, 550, SCR_WIDTH, SCR_HEIGHT);

    // Create VAO & VBO for Bresenham
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, verticesBresenham.size() * sizeof(float), verticesBresenham.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Create VAO & VBO for Xiaolin Wu
    GLuint VAOWu, VBOWu;
    glGenVertexArrays(1, &VAOWu);
    glGenBuffers(1, &VBOWu);

    glBindVertexArray(VAOWu);
    glBindBuffer(GL_ARRAY_BUFFER, VBOWu);
    glBufferData(GL_ARRAY_BUFFER, verticesWu.size() * sizeof(Vertex), verticesWu.data(), GL_DYNAMIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Alpha attribute
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Time uniform
    GLint timeLoc = glGetUniformLocation(shaderProgram, "time");

    // Just to make it bigger
    glPointSize(1.0f);

    // For the colors
    GLint colorLoc = glGetUniformLocation(shaderProgram, "uColor");

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        glUseProgram(shaderProgram);

        // ---------- Animate sine wave ----------
        int x_start = 50;
        int x_end = SCR_WIDTH - 50;
        int num_points = 500;
        float amplitude = 100.0f;  // pixels
        float frequency = 0.02f;   // controls wavelength
        float phase = currentFrame; // animate

        // Bresenham vertices for sine wave
        std::vector<float> verticesBresenham;
        for (int i = 0; i < num_points; ++i) {
            float x = x_start + i * (x_end - x_start) / float(num_points - 1);
            float y = SCR_HEIGHT / 2 + amplitude * sin(frequency * x + phase);
            float ndcX = 2.0f * x / SCR_WIDTH - 1.0f;
            float ndcY = 2.0f * y / SCR_HEIGHT - 1.0f;
            verticesBresenham.push_back(ndcX);
            verticesBresenham.push_back(ndcY);
        }

        // Xiaolin Wu vertices for sine wave
        std::vector<Vertex> verticesWu;
        for (int i = 0; i < num_points; ++i) {
            float x = x_start + i * (x_end - x_start) / float(num_points - 1);
            float y = SCR_HEIGHT / 2 + amplitude * sin(frequency * x + phase);
            int y_int = int(floor(y));
            float frac = y - y_int;

            float ndcX1 = 2.0f * x / SCR_WIDTH - 1.0f;
            float ndcY1 = 2.0f * y_int / SCR_HEIGHT - 1.0f;
            verticesWu.push_back({ ndcX1, ndcY1, 1.0f - frac });

            float ndcY2 = 2.0f * (y_int + 1) / SCR_HEIGHT - 1.0f;
            verticesWu.push_back({ ndcX1, ndcY2, frac });
        }

        // ---------- Cell 1: Top-left (Bresenham, white background) ----------
        glViewport(0, SCR_HEIGHT / 2, SCR_WIDTH / 2, SCR_HEIGHT / 2);
        glEnable(GL_SCISSOR_TEST);
        glUniform3f(colorLoc, 1.0f, 0.0f, 1.0f); // magenta
        glScissor(0, SCR_HEIGHT / 2, SCR_WIDTH / 2, SCR_HEIGHT / 2);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, verticesBresenham.size() * sizeof(float), verticesBresenham.data(), GL_DYNAMIC_DRAW);
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, verticesBresenham.size() / 2);

        // ---------- Cell 2: Bottom-left (Bresenham, black background) ----------
        glViewport(0, 0, SCR_WIDTH / 2, SCR_HEIGHT / 2);
        glEnable(GL_SCISSOR_TEST);
        glUniform3f(colorLoc, 1.0f, 1.0f, 0.0f); // yellow
        glScissor(0, 0, SCR_WIDTH / 2, SCR_HEIGHT / 2);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, verticesBresenham.size() * sizeof(float), verticesBresenham.data(), GL_DYNAMIC_DRAW);
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, verticesBresenham.size() / 2);

        // ---------- Cell 3: Top-right (Xiaolin Wu, white background) ----------
        glViewport(SCR_WIDTH / 2, SCR_HEIGHT / 2, SCR_WIDTH / 2, SCR_HEIGHT / 2);
        glEnable(GL_SCISSOR_TEST);
        glUniform3f(colorLoc, 1.0f, 0.0f, 1.0f); // magenta
        glScissor(SCR_WIDTH / 2, SCR_HEIGHT / 2, SCR_WIDTH / 2, SCR_HEIGHT / 2);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindBuffer(GL_ARRAY_BUFFER, VBOWu);
        glBufferData(GL_ARRAY_BUFFER, verticesWu.size() * sizeof(Vertex), verticesWu.data(), GL_DYNAMIC_DRAW);
        glBindVertexArray(VAOWu);
        glDrawArrays(GL_POINTS, 0, verticesWu.size());
        glDisable(GL_BLEND);

        // ---------- Cell 4: Bottom-right (Xiaolin Wu, black background) ----------
        glViewport(SCR_WIDTH / 2, 0, SCR_WIDTH / 2, SCR_HEIGHT / 2);
        glEnable(GL_SCISSOR_TEST);
        glUniform3f(colorLoc, 1.0f, 1.0f, 0.0f); // yellow
        glScissor(SCR_WIDTH / 2, 0, SCR_WIDTH / 2, SCR_HEIGHT / 2);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindBuffer(GL_ARRAY_BUFFER, VBOWu);
        glBufferData(GL_ARRAY_BUFFER, verticesWu.size() * sizeof(Vertex), verticesWu.data(), GL_DYNAMIC_DRAW);
        glBindVertexArray(VAOWu);
        glDrawArrays(GL_POINTS, 0, verticesWu.size());
        glDisable(GL_BLEND);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    // Clean up
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAOWu);
    glDeleteBuffers(1, &VBOWu);

    glfwTerminate();
    return 0;
}

// process all input
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// framebuffer resize callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// mouse callback
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {}

// scroll callback
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {}
