#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>

struct Settings {
    std::string sceneFilePath;
    int shapeParameter1 = 1;
    int shapeParameter2 = 1;
    float nearPlane = 1;
    float farPlane = 1;
    float focusPlane = 1;
    float moveSpeed = 1;
    bool perPixelFilter = false;
    bool kernelBasedFilter = false;
    bool shadowMapping = false;
    bool screenSpaceDOF = false;
    bool normalMapping = false;
    bool bumpMapping = false;
    bool extraCredit3 = false;
    bool extraCredit4 = false;
    bool cameraPath = false;
    bool colorGrading = false;
    bool watercolor = false;
    bool pixelated = false;
    bool isNight = false;
    float pixelSize = 12.0;
};


// The global Settings object, will be initialized by MainWindow
extern Settings settings;

#endif // SETTINGS_H
