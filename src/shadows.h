#ifndef SHADOWS_H
#define SHADOWS_H
#include "glm/glm.hpp"
#include "utils/sceneparser.h"


class shadows
{
public:
    shadows();
    glm::mat4 getLightVP(const SceneLightData& light);
    void makeShadowFBO();
    void paintLightView(SceneLightData light, const glm::mat4& lightVP);
};

#endif // SHADOWS_H
