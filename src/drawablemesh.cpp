/*********************************************************************************************************************
 *
 * drawablemesh.cpp
 * 
 * Ray_compute
 * Ludovic Blache
 *
 *********************************************************************************************************************/

#include "drawablemesh.h"


DrawableMesh::DrawableMesh()
{
}


DrawableMesh::~DrawableMesh()
{
    glDeleteBuffers(1, &(m_vertexVBO));
    glDeleteBuffers(1, &(m_uvVBO));
    glDeleteBuffers(1, &(m_indexVBO));
    glDeleteVertexArrays(1, &(m_meshVAO));
}


void DrawableMesh::createQuadVAO()
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;


    // SCREEN

    // generate a quad in front of the camera
    vertices = { glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(-1.0f, 1.0f, 0.0f) };
    normals = { glm::vec3(1.0f, 1.0f,  1.0f), glm::vec3(1.0f, 1.0f,  1.0f), glm::vec3(1.0f, 1.0f,  1.0f), glm::vec3(1.0f, 1.0f,  1.0f) };
   
    // add UV coords so we can map textures on the creen quad
    std::vector<glm::vec2> texcoords{ glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec2(0.0f, 0.0f) };
    std::vector<uint32_t> indices {0, 1, 2, 2, 3, 0 };


    // Generates and populates a VBO for vertex coords
    glGenBuffers(1, &(m_vertexVBO));
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexVBO);
    size_t verticesNBytes = vertices.size() * sizeof(vertices[0]);
    glBufferData(GL_ARRAY_BUFFER, verticesNBytes, vertices.data(), GL_STATIC_DRAW);

    // Generates and populates a VBO for the element indices
    glGenBuffers(1, &(m_indexVBO));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexVBO);
    auto indicesNBytes = indices.size() * sizeof(indices[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesNBytes, indices.data(), GL_STATIC_DRAW);

    // Generates and populates a VBO for UV coords
    glGenBuffers(1, &(m_uvVBO));
    glBindBuffer(GL_ARRAY_BUFFER, m_uvVBO);
    size_t texcoordsNBytes = texcoords.size() * sizeof(texcoords[0]);
    glBufferData(GL_ARRAY_BUFFER, texcoordsNBytes, texcoords.data(), GL_STATIC_DRAW);


    // Creates a vertex array object (VAO) for drawing the mesh
    glGenVertexArrays(1, &(m_meshVAO));
    glBindVertexArray(m_meshVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexVBO);
    glEnableVertexAttribArray(POSITION);
    glVertexAttribPointer(POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, m_uvVBO);
    glEnableVertexAttribArray(UV);
    glVertexAttribPointer(UV, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexVBO);
    glBindVertexArray(m_defaultVAO); // unbinds the VAO

    // Additional information required by draw calls
    m_numVertices = (int)vertices.size();
    m_numIndices = (int)indices.size();

    // Clear temporary vectors
    vertices.clear();
    normals.clear();
    indices.clear();
}



void DrawableMesh::drawScreenQuad(GLuint _program, GLuint _tex)
{

        // Activate program
        glUseProgram(_program);

        
        // glBindImageTexture() bind an image and send it as unifnorm layout to shader
        // It replaces glActiveTexture() + glBindTexture() + glUniform1i() used for texture uniforms
        //
        // GL_RGBA8 (UNSIGNED_BYTE) -> declared as rgba8 in compute shader 
        // https://www.khronos.org/opengl/wiki/Image_Load_Store#Format_qualifiers
        // GL_READ_ONLY as we only m,ap the image of a geometry
        glBindImageTexture(0, _tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);

        
        glBindVertexArray(m_meshVAO);                       // bind the VAO
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexVBO);  // do not forget to bind the index buffer AFTER !

        glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, 0);

        glBindVertexArray(m_defaultVAO);


        glUseProgram(0);
}

