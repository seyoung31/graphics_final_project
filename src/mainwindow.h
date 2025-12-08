#pragma once

#include <QMainWindow>
#include <QCheckBox>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include "realtime.h"
#include "utils/aspectratiowidget/aspectratiowidget.hpp"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    void initialize();
    void finish();

private:
    void connectUIElements();
    void connectParam1();
    void connectParam2();
    void connectNear();
    void connectFar();
    void connectFocus();
    void connectSpeed();
    void connectPixelSize();

    // From old Project 6
    // void connectPerPixelFilter();
    // void connectKernelBasedFilter();

    void connectUploadFile();
    void connectSaveImage();
    void connectExtraCredit();

    Realtime *realtime;
    AspectRatioWidget *aspectRatioWidget;

    // From old Project 6
    // QCheckBox *filter1;
    // QCheckBox *filter2;

    QPushButton *uploadFile;
    QPushButton *saveImage;
    QSlider *p1Slider;
    QSlider *p2Slider;
    QSpinBox *p1Box;
    QSpinBox *p2Box;
    QSlider *nearSlider;
    QSlider *farSlider;
    QSlider *focusSlider;    //new
    QSlider *speedSlider;    //new
    QSlider *pixelSizeSlider; //new
    QDoubleSpinBox *nearBox;
    QDoubleSpinBox *farBox;
    QDoubleSpinBox *focusBox; //new
    QDoubleSpinBox *speedBox; //new
    QDoubleSpinBox *pixelSizeBox; //new

    // Extra Credit:
    QCheckBox *ec1;
    QCheckBox *ec2;
    QCheckBox *ec3;
    QCheckBox *ec4;
    QCheckBox *cameraPathCB;
    QCheckBox *colorGradeCB;
    QCheckBox *watercolorCB;
    QCheckBox *pixelatedCB;

private slots:
    // From old Project 6
    // void onPerPixelFilter();
    // void onKernelBasedFilter();

    void onUploadFile();
    void onSaveImage();
    void onValChangeP1(int newValue);
    void onValChangeP2(int newValue);
    void onValChangeNearSlider(int newValue);
    void onValChangeFarSlider(int newValue);
    void onValChangeFocusSlider(int newValue);//new
    void onValChangeSpeedSlider(int newValue);//new
    void onValChangeNearBox(double newValue);
    void onValChangeFarBox(double newValue);
    void onValChangeFocusBox(double newValue);//new
    void onValChangeSpeedBox(double newValue);
    void onValChangePixelSizeSlider(int newValue);
    void onValChangePixelSizeBox(double newValue);

    // Extra Credit:
    void onShadowMapping();
    void onScreenSpaceDOF();
    void onExtraCredit3();
    void onExtraCredit4();
    void onCameraPath();
    void onColorGrading();
    void onWatercolor();
    void onPixelated();
};
