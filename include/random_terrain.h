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
#include "Chunk.h"

class Terrain
{
    public:
    
        Terrain(std::string config_file = "");
        ~Terrain();
        void init();
        
        void Draw(GLenum primitive = -1);

        float getTerrainHeight(float x, float y);
        void newColors(std::vector<glm::vec3>& colors);

        inline float getCollisionOffset()
        {
            return config_struct.collisionOffset;
        }

        inline glm::mat4 getTerrainModelMatrix(){return config_struct.modelMatrix;};
        inline void setTerrainModelMatrix(glm::mat4 newModel){config_struct.modelMatrix = newModel;}
        inline bool treesPresent()const {return config_struct.trees;}
        inline void setCameraPos(glm::vec3* pos){camPos = pos;}

        //float length;
        //float width;
        Texture terrainTexture;
        shader terrainShader;
        shader treeShader;
        
        std::vector<unsigned int> uboShaders;
        std::vector<shader*> notUboShaders;
        void checkBounds();


    private:
        void read_config_file(std::string& name);
        float interpolateFloat(float color1, float color2, float fraction);


        void removeChunk(int x, int y);
        void genNewChunk(int x, int y);
        

        bool genIB = true;

        Water waterObj;

        Json::Value* configuration;
        Config config_struct;
        std::vector<shader*> shaders;

        glm::vec3* camPos;
        std::vector<glm::vec2> chunkPositions;

        std::vector<Chunk*>chunks;


        void genChunkBuffer(std::vector<Chunk*> newChunks);
        void firstGen(std::vector<Chunk*> newChunks);
        std::vector<float*> split(float* buffer,int width, int length);

        Model tree;
        void genTreeModelBuffer();
        unsigned int treeModelBuffer;
        int treeAmount;
        
};

