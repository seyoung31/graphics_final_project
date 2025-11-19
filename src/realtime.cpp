#include "realtime.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QKeyEvent>
#include <iostream>
#include "settings.h"
#include "src/shaderloader.h"
#include "utils/sceneparser.h"
#include "shapes/Sphere.h"
#include "camera/camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp> // for glm::perspective, glm::lookAt TODO REMOVE DELETE
#include "glm/gtc/constants.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include "shapes/Cube.h"
// #include "shader.h"
// ================== Rendering the Scene!

Realtime::Realtime(QWidget *parent)
    : QOpenGLWidget(parent), m_camera()
{
    m_prev_mouse_pos = glm::vec2(size().width()/2, size().height()/2);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    m_keyMap[Qt::Key_W]       = false;
    m_keyMap[Qt::Key_A]       = false;
    m_keyMap[Qt::Key_S]       = false;
    m_keyMap[Qt::Key_D]       = false;
    m_keyMap[Qt::Key_Control] = false;
    m_keyMap[Qt::Key_Space]   = false;

    // If you must use this function, do not edit anything above this

}

void Realtime::finish() {
    killTimer(m_timer);
    this->makeCurrent();

    // Students: anything requiring OpenGL calls when the program exits should be done here
    glDeleteBuffers(1, &m_vbo);
    glDeleteVertexArrays(1, &m_vao);
    glDeleteProgram(m_shader);
    doneCurrent();
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glDeleteBuffers(1, &m_sphere.vbo);
    glDeleteVertexArrays(1, & m_sphere.vao);
    glDeleteBuffers(1, &m_cube.vbo);
    glDeleteVertexArrays(1, & m_cube.vao);
    glDeleteBuffers(1, &m_cylinder.vbo);
    glDeleteVertexArrays(1, & m_cylinder.vao);
    glDeleteBuffers(1, &m_cone.vbo);
    glDeleteVertexArrays(1, & m_cone.vao);
    if (m_shader != 0u) {
        glDeleteProgram(m_shader);
        m_shader = 0u;
    }
    glFinish();


    this->doneCurrent();
}

void Realtime::initializeGL() {
    m_devicePixelRatio = this->devicePixelRatio();

    m_timer = startTimer(1000/60);
    m_elapsedTimer.start();

    // Initializing GL.
    // GLEW (GL Extension Wrangler) provides access to OpenGL functions.
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        std::cerr << "Error while initializing GL: " << glewGetErrorString(err) << std::endl;
    }
    std::cout << "Initialized GL: Version " << glewGetString(GLEW_VERSION) << std::endl;

    // Allows OpenGL to draw objects appropriately on top of one another
    glEnable(GL_DEPTH_TEST);
    // Tells OpenGL to only draw the front face
    glEnable(GL_CULL_FACE);
    // Tells OpenGL how big the screen is
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);

    // Students: anything requiring OpenGL calls when the program starts should be done here

    m_camera.updateCameraData(scene.cameraData);
    m_view = m_camera.getViewMatrix();

    // m_proj = glm::perspective(glm::radians(45.0),1.0 * width() / height(),0.01,100.0);
    // m_proj = camera.calcProjectionMatrix(glm::radians(45.0),1.0 * width() / height(),0.01,100.0);
    m_proj = m_camera.calcProjectionMatrix(m_camera.getHeightAngle(), m_camera.getAspectRatio(width(), height()), settings.nearPlane, settings.farPlane);

    m_shader = ShaderLoader::createShaderProgram(":/resources/shaders/default.vert",":/resources/shaders/default.frag");

    std::cout<<"gets here"<<std::endl;
    Particles particle;
    particle.print();
    std::cout<<"and here"<<std::endl;

    Sphere sphere;
    sphere.updateParams(settings.shapeParameter1, settings.shapeParameter2);
    m_sphere.vertex_data = sphere.generateShape();
    initGLShape(m_sphere, m_sphere.vertex_data);

    Cube cube;
    cube.updateParams(settings.shapeParameter1);
    m_cube.vertex_data = cube.generateShape();
    initGLShape(m_cube, m_cube.vertex_data);

    Cone cone;
    cone.updateParams(settings.shapeParameter1, settings.shapeParameter2);
    m_cone.vertex_data = cone.generateShape();
    initGLShape(m_cone, m_cone.vertex_data);

    Cylinder cylinder;
    cylinder.updateParams(settings.shapeParameter1, settings.shapeParameter2);
    m_cylinder.vertex_data = cylinder.generateShape();
    initGLShape(m_cylinder, m_cylinder.vertex_data);




}

void Realtime::initGLShape(GLData &shape, std::vector<float> &data)
{
    // Generate and bind VBO
    glGenBuffers(1, &shape.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, shape.vbo);
    // Generate sphere data

    // Send data to VBO
    glBufferData(GL_ARRAY_BUFFER,shape.vertex_data.size() * sizeof(GLfloat),shape.vertex_data.data(), GL_STATIC_DRAW);
    // Generate, and bind vao
    glGenVertexArrays(1, &shape.vao);
    glBindVertexArray(shape.vao);

    // Enable and define attribute 0 to store vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6 * sizeof(GLfloat),reinterpret_cast<void *>(0));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6 * sizeof(GLfloat),reinterpret_cast<void *>(3 * sizeof(GLfloat)));

    // Clean-up bindings
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER,0);
}

void Realtime::paintGL() {
    // Students: anything requiring OpenGL calls every frame should be done here


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(m_shader);

    //camera passing
    glm::vec3 world_cam = m_camera.getCameraPosition();
    GLint world_cam_loc = glGetUniformLocation(m_shader, ("world_cam"));
    glUniform3f(world_cam_loc, world_cam.x, world_cam.y, world_cam.z);


    //handle phong passing
    Shader shader;
    shader.handleLighting(m_shader, scene, lights);
    shader.handleGlobals(m_shader, scene);
    shader.handleShapes(m_shader, m_camera, myPrimitives, m_proj, m_sphere, m_cone, m_cube, m_cylinder);

    // Unbind Vertex Array
    glBindVertexArray(0);

    // Task 3: deactivate the shader program by passing 0 into glUseProgram
    glUseProgram(0);

}

void Realtime::resizeGL(int w, int h) {
    // Tells OpenGL how big the screen is
    glViewport(0, 0, size().width() * m_devicePixelRatio, size().height() * m_devicePixelRatio);
    // Students: anything requiring OpenGL calls when the program starts should be done here
    // m_proj = glm::perspective(glm::radians(45.0),1.0 * 800 / 600,0.01,100.0);
    m_proj = m_camera.calcProjectionMatrix(glm::radians(45.0),1.0 * 800 / 600,0.01,100.0);


}

void Realtime::sceneChanged() {


    myPrimitives.clear();

    for (int i = 0; i < 8; i++) {
        lights[i] = {};       // C++ zero-initialization
    }

    RenderData data;
    scene = data;


    m_cube.vertex_data.clear();

    //parse the scene
    SceneParser parser;
    bool parsed;
    if (!parser.parse(settings.sceneFilePath, scene)){ //good ole error check
        std::cerr<<"Scene not parsed"<<std::endl;
    }
    m_camera.updateCameraData(scene.cameraData);
    for (RenderShapeData &shape: scene.shapes){
        m_camera_pos = m_camera.getCameraPosition();
        m_view = m_camera.getViewMatrix();
        m_proj = m_camera.calcProjectionMatrix(m_camera.getHeightAngle(), m_camera.getAspectRatio(width(), height()), settings.nearPlane, settings.farPlane);

        auto glShape = std::make_unique<GLShape>();
        glShape->primType = shape.primitive.type;
        glShape->model = glm::mat4(1.0f);
        glShape->model = shape.ctm;
        glShape->material = shape.primitive.material;
        switch(shape.primitive.type){
        case PrimitiveType::PRIMITIVE_SPHERE:{
            Sphere sphere;
            sphere.updateParams(settings.shapeParameter1, settings.shapeParameter2);  // Update FIRST
            glShape->sphere = sphere;  // Then assign

            if (m_sphere.vbo != 0){
                glBindBuffer(GL_ARRAY_BUFFER, m_sphere.vbo);
                m_sphere.vertex_data = sphere.generateShape();  // Use the updated sphere
                glBufferData(GL_ARRAY_BUFFER,m_sphere.vertex_data.size() * sizeof(GLfloat),m_sphere.vertex_data.data(), GL_STATIC_DRAW);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
            break;
            break;

        }
        case PrimitiveType::PRIMITIVE_CONE:{
            Cone cone;
            glShape->cone = cone;

            if (m_cone.vbo != 0){
                glBindBuffer(GL_ARRAY_BUFFER, m_cone.vbo);
                glShape->cone.updateParams(settings.shapeParameter1, settings.shapeParameter2);
                m_cone.vertex_data = glShape->cone.generateShape();
                glBufferData(GL_ARRAY_BUFFER,m_cone.vertex_data.size() * sizeof(GLfloat),m_cone.vertex_data.data(), GL_STATIC_DRAW);

                // Unbind the VBO
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
            break;



        }
        case PrimitiveType::PRIMITIVE_CUBE:{

            m_cube.vertex_data.clear();

            Cube cube;
            glShape->cube = cube;

            if (m_cube.vbo != 0){
                glBindBuffer(GL_ARRAY_BUFFER, m_cube.vbo);
                glShape->cube.updateParams(settings.shapeParameter1);
                m_cube.vertex_data = glShape->cube.generateShape();
                glBufferData(GL_ARRAY_BUFFER,m_cube.vertex_data.size() * sizeof(GLfloat),m_cube.vertex_data.data(), GL_STATIC_DRAW);

                // Unbind the VBO
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
            break;
        }
        case PrimitiveType::PRIMITIVE_CYLINDER:{
            Cylinder cylinder;
            glShape->cylinder = cylinder;

            if (m_cylinder.vbo != 0){
                glBindBuffer(GL_ARRAY_BUFFER, m_cylinder.vbo);
                glShape->cylinder.updateParams(settings.shapeParameter1, settings.shapeParameter2);
                m_cylinder.vertex_data = glShape->cylinder.generateShape();
                glBufferData(GL_ARRAY_BUFFER,m_cylinder.vertex_data.size() * sizeof(GLfloat),m_cylinder.vertex_data.data(), GL_STATIC_DRAW);

                // Unbind the VBO
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
            break;
        }
        case PrimitiveType::PRIMITIVE_MESH:
            break;
        default:
            break;
        }
        myPrimitives.push_back(std::move(glShape));
    }


    for (int i = 0; i < scene.lights.size(); i++){
        SceneLightData &lightData = scene.lights[i];
        if (lightData.type == LightType::LIGHT_DIRECTIONAL) lights[i].type = 0;
        if (lightData.type == LightType::LIGHT_POINT) lights[i].type = 1;
        if (lightData.type == LightType::LIGHT_SPOT) lights[i].type = 2;

        lights[i].color = lightData.color;
        lights[i].pos = lightData.pos;
        lights[i].dir = lightData.dir;
        lights[i].function = lightData.function;
        lights[i].angle = lightData.angle;
        lights[i].penumbra = lightData.penumbra;
    }
    update(); // asks for a PaintGL() call to occur

    //call when view matrix changes because camera movement occurs
}


void Realtime::settingsChanged() {

    update(); // asks for a PaintGL() call to occur
    //call updateParams for each shape
    //call updates for near and far plane changing


    //can be helper to update vertex data
    for (std::unique_ptr<GLShape> &shape: myPrimitives){
        // shape->vertex_data.clear();
        switch(shape->primType){
        case PrimitiveType::PRIMITIVE_SPHERE:{
            m_sphere.vertex_data.clear();
            shape->sphere.updateParams(settings.shapeParameter1, settings.shapeParameter2);
            m_sphere.vertex_data = shape->sphere.generateShape();
            break;
        }
        break;
        case PrimitiveType::PRIMITIVE_CONE:{
            m_cone.vertex_data.clear();
            shape->cone.updateParams(settings.shapeParameter1, settings.shapeParameter2);
            m_cone.vertex_data = shape->cone.generateShape();
            break;
        }
        case PrimitiveType::PRIMITIVE_CUBE:{
            m_cube.vertex_data.clear();
            shape->cube.updateParams(settings.shapeParameter1);
            m_cube.vertex_data = shape->cube.generateShape();
            break;
        }
        case PrimitiveType::PRIMITIVE_CYLINDER:{
            m_cylinder.vertex_data.clear();
            shape->cylinder.updateParams(settings.shapeParameter1, settings.shapeParameter2);
            m_cylinder.vertex_data = shape->cylinder.generateShape();
            break;
        }
        }
    }

    for (std::unique_ptr<GLShape> &shape: myPrimitives){
        GLuint vboToBind;
        std::vector<float> *vertex;
        switch (shape->primType) {
        case PrimitiveType::PRIMITIVE_SPHERE:   vboToBind = m_sphere.vbo;   vertex = &m_sphere.vertex_data; break;
        case PrimitiveType::PRIMITIVE_CUBE:     vboToBind = m_cube.vbo;     vertex = &m_cube.vertex_data;break;
        case PrimitiveType::PRIMITIVE_CONE:     vboToBind = m_cone.vbo;     vertex = &m_cone.vertex_data; break;
        case PrimitiveType::PRIMITIVE_CYLINDER: vboToBind = m_cylinder.vbo; vertex = &m_cylinder.vertex_data;break;
        default: continue;
        }
        if (vboToBind != 0){
            glBindBuffer(GL_ARRAY_BUFFER, vboToBind);
            glBufferData(GL_ARRAY_BUFFER,vertex->size() * sizeof(GLfloat),vertex->data(), GL_STATIC_DRAW);

            // Unbind the VBO
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }
    m_proj = m_camera.calcProjectionMatrix(m_camera.getHeightAngle(), m_camera.getAspectRatio(width(), height()), settings.nearPlane, settings.farPlane);
}

// ================== Camera Movement!



void Realtime::keyPressEvent(QKeyEvent *event) {
    m_keyMap[Qt::Key(event->key())] = true;
}

void Realtime::keyReleaseEvent(QKeyEvent *event) {
    m_keyMap[Qt::Key(event->key())] = false;
}

void Realtime::mousePressEvent(QMouseEvent *event) {
    if (event->buttons().testFlag(Qt::LeftButton)) {
        m_mouseDown = true;
        m_prev_mouse_pos = glm::vec2(event->position().x(), event->position().y());
    }
}

void Realtime::mouseReleaseEvent(QMouseEvent *event) {
    if (!event->buttons().testFlag(Qt::LeftButton)) {
        m_mouseDown = false;
    }
}

void Realtime::mouseMoveEvent(QMouseEvent *event) {
    if (m_mouseDown) {
        int posX = event->position().x();
        int posY = event->position().y();
        int deltaX = posX - m_prev_mouse_pos.x;
        int deltaY = posY - m_prev_mouse_pos.y;
        m_prev_mouse_pos = glm::vec2(posX, posY);

        // Use deltaX and deltaY here to rotate
        m_camera.processMouseMove((float)deltaX, (float)deltaY, m_mouseDown);


        update(); // asks for a PaintGL() call to occur
    }
}

void Realtime::timerEvent(QTimerEvent *event) {
    int elapsedms   = m_elapsedTimer.elapsed();
    float deltaTime = elapsedms * 0.001f;
    m_elapsedTimer.restart();

    // Use deltaTime and m_keyMap here to move around

    bool w = m_keyMap[Qt::Key_W];
    bool a = m_keyMap[Qt::Key_A];
    bool s = m_keyMap[Qt::Key_S];
    bool d = m_keyMap[Qt::Key_D];
    bool ctrl = m_keyMap[Qt::Key_Control];
    bool space = m_keyMap[Qt::Key_Space];

    // Update camera translation
    m_camera.processKeyboard(w, s, a, d, space, ctrl, deltaTime);

    update(); // asks for a PaintGL() call to occur
}

// DO NOT EDIT
void Realtime::saveViewportImage(std::string filePath) {
    // Make sure we have the right context and everything has been drawn
    makeCurrent();

    int fixedWidth = 1024;
    int fixedHeight = 768;

    // Create Frame Buffer
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // Create a color attachment texture
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fixedWidth, fixedHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    // Optional: Create a depth buffer if your rendering uses depth testing
    GLuint rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, fixedWidth, fixedHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Error: Framebuffer is not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return;
    }

    // Render to the FBO
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, fixedWidth, fixedHeight);

    // Clear and render your scene here
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    paintGL();

    // Read pixels from framebuffer
    std::vector<unsigned char> pixels(fixedWidth * fixedHeight * 3);
    glReadPixels(0, 0, fixedWidth, fixedHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // Unbind the framebuffer to return to default rendering to the screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Convert to QImage
    QImage image(pixels.data(), fixedWidth, fixedHeight, QImage::Format_RGB888);
    QImage flippedImage = image.mirrored(); // Flip the image vertically

    // Save to file using Qt
    QString qFilePath = QString::fromStdString(filePath);
    if (!flippedImage.save(qFilePath)) {
        std::cerr << "Failed to save image to " << filePath << std::endl;
    }

    // Clean up
    glDeleteTextures(1, &texture);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteFramebuffers(1, &fbo);
}

