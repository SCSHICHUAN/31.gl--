
#ifndef animation_h
#define animation_h

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <vector>
#include <map>
#include <iostream>

using namespace std;

struct BoneInfo {
    int id;
    glm::mat4 offset;
};

struct KeyPosition {
    glm::vec3 position;
    float timeStamp;
};

struct KeyRotation {
    glm::quat orientation;
    float timeStamp;
};

struct KeyScale {
    glm::vec3 scale;
    float timeStamp;
};

class Bone {
private:
    vector<KeyPosition> positions;
    vector<KeyRotation> rotations;
    vector<KeyScale> scales;
    int numPositions;
    int numRotations;
    int numScalings;
    int boneID;
    string boneName;
    glm::mat4 localTransform;

public:
    Bone(const string& name, int ID, const aiNodeAnim* channel);
    void update(float animationTime);
    glm::mat4 getLocalTransform() { return localTransform; }
    string getBoneName() const { return boneName; }
    int getBoneID() { return boneID; }

private:
    int getPositionIndex(float animationTime);
    int getRotationIndex(float animationTime);
    int getScaleIndex(float animationTime);
    float getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime);
    glm::mat4 interpolatePosition(float animationTime);
    glm::mat4 interpolateRotation(float animationTime);
    glm::mat4 interpolateScaling(float animationTime);
};

class Animation {
private:
    float duration;
    int ticksPerSecond;
    vector<Bone> bones;
    map<string, BoneInfo> boneInfoMap;
    int boneCount;
    string animationName;
    string sourceName;
    const aiScene* scene;
    glm::mat4 globalInverseTransform;
    int animationIndex;

public:
    Animation(const string& name, const aiScene* scene, int animationIndex, const map<string, BoneInfo>& boneInfoMap, int boneCount);
    void update(float animationTime);
    Bone* findBone(const string& name);
    map<string, BoneInfo>& getBoneInfoMap();
    int getBoneCount();
    float getDuration();
    int getTicksPerSecond();
    const aiScene* getScene() { return scene; }
    const glm::mat4& getGlobalInverseTransform() const { return globalInverseTransform; }
    int getAnimationIndex() const { return animationIndex; }
    const string& getSourceName() const { return sourceName; }
    int getMatchedChannels() const { return (int)bones.size(); }
};

class Animator {
private:
    Animation* currentAnimation;
    float currentTime;
    float deltaTime;
    map<int, glm::mat4> finalBoneMatrices;
    bool looping;

public:
    Animator(Animation* animation);
    void updateAnimation(float dt);
    void playAnimation(Animation* pAnimation, bool resetTime = true);
    void setLooping(bool enabled) { looping = enabled; }
    bool isLooping() const { return looping; }
    bool isFinished() const;
    void calculateBoneTransform(const aiNode* node, glm::mat4 parentTransform);
    map<int, glm::mat4>& getFinalBoneMatrices();
};

#endif /* animation_h */
