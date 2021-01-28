#pragma once
#include "json_parser.h"

#include "renderer.h"
#include <vector>
#include "Texture.h"
#include "Random.h"
#include "shader.h"
#include "model.h"
#include <glm/gtc/type_ptr.hpp>
#include "Water.h"
#include "terrainConfig.h"


class Chunk
{
    public:
        int gridPosY;
        int gridPosX;
    
        Chunk(Config* config, int posX, int posY);
        ~Chunk();
        
        void init(); 
        
        void setTreeModel(Model* tree){treeModel = tree;}

        inline int getVao() const {return vao;}
        inline int getIb() const {return ib;}

        void Draw(GLenum primitive = -1);

        float getTerrainHeight(float x, float y);

        void newColors(std::vector<glm::vec3>& colors);

        inline float getCollisionOffset()
        {
            return config_struct->collisionOffset;
        }

        glm::mat4 getTerrainModelMatrix();

        float length;
        float width;
        
        inline void setShader(shader* terrainSha){terrainShader = terrainSha;};
        inline void setTreeShader(shader* treeSha){treeShader = treeSha;}
        inline void setTerrainTexture(Texture* texture){terrainTexture = texture;}
        inline void setGenIb(bool b){genIB = b;}
        
        shader* terrainShader;
        shader* treeShader;
        Texture* terrainTexture;
        Model* treeModel;
        std::vector<glm::mat4> treeModelMatrices;


    private:
        uint32_t seed;

        void read_config_file(std::string& name);
        float interpolateFloat(float color1, float color2, float fraction);


        bool genIB = true;
        unsigned int vao;
        unsigned int vbo;
        unsigned int ib;
        unsigned int modelMatrixBuffer; //If instancing for trees is true this will be the id for that buffer
        unsigned int normals; //If genNormals is set to true

        Config* config_struct;

        int detVbSize();
        int getStride();
        void fillVertex(float*& vbTerrain,float*& map, int& xPlace,const int& stride, int x, int y);

        float barryCentric(std::vector<float> p1, std::vector<float> p2, std::vector<float> p3, std::vector<float> pos) const;
    
        void determineColAttrib(float *& buffer,int place);
        void clampColor(glm::vec3& col);
        void determineTexAttrib(float *& buffer,int x, int y, int place);

        void genVertexBuffer(float*& vbTerrain,float*& map);
        void indexBufferTriangles(unsigned int*& buffer);
        void indexBufferLines(unsigned int*& buffer);

        void generateNormalsPerFace(float*& vertexBuffer);
        void generateNormalsAveraged(float*& vertexBuffer);
        glm::vec3 getNormalFromPositions(glm::vec3 posA,glm::vec3 posB,glm::vec3 posC);
        void setNormalBuffer(glm::vec3*& normalArray);

        void genTerrainTrees();
        
        float* height_map;


};