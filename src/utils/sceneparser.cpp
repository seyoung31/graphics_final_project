#include "sceneparser.h"
#include "scenefilereader.h"
#include <glm/gtx/transform.hpp>

#include <chrono>
#include <iostream>

bool SceneParser::parse(std::string filepath, RenderData &renderData) {
    ScenefileReader fileReader = ScenefileReader(filepath);
    bool success = fileReader.readJSON();
    if (!success) {
        return false;
    }

    // TODO: Use your Lab 5 code here

    // Task 5: populate renderData with global data, and camera data;
    renderData.globalData = fileReader.getGlobalData();
    renderData.cameraData = fileReader.getCameraData();

    // Task 6: populate renderData's list of primitives and their transforms.
    //         This will involve traversing the scene graph, and we recommend you
    //         create a helper function to do so!
    SceneNode* rootNode = fileReader.getRootNode();
    renderData.shapes.clear();

    glm::mat4 ctm(1.0f);
    traverseTree(*rootNode, ctm, renderData);
    //traverse the tree in a depth-first manner, starting from the root node of the scene graph

    return true;
}

void SceneParser::traverseTree(SceneNode& node, glm::mat4& parentCTM, RenderData& renderData){

    glm::mat4 currCTM = parentCTM * applyTransformations(node.transformations);
    for (ScenePrimitive* nodePrimitive: node.primitives){//for each primitive
        //construct render shape w primitive and corresponding CTM
        RenderShapeData renderShapeData;
        renderShapeData.primitive = *nodePrimitive;
        renderShapeData.ctm = currCTM;
        //append RenderShapeData object onto renderData.shapes
        renderData.shapes.push_back(renderShapeData);
    }
    for (SceneLight* nodeLight: node.lights){
        SceneLightData sceneLightData;
        sceneLightData.id = nodeLight->id;
        sceneLightData.type = nodeLight->type;

        sceneLightData.color = nodeLight->color;
        sceneLightData.function = nodeLight->function;

        glm::vec4 localPos(0,0,0,1); //origin
        glm::vec4 localDir = nodeLight->dir;
        localDir.w = 0;

        switch(nodeLight->type){
        case LightType::LIGHT_DIRECTIONAL:
            sceneLightData.dir = currCTM * localDir;
            break;
        case LightType::LIGHT_POINT:
            sceneLightData.pos = currCTM * localPos;
            break;
        case LightType::LIGHT_SPOT:
            sceneLightData.dir = currCTM * localDir;
            sceneLightData.pos = currCTM * localPos;
            sceneLightData.penumbra = nodeLight->penumbra;
            sceneLightData.angle = nodeLight->angle;
            break;
        }

        sceneLightData.width = nodeLight->width;
        sceneLightData.height = nodeLight->height; //unnecessary?

        renderData.lights.push_back(sceneLightData);
    }
    //for each child recurse
    for (SceneNode* child: node.children){
        traverseTree(*child, currCTM, renderData);
    }

}

glm::mat4 SceneParser::applyTransformations(std::vector<SceneTransformation*> transformations){
    glm::mat4 ctm(1.0f);
    for (SceneTransformation* sceneTransformation: transformations){
        switch(sceneTransformation->type){
        case TransformationType::TRANSFORMATION_TRANSLATE:
            ctm = glm::translate(ctm, sceneTransformation->translate);
            break;
        case TransformationType::TRANSFORMATION_SCALE:
            ctm = glm::scale(ctm, sceneTransformation->scale);
            break;
        case TransformationType::TRANSFORMATION_ROTATE:
            ctm = glm::rotate(ctm, sceneTransformation->angle, sceneTransformation->rotate);
            break;
        case TransformationType::TRANSFORMATION_MATRIX:
            ctm = ctm * sceneTransformation->matrix;
            break;
        default:
            break;
        }
    }
    return ctm;
}
