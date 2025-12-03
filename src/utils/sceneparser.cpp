#include "sceneparser.h"
#include "scenefilereader.h"
#include <glm/gtx/transform.hpp>

glm::mat4 applyTransformation(const SceneTransformation *t) {
    glm::mat4 M(1.f);

    switch(t->type) {
    case TransformationType::TRANSFORMATION_TRANSLATE:
        M = glm::translate(glm::mat4(1.f), t->translate);
        break;
    case TransformationType::TRANSFORMATION_SCALE:
        M = glm::scale(glm::mat4(1.f), t->scale);
        break;
    case TransformationType::TRANSFORMATION_ROTATE:
        M = glm::rotate(glm::mat4(1.f), t->angle, glm::normalize(t->rotate));
        break;
    case TransformationType::TRANSFORMATION_MATRIX:
        M = t->matrix;
        break;
    }

    return M;

}

SceneLightData convertToWorldLight(const SceneLight& L, const glm::mat4& CTM) {
    SceneLightData light{};
    light.id = L.id;
    light.type = L.type;
    light.color = L.color;
    light.function = L.function;
    light.penumbra = 0.f;
    light.angle = 0.f;

    const glm::vec3 pos = glm::vec3(CTM * glm::vec4{0.f, 0.f, 0.f, 1.f});
    const glm::vec3 dir = glm::normalize(glm::mat3(CTM) * glm::vec3(L.dir));

    switch (L.type) {
    case LightType::LIGHT_POINT:
        light.pos = glm::vec4(pos, 1.f);
        light.dir = glm::vec4(0,0,0,0);
        break;
    case LightType::LIGHT_DIRECTIONAL:
        light.pos = glm::vec4(0,0,0,0);
        light.dir = glm::vec4(dir, 0.f);
        break;
    case LightType::LIGHT_SPOT:
        light.pos = glm::vec4(pos, 1.f);
        light.dir = glm::vec4(dir, 0.f);
        light.penumbra = L.penumbra;
        light.angle = L.angle;
        break;
    }

    return light;
}

void dfsHelper(const SceneNode *currNode, const glm::mat4 &parentCTM, RenderData &output) {
    if (!currNode) return;

    // build local transformation
    glm::mat4 M_local(1.f);

    for (const SceneTransformation *t : currNode->transformations) {
        M_local = M_local * applyTransformation(t);
    }

    glm::mat4 localCTM = parentCTM * M_local;

    // add shapes to RenderData.shapes
    for (const ScenePrimitive *p : currNode->primitives) {
        RenderShapeData currShape;
        currShape.primitive = *p;
        currShape.ctm = localCTM;
        output.shapes.push_back(currShape);
    }

    // add lights to RenderData.lights
    for (const SceneLight *l : currNode->lights) {
        output.lights.push_back(convertToWorldLight(*l, localCTM));

    }

    for (const SceneNode *child : currNode->children) {
        dfsHelper(child, localCTM, output);
    }
}

bool SceneParser::parse(std::string filepath, RenderData &renderData) {
    ScenefileReader fileReader = ScenefileReader(filepath);
    bool success = fileReader.readJSON();
    if (!success) {
        return false;
    }

    SceneGlobalData globalData = fileReader.getGlobalData();
    SceneCameraData cameraData = fileReader.getCameraData();

    renderData.globalData = globalData;
    renderData.cameraData = cameraData;

    renderData.shapes.clear();
    renderData.lights.clear();

    SceneNode* root = fileReader.getRootNode();

    if (!root) return false;

    dfsHelper(root, glm::mat4(1.f), renderData);
    return true;
}
