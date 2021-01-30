#include "Chunk.h"


Chunk::Chunk(Config* config, int x, int y)
:gridPosX(x),gridPosY(y)
{
    config_struct = config;
    height_map = new float[config_struct->x*config_struct->y];
    seed = config_struct->seed + (x & 0xFFFF) << 16 | (y & 0xFFFF);
}


Chunk::~Chunk()
{
    delete height_map;
}


void Chunk::init(float* pMap)
{
    float* map;
    float* seeds;
    if(pMap == nullptr)
    {
        setLehmer((uint32_t) seed);
    
   
        map = new float[config_struct->x * config_struct->y];
        seeds = new float[config_struct->x * config_struct->y];
    
        for(int i = 0; i < config_struct->x*config_struct->y;i++)
        {
            //seeds[i] = (float)rand() / (float)RAND_MAX; b
            seeds[i] = randdouble(0.0,1.0);
        }

        perlInNoise2D(config_struct->x,config_struct->y,seeds,config_struct->octaves,config_struct->bias,map);

    }
    else{
        map = pMap;
    }

    



    int attribIndex =1;
    int stride = getStride();

    float *vbChunk = new float[detVbSize()];


    genVertexBuffer(vbChunk,map);

    unsigned int* chunkIB;
    
    //There is no need for an index buffer if we are just drawing the points
    if(genIB) 
    {

        chunkIB = new unsigned int[(config_struct->x-1)*(config_struct->y-1)*6]; 
        if(config_struct->primitive == GL_TRIANGLES) indexBufferTriangles(chunkIB);
        else if(config_struct->primitive == GL_LINES) indexBufferLines(chunkIB);
    }
    

    GLCall(glGenVertexArrays(1,&vao));
    GLCall(glBindVertexArray(vao));


    GLCall(glGenBuffers(1,&vbo));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER,vbo));
    GLCall(glBufferStorage(GL_ARRAY_BUFFER,detVbSize()*sizeof(float),&vbChunk[0],GL_MAP_READ_BIT | GL_DYNAMIC_STORAGE_BIT));


    GLCall(glEnableVertexAttribArray(0));
    GLCall(glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,getStride()*sizeof(float),(void*)0));

    GLCall(glEnableVertexAttribArray(attribIndex));
    GLCall(glVertexAttribPointer(attribIndex,stride-3,GL_FLOAT,GL_FALSE,getStride()*sizeof(float),(void*)(3*sizeof(float))));
    

    if(genIB)
    {
        GLCall(glGenBuffers(1,&ib));
        GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ib));
        GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER,(config_struct->x-1)*(config_struct->y-1)*6*sizeof(unsigned int),&chunkIB[0],GL_STATIC_DRAW));
    }



    GLCall(glBindVertexArray(0));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER,0));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0));

    if(config_struct->genNormals)
    {
        if(config_struct->perFaceNormals)
        {
            generateNormalsPerFace(vbChunk);
        }
        else{
            generateNormalsAveraged(vbChunk);
        }
    }
    if(config_struct->trees)
    {
        genTerrainTrees();
    }

    if(pMap == nullptr)
    {
        delete map;
        delete seeds;
    }
 
    delete vbChunk;
    if(genIB) delete chunkIB;

}

int Chunk::getStride()
{
    int stride = 3;

    if(config_struct->texture)
    {
        stride += 2;
    }
    else{
        stride += 3;
    }
  
    return stride;
}

int Chunk::detVbSize(){
    int stride = getStride();
    int size = 0;

    if(config_struct->genNormals && config_struct->perFaceNormals)
    {
        size = (config_struct->x-1)*(config_struct->y-1)*6 * stride;
        return size;
    }
    return config_struct->x * config_struct->y * stride;
}

float Chunk::interpolateFloat(float color1, float color2, float fraction)
{
    float min = std::min(color1,color2);
    float max = std::max(color1,color2);

    float val = (color1 - color2) * fraction + color2;

    return val;
}


float Chunk::getTerrainHeight(float x, float y)
{
    /* Multiplying the -1 by the y and by the posY is to flip the z because yk opengl be facing the negative z. If you decide to not do the flipping remember to get rid of them*/

    float localX =  x - (float)config_struct->posX - (gridPosX*config_struct->offset * (float)config_struct->x);
    float localY =  y - (float)config_struct->posY*-1 - (gridPosY*config_struct->offset * (float)config_struct->y);


    float squareSize = width * length / config_struct->y - 1;

    int gridX = std::floor(localX / config_struct->offset/*squareSize*/);
    int gridY = std::floor(localY / config_struct->offset/*squareSize*/);


    if(gridX < 0 || gridX > config_struct->x || gridY < 0 || gridY > config_struct->y)
    {
        return 0;
    }
    float xCoord = (std::fmod(localX , config_struct->offset)/*squareSize*/) / config_struct->offset /*squareSize*/;
    float yCoord = (std::fmod(localY , config_struct->offset)/*squareSize*/) / config_struct->offset /*squareSize*/;
    

    float answer;


    if(xCoord <= 1-yCoord) // Top left triangle
    {
        answer = barryCentric({0,height_map[gridX * config_struct->y + gridY],0},{1,height_map[(gridX+1)*config_struct->y+gridY],0},{0,height_map[gridX * config_struct->y + gridY + 1],1},{xCoord,yCoord});
    }
    else
    {    
        answer = barryCentric({1,height_map[(gridX+1) * config_struct->y + gridY],0},{1,height_map[(gridX+1)*config_struct->y+gridY+1],1},{0,height_map[gridX * config_struct->y + gridY + 1],1},{xCoord,yCoord});   
    }
    

    return answer;


}

float Chunk::barryCentric(std::vector<float> p1, std::vector<float> p2, std::vector<float> p3, std::vector<float> pos) const
    {
		float det = (p2[2] - p3[2]) * (p1[0] - p3[0]) + (p3[0] - p2[0]) * (p1[2] - p3[2]);
		float l1 = ((p2[2] - p3[2]) * (pos[0] - p3[0]) + (p3[0] - p2[0]) * (pos[1] - p3[2])) / det;
		float l2 = ((p3[2] - p1[2]) * (pos[0] - p3[0]) + (p1[0] - p3[0]) * (pos[1] - p3[2])) / det;
		float l3 = 1.0f - l1 - l2;
		return l1 * p1[1] + l2 * p2[1] + l3 * p3[1];
	}


void Chunk::determineColAttrib(float*& buffer,int place)
{
        float point_height = (buffer[place + 1]-config_struct->height/(15/7))/4.0f;
        glm::vec3 col {interpolateFloat(config_struct->color1[0],config_struct->color2[0],point_height),interpolateFloat(config_struct->color1[1],config_struct->color2[1],point_height),interpolateFloat(config_struct->color1[2],config_struct->color2[2],point_height)};
        clampColor(col);


            buffer[place+3] = col.r;
            buffer[place+4] = col.g;
            buffer[place+5] = col.b;

            if(config_struct->staticColor)
            {
                if(place % 18 != 0)
                {
                    buffer[place+3] = buffer[place - 3];
                    buffer[place+4] = buffer[place - 2];
                    buffer[place+5] = buffer[place - 1];

                }
            }
}


void Chunk::clampColor(glm::vec3& col)
{
    for(int i=0;i<3;i++)
    {
        if(col[i] < 0.0f) col[i] = 0.0f;
        if(col[i] > 1.0f) col[i] = 1.0f;
    }
}


void Chunk::determineTexAttrib(float*& buffer,int x, int y, int place)
{
   if(!config_struct->textureRepeat)
    {
        float xBlend = (float)x/(config_struct->x-1);
        float yBlend = (float)y/(config_struct->y-1);

        buffer[place+3] = yBlend;
        buffer[place+4] = xBlend;
    }
    else
    {
        float lengX = (float)config_struct->x/config_struct->xTextureRepeatOffset;
        float lengY = (float)config_struct->y/config_struct->yTextureRepeatOffset;


        float xLocationInSquare = ((float)x/(float)config_struct->x*lengX);
        float yLocationInSquare = ((float)y/(float)config_struct->y*lengY);


        buffer[place+3] = xLocationInSquare;
        buffer[place+4] = yLocationInSquare;
    }
}

glm::mat4 Chunk::getTerrainModelMatrix()
{
    return glm::translate(config_struct->modelMatrix,glm::vec3(config_struct->offset*config_struct->x*gridPosX,0.0f,config_struct->offset*config_struct->y*gridPosY*-1));
}


void Chunk::Draw(GLenum primitive)
{
    if(primitive == -1) primitive = config_struct->primitive;

    glBindVertexArray(vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ib);


    terrainShader->Bind();
    terrainShader->setUniformMat4f("model",getTerrainModelMatrix());
    if(config_struct->texture){
        terrainTexture->Bind(config_struct->textureSlot);
        terrainShader->setUniform1i(config_struct->textureUniformName,config_struct->textureSlot); 
    }

    if(genIB){
        glDrawElements(primitive,config_struct->x * config_struct->y * 6,GL_UNSIGNED_INT,nullptr);
    }
    else{
        glDrawArrays(primitive,0,detVbSize()/getStride());
    }
    
    terrainShader->UnBind();

    glBindVertexArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
    
    if(config_struct->trees)
    {
        treeShader->Bind();

        if(!config_struct->instancing)
        {
            for(int i = 0; i < treeModelMatrices.size(); i++)
            {
                treeShader->setUniformMat4f("model",treeModelMatrices[i]);
                treeModel->Draw(*treeShader,false);
            }
            treeShader->UnBind();

        }
    }
}


//For primitive GL_TRIANGLES
void Chunk::genVertexBuffer(float*& vbTerrain,float*& map)
{
    
    int xPlace = 0;
    int yPlace = 0;
    
    int stride = getStride();

    if(!config_struct->genNormals || (!config_struct->perFaceNormals)){
        for(int x=0; x < config_struct->x; x++)
        {
            for(int y =0; y < config_struct->y;y++)
            {
                fillVertex(vbTerrain,map,xPlace,stride,x,y);
                xPlace += stride;
            }
        }
    }

    else {
     for(int x=0; x < config_struct->x-1; x++)
        {
            for(int y =0; y < config_struct->y-1;y++)
            {

                fillVertex(vbTerrain,map,xPlace,stride,x,y);
                xPlace += stride;
                fillVertex(vbTerrain,map,xPlace,stride,x+1,y);
                xPlace += stride;
                fillVertex(vbTerrain,map,xPlace,stride,x,y+1);
                xPlace += stride;
                fillVertex(vbTerrain,map,xPlace,stride,x,y+1);
                xPlace += stride;
                fillVertex(vbTerrain,map,xPlace,stride,x+1,y+1);
                xPlace += stride;
                fillVertex(vbTerrain,map,xPlace,stride,x+1,y);
                xPlace += stride;
            }
        }
    
    
    }
   

}

void Chunk::fillVertex(float*& vbTerrain, float*& map,int& xPlace,const int& stride, int x, int y){
                vbTerrain[xPlace] = (float)x*config_struct->offset + config_struct->posX;
                vbTerrain[xPlace +1] = map[x * config_struct->y + y] * config_struct->height;  
                vbTerrain[xPlace + 2] = (float)(y*config_struct->offset*-1 + config_struct->posY);


                if(!config_struct->texture)
                {
                    determineColAttrib(vbTerrain,xPlace);

                }
                else
                {
                    determineTexAttrib(vbTerrain,x,y,xPlace);
                }
                
                height_map[x*config_struct->y + y] = vbTerrain[xPlace +1];
}


void Chunk::indexBufferTriangles(unsigned int*& buffer)
{
    int place = 0;
    for(int x = 0; x < config_struct->x-1; x++)
    {

        for(int y=0; y < config_struct->y-1;y++)
        {   
            
            buffer[place] = x*config_struct->y + y;
            buffer[place + 1] = (x+1)*config_struct->y + y;
            buffer[place + 2] = (x+1)*config_struct->y + y+1;

            buffer[place + 5] = (x+1)*config_struct->y + y+1;
            buffer[place + 4] = x*config_struct->y + y+1;
            buffer[place + 3] = x*config_struct->y + y;

            place += 6;

        }

    }

}

//For primitive GL_LINES
void Chunk::indexBufferLines(unsigned int*& buffer)
{
    int place = 0;
    for(int x = 0; x < config_struct->x-1; x++)
    {

        for(int y=0; y < config_struct->y-1;y++)
        {   
            
            buffer[place] = x*config_struct->y + y;
            buffer[place + 1] = (x+1)*config_struct->y + y;
            
            buffer[place + 2] = x*config_struct->y + y;
            buffer[place + 3] = x*config_struct->y + y+1;

            buffer[place + 4] = x*config_struct->y + y;
            buffer[place + 5] = (x+1)*config_struct->y + y+1;

            place += 6;

        }
    }
}

//Determine where in terrain the trees will be
void Chunk::genTerrainTrees()
{
    int gridsX = config_struct->x / config_struct->gridX;
    int gridsY = config_struct->y / config_struct->gridY;

    std::vector<std::vector<int>> treePositions;
    for(int x = 0; x < gridsX; x++)
    {
        for(int y = 0; y < gridsY; y++)
        {
            int result = randint(0,config_struct->treeChance);
            if(result == 1)
            {
                int amount;
                if (config_struct->maxNumInGrid == 1) amount = 1; //To avoid floating point errors
                else amount = randint(1,config_struct->maxNumInGrid);

                for(int i = 0; i < amount; i++)
                {
                    glm::vec2 temp = {x,y};
                    float xOffset = randdouble(0,config_struct->gridX);
                    float zOffset = randdouble(0,config_struct->gridY);


                    float positionX = (((temp[0] * config_struct->gridX) + xOffset) * config_struct->offset) + config_struct->posX + (gridPosX*config_struct->x*config_struct->offset);
                    float positionZ = ((((temp[1] * config_struct->gridY) + zOffset) * config_struct->offset) + config_struct->posY + gridPosY*config_struct->y*config_struct->offset) * -1;
                    float positionY = getTerrainHeight(positionX,positionZ*-1);
                    positionY = (config_struct->modelMatrix * glm::vec4(1.0f,positionY,1.0f,1.0f)).y;
    

                    if(!config_struct->waterTrue || positionY > config_struct->waterHeight)
                    {  
                        treePositions.push_back({x,y});
                        treeModelMatrices.push_back(glm::translate(glm::mat4(1.0f),glm::vec3(positionX,positionY,positionZ)));
                    }
                }


            }

        }

    }
    
}


void Chunk::newColors(std::vector<glm::vec3>& colors)
{
    if(config_struct->texture)
    {
        return;
    }
    float* newColorMap = new float[config_struct->x * config_struct->y];
    float* newSeedMap = new float[config_struct->x * config_struct->y]; 
    for(int i = 0; i < config_struct->x * config_struct->y;i++)
    {
        newSeedMap[i] = randdouble(0,1);
    }
    perlInNoise2D(config_struct->x,config_struct->y,newSeedMap,6,1.6,newColorMap);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    if(!(config_struct->genNormals && config_struct->perFaceNormals))
    {
        for(int i = 0; i < config_struct->x * config_struct->y;i++)
        {
            glm::vec3 col = colors[(int)colors.size()*newColorMap[i]];
            glBufferSubData(GL_ARRAY_BUFFER,i*getStride()*sizeof(float)+(3*sizeof(float)),3*sizeof(float),&col[0]);
        }
    }
    else{
        int place = 0;
        for(int x = 0; x < config_struct->x-1;x++)
        {
            for(int y = 0; y < config_struct->y-1;y++)
            {
                glm::vec3 col = colors[(int)colors.size()*newColorMap[x*config_struct->y+y]];
                glBufferSubData(GL_ARRAY_BUFFER,place*(getStride()*sizeof(float))+(3*sizeof(float)),3*sizeof(float),&col[0]);
                
                col = colors[(int)colors.size()*newColorMap[(x+1)*config_struct->y+y]];
                glBufferSubData(GL_ARRAY_BUFFER,(place+1)*(getStride()*sizeof(float))+(3*sizeof(float)),3*sizeof(float),&col[0]);

                col = colors[(int)colors.size()*newColorMap[(x+1)*config_struct->y+(y+1)]];
                glBufferSubData(GL_ARRAY_BUFFER,(place+2)*(getStride()*sizeof(float))+(3*sizeof(float)),3*sizeof(float),&col[0]);

                col = colors[(int)colors.size()*newColorMap[x*config_struct->y+(y+1)]];
                glBufferSubData(GL_ARRAY_BUFFER,(place+3)*(getStride()*sizeof(float))+(3*sizeof(float)),3*sizeof(float),&col[0]);
                
                col = colors[(int)colors.size()*newColorMap[(x+1)*config_struct->y+(y+1)]];
                glBufferSubData(GL_ARRAY_BUFFER,(place+4)*(getStride()*sizeof(float))+(3*sizeof(float)),3*sizeof(float),&col[0]);

                col = colors[(int)colors.size()*newColorMap[(x+1)*config_struct->y+y]];
                glBufferSubData(GL_ARRAY_BUFFER,(place+5)*(getStride()*sizeof(float))+(3*sizeof(float)),3*sizeof(float),&col[0]);
                
                place += 6;
            }
        }
    }
 

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    delete newColorMap;
}

/* This still does not work properly and i need to fix it*/

void Chunk::generateNormalsPerFace(float*& vertexBuffer)
{
    glm::vec3* normalArray = new glm::vec3[(detVbSize()/getStride())];

    int stride = getStride();


    int place = 0;
    int normalPlace = 0;
    
        for(int x =0; x < (detVbSize()/getStride())/3; x++)
        {

            glm::vec3 posA = {vertexBuffer[place],vertexBuffer[place+1],vertexBuffer[place+2]};
            place += stride;
            glm::vec3 posB = {vertexBuffer[place],vertexBuffer[place+1],vertexBuffer[place+2]};
            place += stride;
            glm::vec3 posC = {vertexBuffer[place],vertexBuffer[place+1],vertexBuffer[place+2]};

            glm::vec3 normal = getNormalFromPositions(posA,posB,posC);

            normalArray[normalPlace] = normal;
            normalArray[normalPlace+1] = normal;
            normalArray[normalPlace+2] = normal;

            place += stride;
            normalPlace += 3;
        }
    


    setNormalBuffer(normalArray);


    delete normalArray;
}

void Chunk::generateNormalsAveraged(float*& vertexBuffer)
{
    glm::vec3* normalArray = new glm::vec3[(detVbSize()/getStride())];

    int stride = getStride();
    int place = 0;

    for(int x = 0; x < config_struct->x; x++)
    {
        for(int y = 0; y < config_struct->y; y++)
        {
            std::vector<glm::vec3> norms;
            
            glm::vec3 self;
            glm::vec3 up;
            glm::vec3 left;

            glm::vec3 topleft;
            glm::vec3 down;
            glm::vec3 right;
            glm::vec3 bottomright;

            self = {vertexBuffer[stride*(x*config_struct->x + y)],vertexBuffer[stride*(x*config_struct->x + y)+1],vertexBuffer[stride*(x*config_struct->x + y)+2]}; 
            
            if(!(x < 1))
            {
                left = {vertexBuffer[stride*((x-1)*config_struct->x + y)],vertexBuffer[stride*((x-1)*config_struct->x + y)+1],vertexBuffer[stride*((x-1)*config_struct->x + y)+2]}; 
                if(!(y>config_struct->y-1))
                {
                    topleft = {vertexBuffer[stride*((x+1)*config_struct->x + y-1)],vertexBuffer[stride*((x+1)*config_struct->x + y-1)+1],vertexBuffer[stride*((x+1)*config_struct->x + y-1)+2]}; 
                }
            }
            if(!(x > config_struct->x-1))
            {
                right = {vertexBuffer[stride*((x+1)*config_struct->x + y)],vertexBuffer[stride*((x+1)*config_struct->x + y)+1],vertexBuffer[stride*((x+1)*config_struct->x + y)+2]}; 
                if(!(y<1))
                {
                    bottomright = {vertexBuffer[stride*((x+1)*config_struct->x + y-1)],vertexBuffer[stride*((x+1)*config_struct->x + y-1)+1],vertexBuffer[stride*((x+1)*config_struct->x + y-1)+2]}; 
                }
            }
            if(!(y < 1))
            {
                down = {vertexBuffer[stride*(x*config_struct->x + y-1)],vertexBuffer[stride*(x*config_struct->x + y-1)+1],vertexBuffer[stride*(x*config_struct->x + y-1)+2]}; 
            }
            if(!(y>config_struct->y-1))
            {
                up = {vertexBuffer[stride*(x*config_struct->x + y+1)],vertexBuffer[stride*(x*config_struct->x + y+1)+1],vertexBuffer[stride*(x*config_struct->x + y+1)+2]}; 
            }

            if(!(x<1))
            {
                if(!(y<1))
                {
                  norms.push_back(getNormalFromPositions(left,self,down));
                }
                if(!(y>config_struct->y-1))
                {
                  norms.push_back(getNormalFromPositions(up,self,topleft));
                  norms.push_back(getNormalFromPositions(left,self,topleft));
                }
            }      
            if(!(x>config_struct->x-1))
            {
                if(!(y<1))
                {
                    norms.push_back(getNormalFromPositions(bottomright,self,down));
                    norms.push_back(getNormalFromPositions(bottomright,self,right));
                }
                if(!(y>config_struct->y-1))
                {
                    norms.push_back(getNormalFromPositions(up,self,right));
                }
            }        
            glm::vec3 total = {0,0,0};
            int divisor = 1;
            for(int i =0;i<6;i++)
            {
                if(!(norms[i] == glm::vec3(-1.0f,-1.0f,-1.0f)))
                {
                    total.x += norms[i].x;
                    total.y += norms[i].y;
                    total.z += norms[i].z;
                    divisor++;
                }
            }
            glm::vec3 normal = {total.x/divisor,total.y/divisor,total.z/divisor};

            normalArray[place] = normal;
            place++;
        }
    }
        /*
        glm::vec3 posA = {vertexBuffer[indices[place]*stride],vertexBuffer[(indices[place]*stride)+1],vertexBuffer[(indices[place]*stride)+2]};
        place++;
        glm::vec3 posB = {vertexBuffer[indices[place]*stride],vertexBuffer[(indices[place]*stride)+1],vertexBuffer[(indices[place]*stride)+2]};
        place++;
        glm::vec3 posC = {vertexBuffer[indices[place]*stride],vertexBuffer[(indices[place]*stride)+1],vertexBuffer[(indices[place]*stride)+2]};
        place++;

        glm::vec3 vecA = posB - posA;
        glm::vec3 vecB = posC - posA;
        glm::vec3 normal = glm::normalize(glm::cross(vecA,vecB));
        if(normal.y < 0)
        {
            normal = normal * glm::vec3(-1.0f,-1.0f,-1.0f);
        }
        unAveragedNormals[i] = normal;*/
    
    setNormalBuffer(normalArray);
    delete normalArray;
}

glm::vec3 Chunk::getNormalFromPositions(glm::vec3 posA,glm::vec3 posB,glm::vec3 posC)
{
  
    glm::vec3 vecA = posB - posA;
    glm::vec3 vecB = posC - posA;
    glm::vec3 normal = glm::normalize(glm::cross(vecA,vecB));
    if(normal.y < 0)
    {
        normal = normal * glm::vec3(-1.0f,-1.0f,-1.0f);
    }
    return normal;
}

void Chunk::setNormalBuffer(glm::vec3*& normalArray)
{
    GLCall(glGenBuffers(1,&normals));
    
    GLCall(glBindVertexArray(vao));

    
    GLCall(glBindBuffer(GL_ARRAY_BUFFER,normals));
    GLCall(glBufferData(GL_ARRAY_BUFFER,(detVbSize()/getStride())*3*sizeof(float),&normalArray[0][0],GL_STATIC_DRAW));

    GLCall(glEnableVertexAttribArray(2));
    GLCall(glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0));

    GLCall(glBindBuffer(GL_ARRAY_BUFFER,0));
    glBindVertexArray(0);
}


