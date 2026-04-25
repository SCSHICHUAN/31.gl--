//
//  main.cpp
//  opengl环境
//
//  Created by Stan on 2022/12/8.
//

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <vector>
#include "shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Camera.h"
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "mesh.h"
#include "model.h"
#include "animation.h"




// settings
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = SCR_WIDTH*3024.0/4032.0;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 2.8f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;    // 当前帧和上一帧之间的时间
float lastFrame = 0.0f;

// toggles
bool gEnableModelRotation = true; // key '1'
bool gEnableAnimation = true;     // key '2'
int gSelectedAnimIndex = -1;      // keys '3','4','5'
float gModelYaw = 0.0f;           // accumulated yaw (radians)
bool gAnimPaused = false;         // key 'P'
int gIdleAnimIndex = 0;           // chosen at load time
int gActionAnimIndex = -1;        // current one-shot action (sit/stand)
int gStandAnimIndex = -1;         // detected stand animation index (by name)
int gSitAnimIndex = -1;           // detected sit animation index (by name)

// extra animation bindings (after key '5')
// keys: 6,7,8,9,0 -> fill with remaining animation indices
std::vector<int> gExtraAnimKeySlots;
int gBrowseAnimIndex = 0;         // for '[' / ']' browsing through all animations

// globals for input-controlled switching
Model* gModel = nullptr;
Animator* gAnimator = nullptr;

// lighting
glm::vec3 lightPos(1.0f, 0.0f, 0.0f);
GLFWwindow* initWindow();
unsigned int loadTexture(const char *path);
void createVBOVAO(unsigned int &cubeVAO,unsigned int &lightCubeVAO,unsigned int &VBO);
std::vector<glm::vec3> getLightPositions();
std::vector<glm::vec3> getCubePositions();

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

int main(int argc, const char * argv[]) {
    
    GLFWwindow* window = initWindow();
    if(!window){
        return -1;
    }
    
    unsigned int cubeVAO = 0;
    unsigned int lightCubeVAO = 0;
    unsigned int VBO = 0;
    createVBOVAO(cubeVAO,lightCubeVAO,VBO);
    
//    unsigned int diffuseMap = loadTexture("./src/container2.png");
//    unsigned int specularMap = loadTexture("./src/container2_specular.png");
    
    
    //    Shader ourShader("./shaders/colors-vs.vs", "./shaders/colors-fs.fs");
    Shader lightCubeShader("./shaders/lamp-vs.vs", "./shaders/lamp-fs.fs");
    Shader ourShader("./shaders/colors-vs.vs", "./shaders/colors-fs.fs");
    Model ourModel("./Wolf-fbx/Wolf_One_fbx7.4_binary.fbx");
//    Model ourModel("./nanosuit/nanosuit.obj");
    printf("Model loaded, number of meshes: %d\n", ourModel.getMeshCount());
    printf("Number of bones in model: %d\n", ourModel.getBoneCount());
    Animator* animator = nullptr;

    if (ourModel.getAnimation()) {
        animator = new Animator(ourModel.getAnimation());
    }
    gModel = &ourModel;
    gAnimator = animator;
    // 记录启动时默认选中的常态动画 index（用于站起后自动回常态）
    if (ourModel.getAnimation()) {
        gIdleAnimIndex = ourModel.getAnimation()->getAnimationIndex();
    }
    
    // Build key slots for "remaining" animations (keys 6/7/8/9/0),
    // and enable browsing with '[' / ']'.
    {
        const int animCount = ourModel.getAnimationCount();
        gBrowseAnimIndex = (gIdleAnimIndex >= 0 && gIdleAnimIndex < animCount) ? gIdleAnimIndex : 0;
        
        auto findIdx = [&](const std::string& k) { return ourModel.findAnimationIndexByNameContains(k); };
        const int idxWalk  = findIdx("walk");
        const int idxRun   = findIdx("run");
        const int idxSit   = findIdx("sit");
        int idxCrawl = findIdx("crawl");
        if (idxCrawl == -1) idxCrawl = findIdx("prone");
        if (idxCrawl == -1) idxCrawl = findIdx("creep");
        const int idxStand = findIdx("stand");
        const int idxIdle  = findIdx("idle");
        gStandAnimIndex = idxStand;
        gSitAnimIndex = idxSit;
        
        std::vector<int> reserved;
        reserved.reserve(8);
        auto addReserved = [&](int v) {
            if (v < 0) return;
            if (std::find(reserved.begin(), reserved.end(), v) == reserved.end()) reserved.push_back(v);
        };
        addReserved(idxWalk);
        addReserved(idxRun);
        addReserved(idxSit);
        addReserved(idxCrawl);
        addReserved(idxStand);
        addReserved(idxIdle);
        addReserved(gIdleAnimIndex);
        
        std::vector<int> extras;
        extras.reserve(std::max(0, animCount - (int)reserved.size()));
        for (int i = 0; i < animCount; ++i) {
            if (std::find(reserved.begin(), reserved.end(), i) != reserved.end()) continue;
            extras.push_back(i);
        }
        
        gExtraAnimKeySlots.assign(5, -1); // 6,7,8,9,0
        for (int i = 0; i < 5 && i < (int)extras.size(); ++i) {
            gExtraAnimKeySlots[i] = extras[i];
        }
        
        std::cout << "[KeyMap] extra animations -> keys 6/7/8/9/0:" << std::endl;
        for (int i = 0; i < 5; ++i) {
            int idx = gExtraAnimKeySlots[i];
            if (idx < 0) {
                std::cout << "  key=" << (i == 4 ? '0' : (char)('6' + i)) << " -> <none>" << std::endl;
            } else {
                Animation* a = ourModel.getAnimation(idx);
                std::cout << "  key=" << (i == 4 ? '0' : (char)('6' + i)) << " -> index " << idx
                          << " name=" << (a ? a->getSourceName() : "<null>")
                          << std::endl;
            }
        }
        std::cout << "[KeyMap] browse all animations: '[' prev, ']' next (start at index " << gBrowseAnimIndex << ")" << std::endl;
    }
    
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    
    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // input
        // -----
        processInput(window);
        
        // render
        // ------
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        
        glm::vec3 lempColor(1.f,1.0f,1.0f);
//        float x = sin(glfwGetTime());
//        float y = sin(glfwGetTime());
//        float z = cos(glfwGetTime());
        
        static const glm::vec3 colors[] = {
            {1.0f, 0.0f, 0.0f}, // red
            {0.0f, 1.0f, 0.0f}, // green
            {0.0f, 0.0f, 1.0f}, // blue
            {1.0f, 1.0f, 0.0f}, // yellow
            {1.0f, 0.0f, 1.0f}, // magenta
            {0.0f, 1.0f, 1.0f}, // cyan
        };
        int idx = (int)(glfwGetTime() * 2.0) % (int)(sizeof(colors) / sizeof(colors[0])); // 0.5s per color
        lempColor = colors[idx];
        
        // view/projection transformations 裁截体矩阵
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();//相机矩阵
        
        // 重要：在设置 uniform 之前先激活对应的 shader program，
        // 否则 glUniform* 会写入“当前正在使用的 program”，导致模型 shader 的矩阵未被正确设置。
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        
        // world transformation
        glm::mat4 light_model = glm::mat4(1.0f);//模型
        ourShader.setMat4("model", light_model);
        // also draw the lamp object(s)
        lightCubeShader.use();
        lightCubeShader.setVec3("lampColor",lempColor);
        lightCubeShader.setMat4("projection", projection);
        lightCubeShader.setMat4("view", view);
        
        // we now draw as many light bulbs as we have point lights.模拟灯泡,具体的要在灯泡改颜色
        glBindVertexArray(lightCubeVAO);
        
       // 点光源的位置
//        lightPos = glm::vec3(0.5, y, 0);
        lightPos = glm::vec3(-0.4, 0.8, 0);
        light_model = glm::mat4(1.0f);
        light_model = glm::translate(light_model, lightPos);
        light_model = glm::scale(light_model, glm::vec3(0.05f)); // Make it a smaller cube
        lightCubeShader.setMat4("model", light_model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        
        
    
        // 更新动画
        if (animator && gEnableAnimation && !gAnimPaused) {
            animator->updateAnimation(deltaTime);
        }
        // 站起(一次性)播完后，自动回到常态 idle 循环
        if (animator && gEnableAnimation && !gAnimPaused && animator->isFinished()) {
            if (gStandAnimIndex != -1 && gActionAnimIndex == gStandAnimIndex) {
                Animation* idle = ourModel.getAnimation(gIdleAnimIndex);
                if (idle) {
                    animator->setLooping(true);
                    animator->playAnimation(idle, true);
                    gSelectedAnimIndex = gIdleAnimIndex;
                    gActionAnimIndex = -1;
                    std::cout << "[Anim] stand finished -> back to idle index " << gIdleAnimIndex << std::endl;
                }
            }
        }

        // render the loaded model
        // be sure to activate shader when setting uniforms/drawing objects
        ourShader.use();
        // lamp shader 之后 program 被切换过，这里重新确保矩阵写入当前 program
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setVec3("viewPos", camera.Position);
        ourShader.setFloat("material.shininess", 32.0f);

        // 传递骨骼变换矩阵
        if (animator && ourModel.getAnimation()) {
            // 先把所有骨骼矩阵初始化为单位矩阵，避免 shader 里采样到“未写入的 uniform”
            // （FBX 往往会引用很多 bone index；只传 map 里那部分会导致顶点变换结果随机/不可见）
            const int MAX_BONES = 100;
            glm::mat4 identity(1.0f);
            for (int i = 0; i < MAX_BONES; ++i) {
                string uniformName = "finalBonesMatrices[" + to_string(i) + "]";
                ourShader.setMat4(uniformName.c_str(), identity);
            }

            auto& finalBoneMatrices = animator->getFinalBoneMatrices();

            // 传递到着色器
            for (auto& entry : finalBoneMatrices) {
                string uniformName = "finalBonesMatrices[" + to_string(entry.first) + "]";
                ourShader.setMat4(uniformName.c_str(), entry.second);
            }
        }
//        ourShader.setInt("material.diffuse", 6);
//        ourShader.setInt("material.specular", 6);
        
        
        
        //平行光
        ourShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);//平行光方向
        ourShader.setVec3("dirLight.ambient", 1.0f, 1.0f, 1.0f);
        ourShader.setVec3("dirLight.diffuse", 0.9f, 0.9f, 0.9f);
        ourShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
        
        // 点光源
        glm::vec3 lightColor;
        lightColor = lempColor;
        glm::vec3 diffuseColor = lightColor   * glm::vec3(0.8f); // decrease the influence
        glm::vec3 ambientColor = diffuseColor * glm::vec3(0.05f); //
        ourShader.setVec3("pointLights[3].position", lightPos);
        ourShader.setVec3("pointLights[3].ambient", ambientColor);
        ourShader.setVec3("pointLights[3].diffuse", diffuseColor);
        ourShader.setVec3("pointLights[3].specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("pointLights[3].constant", 1.0f);
        ourShader.setFloat("pointLights[3].linear", 0.09f);
        ourShader.setFloat("pointLights[3].quadratic", 0.032f);
        // 手电筒
        ourShader.setVec3("spotLight.position", camera.Position);
        ourShader.setVec3("spotLight.direction", camera.Front);
        ourShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
        ourShader.setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
        ourShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("spotLight.constant", 1.0f);
        ourShader.setFloat("spotLight.linear", 0.09f);
        ourShader.setFloat("spotLight.quadratic", 0.032f);
        ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
        ourShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));
        
        
        
        // render the loaded model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.7f,0.0f)); // translate it down so it's at the center of the scene
        // 一开始给侧面（固定旋转 90 度）
        model = glm::rotate(model, glm::radians(60.0f) + gModelYaw, glm::vec3(0.0f, 1.0f, 0.0f));
        // 模型缩小一些
        model = glm::scale(model, glm::vec3(0.018f));
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);
        
        
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &lightCubeVAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}

//顶点着色器发送数据 和数据说明
void createVBOVAO(unsigned int &cubeVAO,unsigned int &lightCubeVAO,unsigned int &VBO){
    
    float vertices[] = {
        // -------顶点------     ----顶点法向量----    -纹理坐标-
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
        
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
        
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
        0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
        
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
        0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };
    
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);//导入数据
    
    glBindVertexArray(cubeVAO);
    // position attribute 顶点向量
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute 法向量
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coords 纹理坐标
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    //灯泡顶点向量 和上面的共用一套数据
    glGenVertexArrays(1, &lightCubeVAO);
    glBindVertexArray(lightCubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}
//加载纹理
unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    
    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format = 0;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }
    
    return textureID;
}
//盒子的位置
std::vector<glm::vec3> getCubePositions() {
    return {
        glm::vec3(0.0f,  0.0f,  0.0f),
        glm::vec3(2.0f,  5.0f, -15.0f),
        glm::vec3(-1.5f, -2.2f, -2.5f),
        glm::vec3(-3.8f, -2.0f, -12.3f),
        glm::vec3(2.4f, -0.4f, -3.5f),
        glm::vec3(-1.7f,  3.0f, -7.5f),
        glm::vec3(1.3f, -2.0f, -2.5f),
        glm::vec3(1.5f,  2.0f, -2.5f),
        glm::vec3(1.5f,  0.2f, -1.5f),
        glm::vec3(-1.3f,  1.0f, -1.5f)
    };
}
//灯的位置
std::vector<glm::vec3> getLightPositions() {
    return {
        glm::vec3( 0.7f,  0.2f,  2.0f),
        glm::vec3( 2.3f, -3.3f, -4.0f),
        glm::vec3(-4.0f,  2.0f, -12.0f),
        glm::vec3( 0.0f,  0.0f, -3.0f)
    };
}






GLFWwindow* initWindow(){
    //配置glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH,SCR_HEIGHT, "sc window", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return NULL;
    }
    //当前上下文设置为glfw
    glfwMakeContextCurrent(window);
    
    //初始化glad管理OpenGL函数地址
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return NULL;
    }
    
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);//窗口改变时回调
    glfwSetCursorPosCallback(window, mouse_callback);//鼠标移动
    glfwSetScrollCallback(window, scroll_callback);//鼠标滚轮
    // 告诉GLFW捕获我们的鼠标
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    return window;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // key toggle / hold
    static bool prev2 = false;
    static bool prev3 = false;
    static bool prev4 = false;
    static bool prev5 = false;
    static bool prevLB = false;
    static bool prevRB = false;
    static bool prevP = false;
    bool now1 = (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS);
    bool now2 = (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS);
    bool now3 = (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS);
    bool now4 = (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS);
    bool now5 = (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS);
    bool nowLB = (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS);
    bool nowRB = (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS);
    bool nowP = (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS);

    // 1：按住旋转，放开停止
    gEnableModelRotation = now1;
    if (gEnableModelRotation) {
        const float rotateSpeed = 1.2f; // rad/s
        gModelYaw += deltaTime * rotateSpeed;
    }

    if (nowP && !prevP) {
        gAnimPaused = !gAnimPaused;
        std::cout << "[Toggle] Anim pause: " << (gAnimPaused ? "ON" : "OFF") << std::endl;
    }

    auto switchAnim = [&](int idx, bool loop) {
        if (!gModel || !gAnimator) return;
        Animation* a = gModel->getAnimation(idx);
        if (!a) {
            std::cout << "[Anim] index " << idx << " not available (count=" << gModel->getAnimationCount() << ")" << std::endl;
            return;
        }
        gAnimator->setLooping(loop);
        gAnimator->playAnimation(a, true);
        gEnableAnimation = true;
        gSelectedAnimIndex = idx;
        std::cout << "[Anim] switched to index " << idx
                  << " name=" << a->getSourceName()
                  << " matchedChannels=" << a->getMatchedChannels()
                  << " loop=" << (loop ? "ON" : "OFF")
                  << std::endl;
    };
    auto switchAnimByKeywordOrIndex = [&](const std::string& keyLower, int fallbackIndex, bool loop) {
        if (!gModel) return;
        int idx = gModel->findAnimationIndexByNameContains(keyLower);
        if (idx == -1) idx = fallbackIndex;
        switchAnim(idx, loop);
    };
    auto switchAnimByKeywordsOrIndex = [&](const std::vector<std::string>& keysLower, int fallbackIndex, bool loop) {
        if (!gModel) return;
        int idx = -1;
        for (const auto& keyLower : keysLower) {
            idx = gModel->findAnimationIndexByNameContains(keyLower);
            if (idx != -1) break;
        }
        if (idx == -1) idx = fallbackIndex;
        switchAnim(idx, loop);
    };

    // 2: walk(loop), 3: run(loop), 4: crawl/prone(loop), 5: sit(loop)
    if (now2 && !prev2) switchAnimByKeywordOrIndex("walk", 2, true);
    if (now3 && !prev3) switchAnimByKeywordOrIndex("run", 3, true);
    if (now4 && !prev4) { switchAnimByKeywordsOrIndex({"crawl", "prone", "creep"}, 4, true); gActionAnimIndex = -1; }
    // key 5: back to model's default/idle animation (loop)
    if (now5 && !prev5) {
        switchAnim(gIdleAnimIndex, true);
        gActionAnimIndex = -1;
    }

   
 

    // Browse all animations: '[' prev, ']' next
    if (gModel && gAnimator) {
        const int count = gModel->getAnimationCount();
        if (count > 0) {
            if (nowRB && !prevRB) {
                gBrowseAnimIndex = (gBrowseAnimIndex + 1) % count;
                switchAnim(gBrowseAnimIndex, true);
                gActionAnimIndex = -1;
            }
            if (nowLB && !prevLB) {
                gBrowseAnimIndex = (gBrowseAnimIndex - 1 + count) % count;
                switchAnim(gBrowseAnimIndex, true);
                gActionAnimIndex = -1;
            }
        }
    }
    prev2 = now2;
    prev3 = now3;
    prev4 = now4;
    prev5 = now5;
    prevLB = nowLB;
    prevRB = nowRB;
    prevP = nowP;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE ) == GLFW_PRESS)
        camera.ProcessKeyboard(UPWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
}
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    
    lastX = xpos;
    lastY = ypos;
    
    camera.ProcessMouseMovement(xoffset, yoffset);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
