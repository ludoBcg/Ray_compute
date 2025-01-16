/*********************************************************************************************************************
 *
 * drawablemesh.h
 *
 * Buffer manager for mesh rendering
 * 
 * Ray_compute
 * Ludovic Blache
 *
 *********************************************************************************************************************/

#ifndef DRAWABLEMESH_H
#define DRAWABLEMESH_H

#include <map>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>

#include "utils.h"


// The attribute locations we will use in the vertex shader
enum AttributeLocation 
{
    POSITION = 0,
    UV = 1
};



/*!
* \class DrawableMesh
* \brief Drawable mesh
* Render a TriMesh using a Blinn-Phong shading model and texture mapping
*/
class DrawableMesh
{
    public:

        /*------------------------------------------------------------------------------------------------------------+
        |                                        CONSTRUCTORS / DESTRUCTORS                                           |
        +------------------------------------------------------------------------------------------------------------*/

        /*!
        * \fn DrawableMesh
        * \brief Default constructor of DrawableMesh
        */
        DrawableMesh();


        /*!
        * \fn ~DrawableMesh
        * \brief Destructor of DrawableMesh
        */
        ~DrawableMesh();


        /*------------------------------------------------------------------------------------------------------------+
        |                                              GETTERS/SETTERS                                                |
        +-------------------------------------------------------------------------------------------------------------*/

        inline void setSSAOKernel(std::vector<glm::vec3> _ssaoKernel) { m_ssaoKernel = _ssaoKernel; }
        inline void setNoiseTex(GLuint _noiseTex) { m_noiseTex = _noiseTex; }


        /*------------------------------------------------------------------------------------------------------------+
        |                                               OTHER METHODS                                                 |
        +-------------------------------------------------------------------------------------------------------------*/


        /*!
        * \fn createMeshVAO
        * \brief Create quad VAO and VBOs (for floor or screen quad).
        * \param _type : defines if the quad to build is a screen quad (=1) of a floor quad (=2)
        * \param _y : Y coords of quad to build (for floor quad only) 
        * \param _centerCoords : center of the scene (for floor quad only) 
        * \param _radScene : Radius of the scene (for floor quad only) 
        */
        void createQuadVAO();

     
        /*!
        * \fn drawScreenQuad
        * \brief Draw the screen quad, mapped with a given texture
        * \param _program : shader program
        * \param _tex : texture to map on the screen quad 
        * \param _isBlurOn : true to activate Gaussian blur
        * \param _isGaussH : true for horizontal blur, false for vertical blur
        * \param _filterWidth : Guassian fildter width
        */
        void drawScreenQuad(GLuint _program, GLuint _tex, bool _isBlurOn, bool _isGaussH = true, int _filterWidth = 0 );


    protected:

        /*------------------------------------------------------------------------------------------------------------+
        |                                                ATTRIBUTES                                                   |
        +-------------------------------------------------------------------------------------------------------------*/

        GLuint m_meshVAO;           /*!< mesh VAO */
        GLuint m_defaultVAO;        /*!< default VAO */

        GLuint m_vertexVBO;         /*!< name of vertex 3D coords VBO */
        GLuint m_uvVBO;             /*!< name of UV coords VBO */
        GLuint m_indexVBO;          /*!< name of index VBO */

        int m_numVertices;          /*!< number of vertices in the VBOs */
        int m_numIndices;           /*!< number of indices in the index VBO */

        GLuint m_albedoTex;         /*!< index of albedo map texture */

        GLuint m_noiseTex;          /*!< index of noise texture */
        std::vector<glm::vec3> m_ssaoKernel;

};
#endif // DRAWABLEMESH_H