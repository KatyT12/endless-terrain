#include "random_terrain.h"


Terrain::Terrain(std::string config_file)
{
    if(config_file == "")
    {

    }
    else
    {
        read_config_file(config_file);
    }
    
    int length = config_struct.y * config_struct.offset;
    int width = config_struct.x * config_struct.offset;

    if(config_struct.texture)
    {
        terrainTexture.InitTexture(config_struct.textureLocation,config_struct.wrapMode);
    }

    if(config_struct.geometryShader)
    {
        terrainShader.makeShader(config_struct.shaderLocation,true);
    }
    else
    {    
        terrainShader.makeShader(config_struct.shaderLocation);
    }

    shaders.push_back(&terrainShader);

    if(config_struct.waterTrue)shaders.push_back(&waterObj.waterShader);
    if(config_struct.uniformBuffer){
        terrainShader.proj_and_view_ubo = true;
    } 

    if(config_struct.trees) {
        treeShader.makeShader(config_struct.treeShader);
        shaders.push_back(&treeShader);
        if(config_struct.treeUniformBuffer)
        {
            treeShader.proj_and_view_ubo = true;
        }
    }
    if(config_struct.primitive == GL_POINTS || (config_struct.genNormals && config_struct.perFaceNormals)) genIB = false; else genIB = true; 
    
    for(int i=0; i < shaders.size();i++)
    {
        if(shaders[i]->proj_and_view_ubo){
            uboShaders.push_back(shaders[i]->getShaderID());
        }
        else {
            notUboShaders.push_back(shaders[i]);
        }

    }

}


Terrain::~Terrain()
{
    configuration->empty();
    delete configuration;
    glDeleteBuffers(1,&treeModelBuffer);
}



void Terrain::read_config_file(std::string& name)
{
    configuration = make_json(name);

    Json::Value temp = *configuration;

    auto perlinnoise = temp["perlinnoise"];
    
    config_struct.octaves = perlinnoise["octaves"].asInt();
    config_struct.bias = perlinnoise["bias"].asFloat();
    config_struct.seed = perlinnoise["seed"].asInt();

    auto dimensions = temp["dimensions"];
    config_struct.x = dimensions["x"].asInt();
    config_struct.y = dimensions["y"].asInt();
    config_struct.height = dimensions["max_height"].asFloat();

    config_struct.posX = dimensions["posX"].asFloat();
    config_struct.posY = dimensions["posY"].asFloat();

    config_struct.offset = dimensions["offset"].asFloat();

    if(!dimensions["collisionOffset"].isNull()) config_struct.collisionOffset = dimensions["collisionOffset"].asFloat();

    if(!dimensions["primitive"].isNull())
    {
        /*Only these three primitives for now, may add more in the future*/
        if (dimensions["primitive"].asString() == "GL_POINTS") config_struct.primitive = GL_POINTS;
        if (dimensions["primitive"].asString() == "GL_TRIANGLES") config_struct.primitive = GL_TRIANGLES;
        if (dimensions["primitive"].asString() == "GL_LINES") config_struct.primitive = GL_LINES;
    }


    config_struct.trees = dimensions["trees"].asBool();

    if(config_struct.trees)
    {
        auto grid = dimensions["grid"];
        
        config_struct.gridX = grid["gridX"].asInt();
        config_struct.gridY = grid["gridY"].asInt(); 

        if(!grid["chancePerGrid"].isNull()) config_struct.treeChance = grid["chancePerGrid"].asFloat();
        if(!grid["maxNumInGrid"].isNull()) config_struct.maxNumInGrid = grid["maxNumInGrid"].asInt();

        if(!grid["treeModel"].isNull()) config_struct.treeModel = grid["treeModel"].asString();
        if(!grid["treeShader"].isNull()) config_struct.treeShader = grid["treeShader"].asString();
        config_struct.treeUniformBuffer = grid["treeUniformBufferForProjAndView"].asBool();
        config_struct.instancing = grid["instancing"].asBool();

    }

    auto colors = temp["colors"];


    config_struct.staticColor = colors["staticColor"].asBool();
    config_struct.texture = colors["texture"].asBool();
    if(config_struct.texture)
    {
        config_struct.textureLocation = colors["textureLocation"].asString();
        if(!colors["textureSlot"].isNull())
        {
            config_struct.textureSlot = colors["textureSlot"].asInt();
        }
       
        config_struct.textureRepeat = colors["textureRepeat"].asBool();
        if(config_struct.textureRepeat)
        {
    
            auto textureRepeatConfig = colors["textureRepeatConfig"];
            config_struct.xTextureRepeatOffset = textureRepeatConfig["xTextureRepeatOffset"].asFloat();
            config_struct.yTextureRepeatOffset = textureRepeatConfig["yTextureRepeatOffset"].asFloat();
            if(textureRepeatConfig["wrapMode"] == "GL_WRAP_BORDER") config_struct.wrapMode = GL_WRAP_BORDER;
         

        }
    }
    else
    {   
        if(!colors["color1"].isNull())
        {

            Json::Value tmp = colors["color1"];
            config_struct.color1 = {tmp[0].asFloat(),tmp[1].asFloat(),tmp[2].asFloat()};
        }
        if(!colors["color2"].isNull())
        {

            Json::Value tmp = colors["color2"];
            config_struct.color2 = {tmp[0].asFloat(),tmp[1].asFloat(),tmp[2].asFloat()};
        }
    }
    
    auto shaderConfig = temp["shader"];
    if(!shaderConfig["shaderLocation"].isNull())
    {
        config_struct.shaderLocation = shaderConfig["shaderLocation"].asString();
        if(!shaderConfig["textureUniformName"].isNull()) config_struct.textureUniformName = shaderConfig["textureUniformName"].asString();
        config_struct.geometryShader = shaderConfig["geometryShader"].asBool();
        config_struct.uniformBuffer = shaderConfig["uniformBufferForProjAndView"].asBool();

    }

    if(!temp["matrices"].isNull())
    {
        auto matricesConfig = temp["matrices"];
        if(!matricesConfig["model"].isNull()) {
            float model[16];

            auto modelJson = matricesConfig["model"];
            int index =0;
            for (const auto& el : modelJson)
            {   
                model[index] = el.asFloat();
                index++;
            }

            config_struct.modelMatrix = transpose(glm::make_mat4(model));            
        }
    }

    config_struct.genNormals = temp["genNormals"].asBool();
    if(!temp["lighting"].isNull() && config_struct.genNormals)
    {
        auto lightingConfig = temp["lighting"];
        config_struct.perFaceNormals = lightingConfig["perFaceNormals"].asBool();
    }

    config_struct.waterTrue = temp["waterPresent"].asBool();
    if(config_struct.waterTrue)
    {
        if(!temp["water"].isNull())
        {
            auto waterConfig = temp["water"];
            if(!waterConfig["waterY"].isNull()) config_struct.waterHeight = waterConfig["waterY"].asFloat();
            if(!waterConfig["waterColor"].isNull())
            {
                Json::Value tmp = waterConfig["waterColor"];
                config_struct.waterColor = {tmp[0].asFloat(),tmp[1].asFloat(),tmp[2].asFloat()};
            }
        }
    }


}

//In desperate need of some abstraction idk
void Terrain::init()
{
    setLehmer((uint32_t) config_struct.seed);

    if(config_struct.waterTrue)
    {
        waterObj = Water(10,config_struct.x*config_struct.offset * 2);
        waterObj.setHeight(config_struct.waterHeight);
        waterObj.waterColor = config_struct.waterColor;
        waterObj.genBuffer();
        waterObj.setShader("res/shaders/2d.shader");
        waterObj.setExtraModel(glm::translate(glm::mat4(1.0f),glm::vec3((-config_struct.x*config_struct.offset),0.0f,(config_struct.y*config_struct.offset))));
    }

    if(config_struct.trees)
    {
        tree.loadModel(config_struct.treeModel);
    }

    genNewChunk(-1,0);
    genNewChunk(0,0);
    genNewChunk(-1,-1);
    genNewChunk(0,-1);

    if(config_struct.trees && config_struct.instancing)
    {
        genTreeModelBuffer();
    }

}


float Terrain::getTerrainHeight(float x, float y)
{
    /* Multiplying the -1 by the y and by the posY is to flip the z because yk opengl be facing the negative z. If you decide to not do the flipping remember to get rid of them*/
    y *= -1;

    float localX =  x - (config_struct.modelMatrix*glm::vec4(config_struct.posX,0.0f,config_struct.posY,1.0f)).r;
    float localY =  y - (config_struct.modelMatrix*glm::vec4(config_struct.posX,0.0f,config_struct.posY,1.0f)).b;

    int gridX = localX/(config_struct.x*config_struct.offset);
    int gridY = localY/(config_struct.y*config_struct.offset);
    
    if(localX < 0) gridX--;
    if(localY < 0) gridY--;


    float answer = 0;
    

    for(Chunk* ch : chunks)
    {
        if(ch->gridPosX == gridX && ch->gridPosY == gridY)
        {
            answer = ch->getTerrainHeight(x,y);
        }
    }    
    return answer;
}

void Terrain::Draw(GLenum primitive)
{
    for(int i =0;i<chunks.size();i++)
    {
        chunks[i]->Draw();
    }
    if(config_struct.waterTrue)
    {
        waterObj.setExtraModel(glm::translate(glm::mat4(1.0f),glm::vec3(camPos->x-config_struct.x*config_struct.offset,0.0f,camPos->z+(config_struct.y*config_struct.offset))));
        waterObj.Draw();
    }
    if(config_struct.trees && config_struct.instancing)
    {
        treeShader.Bind();
        tree.DrawInstanced(treeShader,treeAmount,false);
        treeShader.UnBind();
    }
}

void Terrain::newColors(std::vector<glm::vec3>& colors)
{
    for(int i=0;i<chunks.size();i++)
    {
        chunks[i]->newColors(colors);
    }
}

void Terrain::genTreeModelBuffer()
{
    int amount = 0;
    for(Chunk* ch : chunks) 
    {
        amount += ch->treeModelMatrices.size();
    }
    glm::mat4* modelBuffer = new glm::mat4[amount];

    int n = 0;
    for(int ch=0;ch<chunks.size();ch++)
    {
        for(glm::mat4 m : chunks[ch]->treeModelMatrices)
        {
            modelBuffer[n++] = m;
        }
    }
    glGenBuffers(1,&treeModelBuffer);
    glBindBuffer(GL_ARRAY_BUFFER,treeModelBuffer);
    glBufferData(GL_ARRAY_BUFFER,amount * sizeof(glm::mat4),&modelBuffer[0],GL_STATIC_DRAW);

    for(int i = 0; i < tree.meshes.size(); i++)
    {
        unsigned int vao = tree.meshes[i].VAO;
        glBindVertexArray(vao);
        std::size_t sizeVec4 = sizeof(glm::vec4);
        glEnableVertexAttribArray(3); //Assuming that the third and onwards attribute is already empty and that the shader is using the 3rd attrib for it. May need to make this a setting in the config
        glVertexAttribPointer(3,4,GL_FLOAT,GL_FALSE,4 * sizeVec4,(void*)0);
        glEnableVertexAttribArray(4); 
        glVertexAttribPointer(4,4,GL_FLOAT,GL_FALSE,4 * sizeVec4,(void*) (1 * sizeVec4));
        glEnableVertexAttribArray(5); 
        glVertexAttribPointer(5,4,GL_FLOAT,GL_FALSE,4 * sizeVec4,(void*) (2 * sizeVec4));
        glEnableVertexAttribArray(6); 
        glVertexAttribPointer(6,4,GL_FLOAT,GL_FALSE,4 * sizeVec4,(void*) (3 * sizeVec4));
        glVertexAttribDivisor(3,1); //Every instance rather than every vertex shader iteration
        glVertexAttribDivisor(4,1);
        glVertexAttribDivisor(5,1);
        glVertexAttribDivisor(6,1);
        glBindVertexArray(0);
    }
        glBindBuffer(GL_ARRAY_BUFFER,0);
        treeAmount = amount;
}

void Terrain::removeChunk(int x, int y)
{
    for(int i =0; i < chunks.size();i++)
    {
        Chunk* ch = chunks[i];
        if(ch->gridPosX == x && ch->gridPosY == y)
        {
            ch->~Chunk();
            chunks.erase(chunks.begin() + i);
            chunkPositions.erase(chunkPositions.begin() + i);
        }
    }
}

void Terrain::genNewChunk(int x, int y)
{
    bool present = false;

    for(Chunk* ch : chunks)
    {
        if(ch->gridPosX == x && ch->gridPosY == y)
        {
            present = true;
        }
    }
    if(present)
    {
        return;
    }

    chunks.push_back(new Chunk(&config_struct,x,y));
    
    int n = chunks.size()-1; 
    chunks[n]->setGenIb(genIB);
    chunks[n]->setShader(&terrainShader);

    

    if(config_struct.trees)
    {
        chunks[n]->setTreeModel(&tree);
        chunks[n]->setTreeShader(&treeShader);
    }

    if(config_struct.texture)
    {
        chunks[n]->setTerrainTexture(&terrainTexture);
    }

    chunkPositions.push_back({x,y});
    //firstGen(chunks);
    chunks[n]->init();
    
}

void Terrain::checkBounds()
{
    float localX =  camPos->x - (config_struct.modelMatrix*glm::vec4(config_struct.posX,0.0f,config_struct.posY,1.0f)).r;
    float localY =  (camPos->z*-1) - (config_struct.modelMatrix*glm::vec4(config_struct.posX,0.0f,config_struct.posY,1.0f)).b;

    int gridX = localX/(config_struct.x*config_struct.offset);
    int gridY = localY/(config_struct.y*config_struct.offset);
    
    float fgridX = localX/(config_struct.x*config_struct.offset);
    float fgridY = localY/(config_struct.y*config_struct.offset);

    glm::vec2 offset;
    if(localX < 0) 
    {
        gridX--;
        fgridX--;
        offset.x = (1 - ((fgridX - gridX)*-1)) * config_struct.x * config_struct.offset;
    }
    else{
        offset.x = (fgridX-(float)gridX)*config_struct.x *config_struct.offset;
    }
    if(localY < 0) 
    {
        gridY--;
        fgridY--;
        offset.y = (1 - ((fgridY - gridY)*-1)) * config_struct.x * config_struct.offset;

    }
    else{
        offset.y = (fgridY-(float)gridY)*config_struct.y *config_struct.offset;
    }

    glm::vec2 camGrid = {gridX,gridY};

    glm::vec2 tr;
    glm::vec2 tl;
    glm::vec2 br;
    glm::vec2 bl;

    int top = -123123213;
    int right = -123123213;
    int bottom = 123123213;
    int left = 123123213;
    for(int i=0; i < chunks.size();i++)
    {
        if(chunks[i]->gridPosX > right) right = chunks[i]->gridPosX;
        if(chunks[i]->gridPosY > top) top = chunks[i]->gridPosY;
        if(chunks[i]->gridPosY < bottom) bottom = chunks[i]->gridPosY;
        if(chunks[i]->gridPosX < left) left = chunks[i]->gridPosX;
    }
    
    tr = {right,top};
    tl = {left,top};
    br = {right, bottom};
    bl = {left, bottom};

    bool gen1 = false;
    bool gen2 = false;
    std::vector<glm::vec2> addgrid;
    std::vector<glm::vec2> rmgrid;
    
    std::cout << gridX << " " << gridY <<"\n";
    if(offset.x < config_struct.x*config_struct.offset && offset.y < config_struct.y*config_struct.offset)
    {
        if(camGrid == tl)
        {
            std::cout << "top left\n";
            if(offset.x < 0.5*config_struct.x * config_struct.offset)
            {
                gen1 = true;
                addgrid.push_back({camGrid.x-1,camGrid.y});
                rmgrid.push_back(tr);
                rmgrid.push_back(br);

            }
            if(offset.y > 0.5*config_struct.y * config_struct.offset)
            {
                gen2 = true;
                addgrid.push_back({camGrid.x,camGrid.y+1});
                rmgrid.push_back(bl);
                rmgrid.push_back(br);
            }
            if(gen1 && !gen2)
            {
                addgrid.push_back({camGrid.x-1,camGrid.y-1});
            }
            else if(!gen1 && gen2)
            {
                addgrid.push_back({camGrid.x+1,camGrid.y+1});
            }
            else if(gen1 && gen2)
            {
                addgrid.push_back({camGrid.x-1,camGrid.y+1});
            }
        }
        else if(camGrid == tr)
        {
            std::cout << "top right\n";

            if(offset.x > 0.5*config_struct.x * config_struct.offset)
            {
                gen1 = true;
                addgrid.push_back({camGrid.x+1,camGrid.y});
                rmgrid.push_back(tl);
                rmgrid.push_back(bl);

            }
            if(offset.y > 0.5*config_struct.y * config_struct.offset)
            {
                gen2 = true;
                addgrid.push_back({camGrid.x,camGrid.y+1});
                rmgrid.push_back(br);
                rmgrid.push_back(bl);
            }
            if(gen1 && !gen2)
            {
                addgrid.push_back({camGrid.x+1,camGrid.y-1});
            }
            else if(!gen1 && gen2)
            {
                addgrid.push_back({camGrid.x-1,camGrid.y+1});
            }
            else if(gen1 && gen2)
            {
                addgrid.push_back({camGrid.x+1,camGrid.y+1});
            }
        }
        else if(camGrid == bl)
        {
            std::cout << "bottom left\n";

            if(offset.x < 0.5*config_struct.x * config_struct.offset)
            {
                gen1 = true;
                addgrid.push_back({camGrid.x-1,camGrid.y});
                rmgrid.push_back(br);
                rmgrid.push_back(tr);

            }
            if(offset.y < 0.5*config_struct.y * config_struct.offset)
            {
                gen2 = true;
                addgrid.push_back({camGrid.x,camGrid.y-1});
                rmgrid.push_back(tl);
                rmgrid.push_back(tr);
            }

            if(gen1 && !gen2)
            {
                addgrid.push_back({camGrid.x-1,camGrid.y+1});
            }
            else if(!gen1 && gen2)
            {
                addgrid.push_back({camGrid.x+1,camGrid.y-1});
            }
            else if(gen1 && gen2)
            {
                addgrid.push_back({camGrid.x-1,camGrid.y-1});
            }
        }
        else if(camGrid == br)
        {
            std::cout << "bottom right\n";

            if(offset.x > 0.5*config_struct.x * config_struct.offset)
            {
                gen1 = true;
                addgrid.push_back({camGrid.x+1,camGrid.y});
                rmgrid.push_back(bl);
                rmgrid.push_back(tl);

            }
            if(offset.y < 0.5*config_struct.y * config_struct.offset)
            {
                gen2 = true;
                addgrid.push_back({camGrid.x,camGrid.y-1});
                rmgrid.push_back(tr);
                rmgrid.push_back(tl);
            }
            if(gen1 && !gen2)
            {
                addgrid.push_back({camGrid.x+1,camGrid.y+1});
            }
            else if(!gen1 && gen2)
            {
                addgrid.push_back({camGrid.x-1,camGrid.y-1});
            }
            else if(gen1 && gen2)
            {
                addgrid.push_back({camGrid.x+1,camGrid.y-1});
            }
            
        }

        
        for(int i =0; i < addgrid.size();i++)
        {
            genNewChunk(addgrid[i].x,addgrid[i].y);
            std::cout << addgrid[i].x << " " << addgrid[i].y << "\n";
            std::cout << tr.x << " " << tr.y << "\n";

        }
        for(int i =0; i < rmgrid.size();i++)
        {
            removeChunk(rmgrid[i].x,rmgrid[i].y);
        }
        if(gen1 || gen2)
        {
            genTreeModelBuffer();
        }
    }
   
}


void Terrain::firstGen(std::vector<Chunk*> newChunks)
{
    std::vector<float*> maps;

    for(Chunk* ch : newChunks)
    {
        setLehmer(ch->seed);
        float* seeds = new float[config_struct.x * config_struct.y*4];
        for(int i =0;i<config_struct.x*config_struct.y*4;i++)
        {
            seeds[i] = randdouble(0,1);
        }
        float* temp = new float[config_struct.x * config_struct.y*4];
        perlInNoise2D(config_struct.x*2,config_struct.y*2,seeds,config_struct.octaves,config_struct.bias,temp);
        maps.push_back(temp);
        delete seeds;
    }

    float* averagedMap = new float[config_struct.x * config_struct.y * 4];
    for(int i=0; i < config_struct.x * config_struct.y * 4;i++)
    {
        float val = 0.0f;
        for(int j=0; j < maps.size();j++)
        {
            val += maps[j][i];
        }
        averagedMap[i] = val/(float)maps.size();
    }

    for(int i=0;i<maps.size();i++)
    {
        delete[] maps[i];
    }

    std::vector<float*> newMaps;
   
    newMaps = split(averagedMap,config_struct.x*2,config_struct.y*2);

    delete[] averagedMap;

    for(int i=0;i<newChunks.size();i++)
    {
        newChunks[i]->init(newMaps[i]);
    }

    for(int i=0;i < newMaps.size();i++)
    {
        delete[] newMaps[i];
    }
}

std::vector<float*> Terrain::split(float* buffer,int width, int length)
{
    float* tl = new float[(int)(width*length * 0.25f)];
    float* tr = new float[(int)(width*length * 0.25f)];
    float* bl = new float[(int)(width*length * 0.25f)];
    float* br = new float[(int)(width*length * 0.25f)];

    int place = 0;
    for(int x=0; x < width/2;x++)
    {
        for(int y = length/2; y < length;y++)
        {
            tl[place] = buffer[x*length + y];;
            place++;
        }
    }

    place = 0;
    for(int x=width/2; x < width;x++)
    {
        for(int y = length/2; y < length;y++)
        {
            tr[place] = buffer[x*length + y];;
            place++;
        }
    }

    place = 0;
    for(int x=0; x < width/2;x++)
    {
        for(int y = 0; y < length/2;y++)
        {
            bl[place] = buffer[x*length + y];
            place++;
        }
    }
    
    place = 0;
    for(int x=width/2; x < width;x++)
    {
        for(int y = 0; y < length/2;y++)
        {
            br[place] = buffer[x*length + y];;
            place++;
        }
    }

    std::vector<float*> s = {tl,tr,bl,br};
    return s;

}

void Terrain::genChunkBuffer(std::vector<Chunk*> newChunks)
{

}


