/*********************************************************************************************************************
 *
 * main.cpp
 * 
 * Ray_compute
 * Ludovic Blache
 *
 *********************************************************************************************************************/


// Standard includes 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <cstdlib>
#include <algorithm>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "drawablemesh.h"


// Window
GLFWwindow *m_window;               /*!<  GLFW window */
int m_winWidth = 1024;              /*!<  window width (XGA) */
int m_winHeight = 720;              /*!<  window height (XGA) */
const unsigned int TEX_WIDTH = 512, TEX_HEIGHT = 512  ; /*!< textures dimensions  */

int m_nbSamples = 1;                /*!<  number of samples per pixel */
int m_nbBounces = 2;                /*!<  number of bounces (i.e., depth of path tracing) */
float m_lightIntensity = 1000.0f;   /*!<  light emission */


// 3D objects
std::unique_ptr<DrawableMesh> m_drawQuad;   /*!<  drawable object: screen quad */
std::vector<Sphere> m_spheres;              /*!<  Cornell box sphere geometry */

GLuint m_defaultVAO;            /*!<  default VAO */
GLuint m_uboSpheres;            /*!<  Sphere geometry Uniform Buffer Object */
GLuint m_ubo;


// Textures
GLuint m_screenTex;             /*!< Destination texture for screen-space processing (stores final lighting result) */
GLuint m_perlinR;
GLuint m_perlinG;
GLuint m_perlinB;

// shader programs
GLuint m_programQuad;           /*!< handle of the program object (i.e. shaders) for screen quad rendering */
GLuint m_programRay;            /*!< compute shader for ray tracing*/


std::vector<glm::vec3> m_ssaoKernel;
GLuint m_noiseTex;          

std::string shaderDir = "../../src/shaders/";   /*!< relative path to shaders folder  */
std::string modelDir = "../../models/";   /*!< relative path to meshes and textures files folder  */

void initialize();
void setupImgui(GLFWwindow *window);
void update();
void renderRays();
void displayScreen();
void resizeCallback(GLFWwindow* window, int width, int height);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void charCallback(GLFWwindow* window, unsigned int codepoint);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void cursorPosCallback(GLFWwindow* window, double x, double y);
void runGUI();
int main(int argc, char** argv);


    /*------------------------------------------------------------------------------------------------------------+
    |                                                      INIT                                                   |
    +-------------------------------------------------------------------------------------------------------------*/


void initialize()
{
    // Cornell box geometry described as spheres
    m_spheres = { Sphere( glm::vec3( -1e5 - 5,      0.0,      -10.0), glm::vec3(0.75, 0.25, 0.25), 1e5 ) ,	/* Left wall */
				  Sphere( glm::vec3(  1e5 + 5,      0.0,      -10.0), glm::vec3(0.25, 0.25, 0.75), 1e5 ) ,	/* Right wall */
				  Sphere( glm::vec3(      0.0,      0.0,  -1e5 - 15), glm::vec3(0.75, 0.75, 0.75), 1e5 ) ,	/* Back wall */
				  Sphere( glm::vec3(      0.0,      0.0,  1e5 + 0.1), glm::vec3(0.75, 0.75, 0.75), 1e5 ) ,	/* Front wall (just behing the camera) */
				  Sphere( glm::vec3(      0.0,  1e5 + 5,      -10.0), glm::vec3(0.75, 0.75, 0.75), 1e5 ) ,	/* Floor */
				  Sphere( glm::vec3(      0.0, -1e5 - 5,      -10.0), glm::vec3(0.75, 0.75, 0.75), 1e5 ) ,	/* Ceiling */
				  Sphere( glm::vec3(     -2.5,      3.0,      -12.5), glm::vec3(0.95,  0.5, 0.25), 2.0 ) ,	/* Mirror sphere */
			      Sphere( glm::vec3(      2.5,      3.0,       -8.5), glm::vec3(0.95,  0.5, 0.25), 1.5 ) ,	/* Glass sphere */
				  Sphere( glm::vec3(      0.0,     -4.5,      -10.0), glm::vec3( 1.0,  1.0,  1.0), 0.25 ) 	/* Light source */					 
    };

    assert(m_spheres.size() == NB_SPHERES);

    // Setup background color
    glClearColor(0.0f, 0.0f, 0.0f, 0.0);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // setup screen quad rendering
    m_drawQuad = std::make_unique<DrawableMesh>();
    m_drawQuad->createQuadVAO();

    //m_drawQuad->loadAlbedoTex( modelDir + "UVchecker.png" );

    // init screen texture
    buildScreenTex(&m_screenTex, TEX_WIDTH, TEX_HEIGHT);

    checkWorkGroups();

    m_programQuad = loadShaderProgram(shaderDir + "quadTex.vert", shaderDir + "quadTex.frag");
    m_programRay = loadCompShaderProgram(shaderDir + "rayTrace.comp");

    buildRandKernel(m_ssaoKernel);
    buildKernelRot(&m_noiseTex);
    buildPerlinTex(m_perlinR, 0);
    buildPerlinTex(m_perlinG, 100);
    buildPerlinTex(m_perlinB, 200);

    createSpheresUBO(m_spheres, m_ubo);
}



void setupImgui(GLFWwindow *window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    // enable keyboard controls?
    ImGui::StyleColorsDark();
    // platform and renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
}


    /*------------------------------------------------------------------------------------------------------------+
    |                                                     UPDATE                                                  |
    +-------------------------------------------------------------------------------------------------------------*/

void update()
{
}



    /*------------------------------------------------------------------------------------------------------------+
    |                                                     DISPLAY                                                 |
    +-------------------------------------------------------------------------------------------------------------*/


void renderRays()
{

    // use compute shader
    glUseProgram(m_programRay);



    // glBindImageTexture() bind an image and send it as unifnorm layout to shader
    // It replaces glActiveTexture() + glBindTexture() + glUniform1i() used for texture uniforms
    //
    // GL_RGBA8 (UNSIGNED_BYTE) -> declared as rgba8 in compute shader 
    // https://www.khronos.org/opengl/wiki/Image_Load_Store#Format_qualifiers
    // GL_WRITE_ONLY as we only writeinto the image
    glBindImageTexture(0, m_screenTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    // bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_noiseTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_perlinR);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_perlinG);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, m_perlinB);

    glUniform1i(glGetUniformLocation(m_programRay, "u_screenWidth"), m_winWidth);
    glUniform1i(glGetUniformLocation(m_programRay, "u_screenHeight"), m_winHeight);
    glUniform1i(glGetUniformLocation(m_programRay, "u_nbSamples"), m_nbSamples);
    glUniform1i(glGetUniformLocation(m_programRay, "u_nbBounces"), m_nbBounces);
    glUniform1f(glGetUniformLocation(m_programRay, "u_lightIntensity"), m_lightIntensity);

    for (unsigned int i = 0; i < 64; ++i)
    {
        std::string str = "u_samples[" + std::to_string(i) + "]";
        glm::vec3 myVec = m_ssaoKernel[i];
        glUniform3fv(glGetUniformLocation(m_programRay, str.c_str()), 1, &myVec[0]);
    }   

    glUniform1i(glGetUniformLocation(m_programRay, "u_noiseTex"), 0);
    glUniform1i(glGetUniformLocation(m_programRay, "u_perlinR"), 1);
    glUniform1i(glGetUniformLocation(m_programRay, "u_perlinG"), 2);
    glUniform1i(glGetUniformLocation(m_programRay, "u_perlinB"), 3);
 

    // execute compute shader on TEX_WIDTH x TEX_WIDTH x 1 local work groups (i.e., one local work group for each pixel in the image)
    glDispatchCompute((GLuint)TEX_WIDTH, (GLuint)TEX_WIDTH, 1);

  
    // make sure writing to image has finished before read
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);


}

void displayScreen()
{

        // bind dedicated FBO
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // Clear window with background color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 

        // draw screen quad with texture
        m_drawQuad->drawScreenQuad(m_programQuad, m_screenTex, false);
       
}


    /*------------------------------------------------------------------------------------------------------------+
    |                                                CALLBACK METHODS                                             |
    +-------------------------------------------------------------------------------------------------------------*/


void resizeCallback(GLFWwindow* window, int width, int height)
{
    m_winWidth = width;
    m_winHeight = height;
    glViewport(0, 0, width, height);

    // keep drawing while resize
    update();

    renderRays();
    displayScreen();

    // Swap between front and back buffer
    glfwSwapBuffers(m_window);
}


void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // return to init positon when "R" pressed
    if (key == GLFW_KEY_R && action == GLFW_PRESS) 
    {
       
    }
}


void charCallback(GLFWwindow* window, unsigned int codepoint)
{}


void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{}


void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{}


void cursorPosCallback(GLFWwindow* window, double x, double y)
{}




    /*------------------------------------------------------------------------------------------------------------+
    |                                                      MAIN                                                   |
    +-------------------------------------------------------------------------------------------------------------*/

void runGUI()
{

    if(ImGui::Begin("Settings"))
    {
        
        // ImGui frame rate measurement
        float frameRate = ImGui::GetIO().Framerate;
        ImGui::Text("FrameRate: %.3f ms/frame (%.1f FPS)", 1000.0f / frameRate, frameRate);

        ImGui::SliderInt("Samples per pixel", &m_nbSamples, 1, 8);

        ImGui::SliderInt("Number of bounces", &m_nbBounces, 1, 5);

        ImGui::SliderFloat("Light intensity", &m_lightIntensity, 0.0f, 2000.0f, "%.0f");
        

    } // end "Settings"

    
    ImGui::End();

    // render
    ImGui::Render();
}

int main(int argc, char** argv)
{

    // Initialize GLFW and create a window
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);//3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);//2
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // <-- activate this line on MacOS
    m_window = glfwCreateWindow(m_winWidth, m_winHeight, "Ray_compute demo", nullptr, nullptr);
    glfwMakeContextCurrent(m_window);
    glfwSetFramebufferSizeCallback(m_window, resizeCallback);
    glfwSetKeyCallback(m_window, keyCallback);
    glfwSetCharCallback(m_window, charCallback);
    glfwSetMouseButtonCallback(m_window, mouseButtonCallback);
    glfwSetScrollCallback(m_window, scrollCallback);
    glfwSetCursorPosCallback(m_window, cursorPosCallback);

    // init ImGUI
    setupImgui(m_window);


    // init GL extension wrangler
    glewExperimental = true;
    GLenum res = glewInit();
    if (res != GLEW_OK) 
    {
        fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
        return 1;
    }
    std::cout << std::endl
              << " Welcome to Ray_compute " << std::endl
              << "Log:" << std::endl
              << "OpenGL version: " << glGetString(GL_VERSION) << std::endl
              << "Vendor: " << glGetString(GL_VENDOR) << std::endl;

    glGenVertexArrays(1, &m_defaultVAO);
    glBindVertexArray(m_defaultVAO);

    // call init function
    initialize();

    // main rendering loop
    while (!glfwWindowShouldClose(m_window)) 
    {
        // process events
        glfwPollEvents();
        // start frame for ImGUI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        // build GUI
        runGUI();

        // idle updates
        update();
        // compute shader
        renderRays();
        // render
        displayScreen();

        // render GUI
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        // Swap between front and back buffer
        glfwSwapBuffers(m_window);
    }


    // delete shadow map FBO and texture
    glDeleteTextures(1, &m_screenTex);

    // Cleanup imGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Close window
    glfwDestroyWindow(m_window);
    glfwTerminate();

    std::cout << std::endl << "Bye!" << std::endl;

    return 0;
}
