#ifndef SHADER_H
#define SHADER_H
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "utils/scenedata.h"
#include "utils/sceneparser.h"
#include <QElapsedTimer>

struct RenderData;
struct GLight;
class Camera;
struct GLShape;
struct GLData;

class Shader
{
public:
    Shader();
    void handleLighting(GLuint &m_shader, RenderData &scene, GLight lights[8]);
    void handleGlobals(GLuint &m_shader, RenderData &scene);
    void handleShapes(GLuint &m_shader, Camera &m_camera, std::vector<std::unique_ptr<GLShape>> &myPrimitives, glm::mat4 &m_proj, GLData &m_sphere, GLData &m_cone, GLData &m_cube, GLData &m_cylinder );


};

#endif // SHADER_H
