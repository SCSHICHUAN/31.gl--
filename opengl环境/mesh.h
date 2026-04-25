//
//  mesh.h
//  opengl环境
//
//  Created by Stan on 2024/9/21.
//

#ifndef mesh_h
#define mesh_h

#include <glad/glad.h>
#include <iostream>
#include <cmath>
#include "shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <vector>

using namespace std;
#define MAX_BONE_INFLUENCE 4

struct Vertex {
    glm::vec3 Position; //顶点向量
    glm::vec3 Normal;   //顶点法向量
    glm::vec2 TexCoords;//纹理坐标

    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];

    Vertex() {
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
            m_BoneIDs[i] = -1;
            m_Weights[i] = 0.0f;
        }
    }

    void addBoneData(int boneID, float weight) {
        // 首先检查是否已经有相同的骨骼ID
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
            if (m_BoneIDs[i] == boneID) {
                // 如果已经有相同的骨骼ID，我们只更新权重（虽然不应该发生）
                m_Weights[i] += weight;
                return;
            }
        }

        // 检查是否有空位
        for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
            if (m_BoneIDs[i] == -1) {
                m_BoneIDs[i] = boneID;
                m_Weights[i] = weight;
                return;
            }
        }

        // 如果已经有4个骨骼影响，找到权重最小的骨骼并替换
        int minWeightIndex = 0;
        float minWeight = m_Weights[0];

        for (int i = 1; i < MAX_BONE_INFLUENCE; i++) {
            if (m_Weights[i] < minWeight) {
                minWeight = m_Weights[i];
                minWeightIndex = i;
            }
        }

        // 只有在新权重大于最小权重时才替换
        if (weight > minWeight) {
            m_BoneIDs[minWeightIndex] = boneID;
            m_Weights[minWeightIndex] = weight;
        }
    }
};

struct Texture {
    unsigned int id; //纹理id
    string type;     //纹理类型
    string path;
};

class Mesh {
public:
    vector<Vertex>      vertices;  //顶点向量数组
    vector<unsigned int> indices;  //纹理ID数组
    vector<Texture>     textures;  //纹理数组
    
    unsigned int VAO;
    // constructor
    Mesh(vector<Vertex> vertices,vector<unsigned int> indices,vector<Texture> textures){
        this->vertices = vertices;
        this->indices  = indices;
        this->textures = textures;
        
        setupMesh();
    }
    
    //tender the mesh
    void Draw(Shader &shader){
        unsigned int diffuseNr  = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr   = 1;
        unsigned int heightNr   = 1;

        bool useAlphaCutout = false;
        
        for(unsigned int i = 0; i < textures.size(); i++){
            glActiveTexture(GL_TEXTURE0 + i);//激活纹理 GL_TEXTURE0 ....GL_TEXTUREi
            glBindTexture(GL_TEXTURE_2D,textures[i].id);//绑定纹理的位置为 id
            
            string number;
            string name = textures[i].type;
            if(name == "texture_diffuse"){
                name = "material.diffuse";
                number = std::to_string(diffuseNr++);
                
            }else if (name == "texture_specular"){
                name = "material.specular";
                number = std::to_string(specularNr++);
                
            }else if (name == "texture_normal"){
                number = std::to_string(normalNr++);
            }else if (name == "texture_height"){
                number = std::to_string(heightNr++);
            }
//          glUniform1i(glGetUniformLocation(shader.ID,(name + number).c_str()),i);//也要设置相同的 数字 到作色器 GL_TEXTURE0
            shader.setInt(name.c_str(), i);

            // 针对狼毛发贴图（Wolf_Fur.jpg 无 alpha），启用 cutout
            if (textures[i].type == "texture_diffuse") {
                if (textures[i].path.find("Fur") != string::npos || textures[i].path.find("fur") != string::npos) {
                    useAlphaCutout = true;
                }
            }

        }

        shader.setBool("useAlphaCutout", useAlphaCutout);
        
        //draw mseh
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES,static_cast<unsigned>(indices.size()),GL_UNSIGNED_INT,0);
        glBindVertexArray(0);
        
        glActiveTexture(GL_TEXTURE0);
    }
private:
    unsigned int VBO,EBO;
    
    void setupMesh(){
        //创建
        glGenVertexArrays(1,&VAO);
        glGenBuffers(1,&VBO);
        glGenBuffers(1,&EBO);
        
        glBindVertexArray(VAO);//绑定VAO
        
        //绑定VBO 和 导入数据
        glBindBuffer(GL_ARRAY_BUFFER,VBO);
        glBufferData(GL_ARRAY_BUFFER,vertices.size() * sizeof(Vertex),&vertices[0],GL_STATIC_DRAW);
        //绑定VBO 和 导入数据
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,indices.size() * sizeof(unsigned int),&indices[0],GL_STATIC_DRAW);
        
        //VAO 说明顶点属性 利用struct vector 中储存的特点,连续的数据
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex, TexCoords));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex, Tangent));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex, Bitangent));
        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5,4,GL_INT,sizeof(Vertex),(void*)offsetof(Vertex, m_BoneIDs));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6,4,GL_FLOAT,GL_FALSE,sizeof(Vertex),(void*)offsetof(Vertex, m_Weights));
        
        //解绑VAO
        glBindVertexArray(0);
    }
};





#endif /* mesh_h */
