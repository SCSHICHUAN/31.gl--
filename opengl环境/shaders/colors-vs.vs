/* 
  vertex.strings
  opengl环境

  Created by Stan on 2024/8/12.
  
*/
#version 330 core
layout (location = 0) in vec3 aPos;   //顶点向量
layout (location = 1) in vec3 aNormal; //法线向量
layout (location = 2) in vec2 aTexCoords; //纹理坐标
layout (location = 5) in ivec4 aBoneIDs; //骨骼ID
layout (location = 6) in vec4 aWeights; //骨骼权重

//输出
out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

const int MAX_BONES = 100;
uniform mat4 finalBonesMatrices[MAX_BONES];

void main()
{
    vec4 totalPosition = vec4(0.0f);
    vec3 totalNormal = vec3(0.0f);
    bool hasBoneInfluence = false;

    for(int i = 0 ; i < 4 ; i++)
    {
        if(aBoneIDs[i] == -1)
            continue;
        if(aBoneIDs[i] >= MAX_BONES)
        {
            totalPosition = vec4(aPos, 1.0f);
            totalNormal = aNormal;
            break;
        }

        vec4 localPosition = finalBonesMatrices[aBoneIDs[i]] * vec4(aPos, 1.0f);
        totalPosition += localPosition * aWeights[i];

        vec3 localNormal = mat3(finalBonesMatrices[aBoneIDs[i]]) * aNormal;
        totalNormal += localNormal * aWeights[i];
        hasBoneInfluence = true;
    }

    // 如果模型没有骨骼/权重数据（如普通 obj），走常规顶点变换，避免所有顶点塌缩到原点
    if(!hasBoneInfluence) {
        totalPosition = vec4(aPos, 1.0f);
        totalNormal = aNormal;
    }

    FragPos = vec3(model * totalPosition);
    Normal = mat3(transpose(inverse(model))) * totalNormal;//法向量防止物体拉升而丢失
    TexCoords = aTexCoords;

    gl_Position = projection * view * model * totalPosition;
}
