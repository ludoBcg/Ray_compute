/*********************************************************************************************************************
 *
 * utils.h
 *
 * helper functions
 * 
 * Ray_compute
 * Ludovic Blache
 *
 *********************************************************************************************************************/


#ifndef UTILS_H
#define UTILS_H


#include <vector>
#include <array>
#include <fstream>
#include <sstream>
#include <iostream>
#include <random>

#define GLM_FORCE_RADIANS

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/noise.hpp>

#define QT_NO_OPENGL_ES_2
#include <GL/glew.h>
#include <GLFW/glfw3.h>



        /*------------------------------------------------------------------------------------------------------------+
        |                                             GEOMETRY BUFFER                                                 |
        +------------------------------------------------------------------------------------------------------------*/

const GLuint NB_SPHERES = 9;

// sphere structure
struct Sphere
{
    Sphere(glm::vec3 _center, glm::vec3 _color, float _radius)
        : center(_center), color(_color), radius(_radius)
        , pad1(0.0f), pad2(0.0f), pad3(0.0f)
    {}

    // Add intermediate padding for block alignement
    // cf. https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL
    // Must be consistent with struct Sphere defined in compute shader
    // and allows us to use the std140 layout
  	glm::vec3 center;
    float pad1;         
    glm::vec3 color;
    float pad2;
	float radius;
    glm::vec3 pad3;
};


/*!
* \fn createSpheresUBO
* \brief Creates a Uniform Buffer Object to send geometry of the scene to compute shader
* \param _spheres : geometry of the scene represented as spheres
* \param _ubo : UBO to be allocated and populated
* \return populated _ubo
*/
inline void createSpheresUBO(std::vector<Sphere>& _spheres, GLuint& _ubo)
{
    // 48 bytes per sphere (with intermediate padding)
    size_t NBytes = sizeof(Sphere);

    // Create and allocate UBO
    glGenBuffers(1, &_ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, _ubo);
    glBufferData(GL_UNIFORM_BUFFER, NBytes * _spheres.size(), NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Bind UBO to an indexed buffer target
    // Here using the index 1, which is correspond to uniform ValBlock 
    // in compute shader (binding = 1)
    // This way we don't have to use glGetUniformBlockIndex() and glUniformBlockBinding()
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, _ubo); 

    // Populate UBO with spheres
    glBindBuffer(GL_UNIFORM_BUFFER, _ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, NBytes * _spheres.size(), _spheres.data() ); 

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}



        /*------------------------------------------------------------------------------------------------------------+
        |                                         READ AND COMPILE SHADERS                                            |
        +------------------------------------------------------------------------------------------------------------*/


/*!
* \fn readShaderSource
* \brief read shader program and copy it in a string
* \param _filename : shader file name
* \return string containing shader program
*/
inline std::string readShaderSource(const std::string& _filename)
{
    std::ifstream file(_filename);
    std::stringstream stream;
    stream << file.rdbuf();

    return stream.str();
}



/*!
* \fn showShaderInfoLog
* \brief print out shader info log (i.e. compilation errors)
* \param _shader : shader
*/
inline void showShaderInfoLog(GLuint _shader)
{
    GLint infoLogLength = 0;
    glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &infoLogLength);
    std::vector<char> infoLog(infoLogLength);
    glGetShaderInfoLog(_shader, infoLogLength, &infoLogLength, &infoLog[0]);
    std::string infoLogStr(infoLog.begin(), infoLog.end());
    std::cerr << "[SHADER INFOLOG] " << infoLogStr << std::endl;
}



/*!
* \fn showProgramInfoLog
* \brief print out program info log (i.e. linking errors)
* \param _program : program
*/
inline void showProgramInfoLog(GLuint _program)
{
    GLint infoLogLength = 0;
    glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &infoLogLength);
    std::vector<char> infoLog(infoLogLength);
    glGetProgramInfoLog(_program, infoLogLength, &infoLogLength, &infoLog[0]);
    std::string infoLogStr(infoLog.begin(), infoLog.end());
    std::cerr << "[PROGRAM INFOLOG] " << infoLogStr << std::endl;
}



/*!
* \fn loadShaderProgram
* \brief load shader program from shader files
* \param _vertShaderFilename : vertex shader filename
* \param _fragShaderFilename : fragment shader filename
*/
inline GLuint loadShaderProgram(const std::string& _vertShaderFilename, const std::string& _fragShaderFilename, const std::string& _vertHeader="", const std::string& _fragHeader="")
{
    // read headers
    std::string vertHeaderSource, fragHeaderSource;
    vertHeaderSource = readShaderSource(_vertHeader);
    fragHeaderSource = readShaderSource(_fragHeader);


    // Load and compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    std::string vertexShaderSource = readShaderSource(_vertShaderFilename);
    if(!_vertHeader.empty() )
    {
        // if headers are provided, add them to the shader
        const char *vertSources[2] = {vertHeaderSource.c_str(), vertexShaderSource.c_str()};
        glShaderSource(vertexShader, 2, vertSources, nullptr);
    }
    else
    {
        // if no header provided, the shader is contained in a single file
        const char *vertexShaderSourcePtr = vertexShaderSource.c_str();
        glShaderSource(vertexShader, 1, &vertexShaderSourcePtr, nullptr);
    }
    glCompileShader(vertexShader);
    GLint success = 0;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) 
    {
        std::cerr << "[ERROR] loadShaderProgram(): Vertex shader compilation failed:" << std::endl;
        showShaderInfoLog(vertexShader);
        glDeleteShader(vertexShader);
        return 0;
    }


    // Load and compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    std::string fragmentShaderSource = readShaderSource(_fragShaderFilename);
    if(!_fragHeader.empty() )
    {
        // if headers are provided, add them to the shader
        const char *fragSources[2] = {fragHeaderSource.c_str(), fragmentShaderSource.c_str()};
        glShaderSource(fragmentShader, 2, fragSources, nullptr);
    }
    else
    {
        // if no header provided, the shader is contained in a single file
        const char *fragmentShaderSourcePtr = fragmentShaderSource.c_str();
        glShaderSource(fragmentShader, 1, &fragmentShaderSourcePtr, nullptr);
    }
    glCompileShader(fragmentShader);
    success = 0;
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) 
    {
        std::cerr << "[ERROR] loadShaderProgram(): Fragment shader compilation failed:" << std::endl;
        showShaderInfoLog(fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }


    // Create program object
    GLuint program = glCreateProgram();

    // Attach shaders to the program
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);


    // Link program
    glLinkProgram(program);

    // Check linking status
    success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) 
    {
        std::cerr << "[ERROR] loadShaderProgram(): Linking failed:" << std::endl;
        showProgramInfoLog(program);
        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    // Clean up
    glDetachShader(program, vertexShader);
    glDetachShader(program, fragmentShader);

    return program;
}



inline GLuint loadCompShaderProgram(const std::string& _compShaderFilename)
{
    // create compute shader
    GLuint compShader = glCreateShader(GL_COMPUTE_SHADER);

    // read shader
    std::string compShaderSource = readShaderSource(_compShaderFilename);
    const char *compShaderSourcePtr = compShaderSource.c_str();
    glShaderSource(compShader, 1, &compShaderSourcePtr, nullptr);

    // compile shader
    glCompileShader(compShader);

    // check for error
    GLint success = 0;
    glGetShaderiv(compShader, GL_COMPILE_STATUS, &success);
    if (!success) 
    {
        std::cerr << "[ERROR] loadCompShaderProgram(): Compute shader compilation failed:" << std::endl;
        showShaderInfoLog(compShader);
        glDeleteShader(compShader);
        return 0;
    }


    // Create program object
    GLuint program = glCreateProgram();

    // Attach shader to the program
    glAttachShader(program, compShader);

    // Link program
    glLinkProgram(program);

    // Check linking status
    success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) 
    {
        std::cerr << "[ERROR] loadShaderProgram(): Linking failed:" << std::endl;
        showProgramInfoLog(program);
        glDeleteProgram(program);
        glDeleteShader(compShader);
        return 0;
    }

    // Clean up
    glDetachShader(program, compShader);


    return program;
}


        /*------------------------------------------------------------------------------------------------------------+
        |                                               GENERATE TEXTURES                                             |
        +------------------------------------------------------------------------------------------------------------*/


// Init screen texture
inline void buildScreenTex(GLuint *_screenTex, unsigned int _texWidth, unsigned int _texHeight)
{

    // generate texture
    glGenTextures(1, _screenTex);
    // bind texture
    glBindTexture(GL_TEXTURE_2D, *_screenTex);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // create and empty texture with given dimensions
    // GL_RGBA8 (UNSIGNED_BYTE) -> declared as rgba8 in compute shader 
    // https://www.khronos.org/opengl/wiki/Image_Load_Store#Format_qualifiers
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  _texWidth, _texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

}


// Clear screen texture
inline void clearScreenTex(GLuint *_screenTex, unsigned int _texWidth, unsigned int _texHeight, std::vector<unsigned char>& _zeroBuffer)
{
    // bind texture
    glBindTexture(GL_TEXTURE_2D, *_screenTex);
    
    // update a sub part of the image (in this case the entire image) with buffer content (here a zero buffer)
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _texWidth, _texHeight, GL_RGBA, GL_UNSIGNED_BYTE, &_zeroBuffer.at(0));

}



/*
 * cf. https://learnopengl.com/Advanced-Lighting/SSAO
 */

inline float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}  

// Compute a list of randomly sampled 3D vecors
inline void buildRandKernel(std::vector<glm::vec3>& _ssaoKernel)
{
    _ssaoKernel.clear(); 

    // generate sample kernel 
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between 0.0 and 1.0
    std::default_random_engine generator;

    int i = 0;
    while(i < 64)//for (unsigned int i = 0; i < 64; ++i)
    {
        glm::vec3 sample( randomFloats(generator) * 2.0 - 1.0, 
                          randomFloats(generator) * 2.0 - 1.0, 
                          randomFloats(generator) );
        if( glm::length(sample) <= 1.0)
        {
            sample  = glm::normalize(sample);
            sample *= randomFloats(generator);
            float scale = (float)i / 64.0f; 

            // scale samples s.t. they're more aligned to center of kernel
            scale = lerp(0.1f, 1.0f, scale * scale);
            //sample *= scale;                      // remove artifacts !!!
            _ssaoKernel.push_back(sample);  
            i++;

        }
    }
}


/*!
* \fn buildPerlinTexRGB
* \brief Generate pseudo random Perlin noise, to be stored in a texture
* \param _noiseTex : 2D texture to containing the output random values
*/
inline void buildPerlinTex(GLuint& _perlinTex, unsigned int _texWidth, unsigned int _texHeight, float _scale)
{
    std::vector<glm::vec3> noise;
    for (unsigned int i = 0; i < _texWidth; i++)
    {
        for (unsigned int j = 0; j < _texHeight; j++)
        {
            // sample R values
            float valR = glm::perlin(glm::vec2((float)i / (float)(_texWidth) * _scale, (float)j / (float)(_texHeight) * _scale));
            // change range from [-1;1] to [0;1]
            valR = (valR + 1.0f) * 0.5f;

            // sample G values (add offset to avoid same values as R)
            float valG = glm::perlin(glm::vec2((float)(i + _texWidth) / (float)(_texWidth) * _scale, (float)(j + _texHeight) / (float)(_texHeight) * _scale));
            valG = (valG + 1.0f) * 0.5f;

            // sample B values (add offset to avoid same values as R and G)
            float valB = glm::perlin(glm::vec2((float)(i + _texWidth*2) / (float)(_texWidth) * _scale, (float)(j + _texHeight*2) / (float)(_texHeight) * _scale));
            valB = (valB + 1.0f) * 0.5f;

            noise.push_back( glm::vec3(valR, valG, valB) );
        }
    }

    glGenTextures(1, &_perlinTex);
    glBindTexture(GL_TEXTURE_2D, _perlinTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, _texWidth, _texHeight, 0, GL_RGB, GL_FLOAT, &noise[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}


        /*------------------------------------------------------------------------------------------------------------+
        |                                                     MISC.                                                   |
        +------------------------------------------------------------------------------------------------------------*/

// Get Work groups info for compute shader
inline void checkWorkGroups(glm::ivec3& _maxWorkGroupCount)
{
    // work group count
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &_maxWorkGroupCount[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &_maxWorkGroupCount[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &_maxWorkGroupCount[2]);

    std::cout << "max global (total) work group counts x: " << _maxWorkGroupCount[0] << "  y: " <<  _maxWorkGroupCount[1] << "  z: " <<  _maxWorkGroupCount[2] << std::endl;


    // work group size
    int work_grp_size[3];

    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);

    std::cout << "max local (in one shader) work group sizes x: " << work_grp_size[0] << "  y: " <<  work_grp_size[1] << "  z: " <<  work_grp_size[2] << std::endl;


    // max nb group work invocation
    int work_grp_inv;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);

    std::cout << "max local work group invocations: " << work_grp_inv << std::endl;

}
#endif // UTILS_H