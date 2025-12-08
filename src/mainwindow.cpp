#include "mainwindow.h"
#include "settings.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QSettings>
#include <QLabel>
#include <QGroupBox>
#include <iostream>

void MainWindow::initialize() {
    realtime = new Realtime;
    aspectRatioWidget = new AspectRatioWidget(this);
    aspectRatioWidget->setAspectWidget(realtime, 3.f/4.f);
    QHBoxLayout *hLayout = new QHBoxLayout; // horizontal alignment
    QVBoxLayout *vLayout = new QVBoxLayout(); // vertical alignment
    vLayout->setAlignment(Qt::AlignTop);
    vLayout->setSpacing(5); // Reduce spacing between widgets
    vLayout->setContentsMargins(5, 5, 5, 5); // Reduce margins
    hLayout->addLayout(vLayout);
    hLayout->addWidget(aspectRatioWidget, 1);
    this->setLayout(hLayout);

    // Create labels in sidebox
    QFont font;
    font.setPointSize(12);
    font.setBold(true);
    QLabel *tesselation_label = new QLabel(); // Parameters label
    tesselation_label->setText("Tesselation");
    tesselation_label->setFont(font);
    QLabel *camera_label = new QLabel(); // Camera label
    camera_label->setText("Camera");
    camera_label->setFont(font);

    // From old Project 6
    // QLabel *filters_label = new QLabel(); // Filters label
    // filters_label->setText("Filters");
    // filters_label->setFont(font);

    QLabel *ec_label = new QLabel(); // Extra Credit label
    ec_label->setText("Extra Credit");
    ec_label->setFont(font);
    QLabel *param1_label = new QLabel(); // Parameter 1 label
    param1_label->setText("Parameter 1:");
    QLabel *param2_label = new QLabel(); // Parameter 2 label
    param2_label->setText("Parameter 2:");
    QLabel *near_label = new QLabel(); // Near plane label
    near_label->setText("Near Plane:");
    QLabel *far_label = new QLabel(); // Far plane label
    far_label->setText("Far Plane:");

    //Focus plane
    QLabel *focus_label = new QLabel(); // Focus plane label
    focus_label->setText("Focus Plane:");

    //speed
    QLabel *speed_label = new QLabel(); // speedlabel
    speed_label->setText("Move Speed:");

    //pixel size
    QLabel *pixelSize_label = new QLabel(); // pixel size label
    pixelSize_label->setText("Pixel Size:");

    // From old Project 6
    // // Create checkbox for per-pixel filter
    // filter1 = new QCheckBox();
    // filter1->setText(QStringLiteral("Per-Pixel Filter"));
    // filter1->setChecked(false);
    // // Create checkbox for kernel-based filter
    // filter2 = new QCheckBox();
    // filter2->setText(QStringLiteral("Kernel-Based Filter"));
    // filter2->setChecked(false);

    // Create file uploader for scene file
    uploadFile = new QPushButton();
    uploadFile->setText(QStringLiteral("Upload Scene File"));
    
    saveImage = new QPushButton();
    saveImage->setText(QStringLiteral("Save Image"));

    // Creates the boxes containing the parameter sliders and number boxes
    QGroupBox *p1Layout = new QGroupBox(); // horizonal slider 1 alignment
    QHBoxLayout *l1 = new QHBoxLayout();
    QGroupBox *p2Layout = new QGroupBox(); // horizonal slider 2 alignment
    QHBoxLayout *l2 = new QHBoxLayout();

    // Create slider controls to control parameters
    p1Slider = new QSlider(Qt::Orientation::Horizontal); // Parameter 1 slider
    p1Slider->setTickInterval(1);
    p1Slider->setMinimum(1);
    p1Slider->setMaximum(25);
    p1Slider->setValue(1);

    p1Box = new QSpinBox();
    p1Box->setMinimum(1);
    p1Box->setMaximum(25);
    p1Box->setSingleStep(1);
    p1Box->setValue(1);

    p2Slider = new QSlider(Qt::Orientation::Horizontal); // Parameter 2 slider
    p2Slider->setTickInterval(1);
    p2Slider->setMinimum(1);
    p2Slider->setMaximum(25);
    p2Slider->setValue(1);

    p2Box = new QSpinBox();
    p2Box->setMinimum(1);
    p2Box->setMaximum(25);
    p2Box->setSingleStep(1);
    p2Box->setValue(1);

    // Adds the slider and number box to the parameter layouts
    l1->addWidget(p1Slider);
    l1->addWidget(p1Box);
    p1Layout->setLayout(l1);

    l2->addWidget(p2Slider);
    l2->addWidget(p2Box);
    p2Layout->setLayout(l2);

    // Creates the boxes containing the camera sliders and number boxes
    QGroupBox *nearLayout = new QGroupBox(); // horizonal near slider alignment
    QHBoxLayout *lnear = new QHBoxLayout();
    QGroupBox *farLayout = new QGroupBox(); // horizonal far slider alignment
    QHBoxLayout *lfar = new QHBoxLayout();
    //new for focus
    QGroupBox *focusLayout = new QGroupBox(); // horizonal focus slider alignment
    QHBoxLayout *lfocus = new QHBoxLayout();
    //new for speed
    QGroupBox *speedLayout = new QGroupBox(); // horizonal speed slider alignment
    QHBoxLayout *lspeed = new QHBoxLayout();
    //new for pixel size
    QGroupBox *pixelSizeLayout = new QGroupBox(); // horizonal pixel size slider alignment
    QHBoxLayout *lpixelSize = new QHBoxLayout();

    // Create slider controls to control near/far planes
    nearSlider = new QSlider(Qt::Orientation::Horizontal); // Near plane slider
    nearSlider->setTickInterval(1);
    nearSlider->setMinimum(1);
    nearSlider->setMaximum(1000);
    nearSlider->setValue(10);

    nearBox = new QDoubleSpinBox();
    nearBox->setMinimum(0.01f);
    nearBox->setMaximum(10.f);
    nearBox->setSingleStep(0.1f);
    nearBox->setValue(0.1f);

    farSlider = new QSlider(Qt::Orientation::Horizontal); // Far plane slider
    farSlider->setTickInterval(1);
    farSlider->setMinimum(1000);
    farSlider->setMaximum(10000);
    farSlider->setValue(10000);

    farBox = new QDoubleSpinBox();
    farBox->setMinimum(10.f);
    farBox->setMaximum(100.f);
    farBox->setSingleStep(0.1f);
    farBox->setValue(100.f);

    focusSlider = new QSlider(Qt::Orientation::Horizontal); // focus plane slider
    focusSlider->setTickInterval(1);
    focusSlider->setMinimum(0.01f);
    focusSlider->setMaximum(10000);
    focusSlider->setValue(10000);

    focusBox = new QDoubleSpinBox();
    focusBox->setMinimum(0.01f);
    focusBox->setMaximum(100.f);
    focusBox->setSingleStep(0.1f);
    focusBox->setValue(100.f);

    speedSlider = new QSlider(Qt::Orientation::Horizontal); // speed slider
    speedSlider->setTickInterval(.2f);
    speedSlider->setMinimum(0.1f);
    speedSlider->setMaximum(100.f);
    speedSlider->setValue(1.f);

    speedBox = new QDoubleSpinBox();
    speedBox->setMinimum(0.1f);
    speedBox->setMaximum(10.f);
    speedBox->setSingleStep(0.2f);
    speedBox->setValue(1.f);

    pixelSizeSlider = new QSlider(Qt::Orientation::Horizontal); // pixel size slider
    pixelSizeSlider->setTickInterval(1);
    pixelSizeSlider->setMinimum(1);
    pixelSizeSlider->setMaximum(100);
    pixelSizeSlider->setValue(12);

    pixelSizeBox = new QDoubleSpinBox();
    pixelSizeBox->setMinimum(1.0f);
    pixelSizeBox->setMaximum(100.0f);
    pixelSizeBox->setSingleStep(1.0f);
    pixelSizeBox->setValue(12.0f);

    // Adds the slider and number box to the parameter layouts
    lnear->addWidget(nearSlider);
    lnear->addWidget(nearBox);
    nearLayout->setLayout(lnear);

    lfar->addWidget(farSlider);
    lfar->addWidget(farBox);
    farLayout->setLayout(lfar);

    lfocus->addWidget(focusSlider);
    lfocus->addWidget(focusBox);
    focusLayout->setLayout(lfocus);

    lspeed->addWidget(speedSlider);
    lspeed->addWidget(speedBox);
    speedLayout->setLayout(lspeed);

    lpixelSize->addWidget(pixelSizeSlider);
    lpixelSize->addWidget(pixelSizeBox);
    pixelSizeLayout->setLayout(lpixelSize);

    // Extra Credit:
    ec1 = new QCheckBox();
    ec1->setText(QStringLiteral("Shadow Mapping"));
    ec1->setChecked(false);

    ec2 = new QCheckBox();
    ec2->setText(QStringLiteral("Screen Space DOF"));
    ec2->setChecked(false);

    ec3 = new QCheckBox();
    ec3->setText(QStringLiteral("Normal Mapping"));
    ec3->setChecked(false);

    ec3_bump = new QCheckBox();
    ec3_bump->setText(QStringLiteral("Bump Mapping"));
    ec3_bump->setChecked(false);

    ec4 = new QCheckBox();
    ec4->setText(QStringLiteral("Let it snow!!!"));
    ec4->setChecked(false);

    cameraPathCB = new QCheckBox();
    cameraPathCB->setText(QStringLiteral("Camera Path"));
    cameraPathCB->setChecked(false);

    colorGradeCB = new QCheckBox();
    colorGradeCB->setText(QStringLiteral("Color Grading"));
    colorGradeCB->setChecked(false);

    watercolorCB = new QCheckBox();
    watercolorCB->setText(QStringLiteral("Line Art Style"));
    watercolorCB->setChecked(false);

    pixelatedCB = new QCheckBox();
    pixelatedCB->setText(QStringLiteral("Pixelated Style"));
    pixelatedCB->setChecked(false);


    vLayout->addWidget(uploadFile);
    vLayout->addWidget(saveImage);
    vLayout->addWidget(tesselation_label);
    vLayout->addWidget(param1_label);
    vLayout->addWidget(p1Layout);
    vLayout->addWidget(param2_label);
    vLayout->addWidget(p2Layout);
    vLayout->addWidget(camera_label);
    vLayout->addWidget(near_label);
    vLayout->addWidget(nearLayout);
    vLayout->addWidget(far_label);
    vLayout->addWidget(farLayout);
    //focus
    vLayout->addWidget(focus_label);
    vLayout->addWidget(focusLayout);
    //speed
    vLayout->addWidget(speed_label);
    vLayout->addWidget(speedLayout);


    vLayout->addWidget(pixelSize_label);
    vLayout->addWidget(pixelSizeLayout);

    // From old Project 6
    // vLayout->addWidget(filters_label);
    // vLayout->addWidget(filter1);
    // vLayout->addWidget(filter2);

    // Extra Credit:
    // Extra Credit:
    vLayout->addWidget(ec_label);

    //CHANGED THESE TO BE SIDE BY SIDE
    QWidget *ecWidget = new QWidget();
    QGridLayout *ecLayout = new QGridLayout();
    ecLayout->setSpacing(10); // optional, spacing between checkboxes
    //below line creates more space in side by side
    ecLayout->setColumnMinimumWidth(0, 150); // forces left column to be at least 150px wide

    // Add checkboxes to the grid layout in 2 columns
    ecLayout->addWidget(ec1, 0, 0);
    ecLayout->addWidget(ec2, 1, 0);
    ecLayout->addWidget(ec3, 2, 0);
    ecLayout->addWidget(ec3_bump, 3, 0);
    ecLayout->addWidget(ec4, 4, 0);

    ecLayout->addWidget(cameraPathCB, 0, 1);
    ecLayout->addWidget(colorGradeCB, 1, 1);
    ecLayout->addWidget(watercolorCB, 2, 1);
    ecLayout->addWidget(pixelatedCB, 3, 1);

    ecWidget->setLayout(ecLayout);
    vLayout->addWidget(ecWidget);


    connectUIElements();

    // Set default values of 5 for tesselation parameters
    onValChangeP1(5);
    onValChangeP2(5);

    // Set default values for near and far planes
    onValChangeNearBox(0.1f);
    onValChangeFarBox(100.f);
    //default focus
    onValChangeFocusBox(5.f);
    //defaul Speed
    onValChangeSpeedBox(1.f);
    //default Pixel Size
    onValChangePixelSizeBox(3.0f);
}

void MainWindow::finish() {
    realtime->finish();
    delete(realtime);
}

void MainWindow::connectUIElements() {
    // From old Project 6
    //connectPerPixelFilter();
    //connectKernelBasedFilter();
    connectUploadFile();
    connectSaveImage();
    connectParam1();
    connectParam2();
    connectNear();
    connectFar();
    connectFocus();
    connectSpeed();
    connectPixelSize();
    connectExtraCredit();
}


// From old Project 6
// void MainWindow::connectPerPixelFilter() {
//     connect(filter1, &QCheckBox::clicked, this, &MainWindow::onPerPixelFilter);
// }
// void MainWindow::connectKernelBasedFilter() {
//     connect(filter2, &QCheckBox::clicked, this, &MainWindow::onKernelBasedFilter);
// }

void MainWindow::connectUploadFile() {
    connect(uploadFile, &QPushButton::clicked, this, &MainWindow::onUploadFile);
}

void MainWindow::connectSaveImage() {
    connect(saveImage, &QPushButton::clicked, this, &MainWindow::onSaveImage);
}

void MainWindow::connectParam1() {
    connect(p1Slider, &QSlider::valueChanged, this, &MainWindow::onValChangeP1);
    connect(p1Box, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MainWindow::onValChangeP1);
}

void MainWindow::connectParam2() {
    connect(p2Slider, &QSlider::valueChanged, this, &MainWindow::onValChangeP2);
    connect(p2Box, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &MainWindow::onValChangeP2);
}

void MainWindow::connectNear() {
    connect(nearSlider, &QSlider::valueChanged, this, &MainWindow::onValChangeNearSlider);
    connect(nearBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onValChangeNearBox);
}

void MainWindow::connectFar() {
    connect(farSlider, &QSlider::valueChanged, this, &MainWindow::onValChangeFarSlider);
    connect(farBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onValChangeFarBox);
}

void MainWindow::connectFocus() {
    connect(focusSlider, &QSlider::valueChanged, this, &MainWindow::onValChangeFocusSlider);
    connect(focusBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onValChangeFocusBox);
}

void MainWindow::connectSpeed() {
    connect(speedSlider, &QSlider::valueChanged, this, &MainWindow::onValChangeSpeedSlider);
    connect(speedBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onValChangeSpeedBox);
}

void MainWindow::connectPixelSize() {
    connect(pixelSizeSlider, &QSlider::valueChanged, this, &MainWindow::onValChangePixelSizeSlider);
    connect(pixelSizeBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onValChangePixelSizeBox);
}

void MainWindow::connectExtraCredit() {
    connect(ec1, &QCheckBox::clicked, this, &MainWindow::onShadowMapping);
    connect(ec2, &QCheckBox::clicked, this, &MainWindow::onScreenSpaceDOF);
    connect(ec3, &QCheckBox::clicked, this, &MainWindow::onExtraCredit3);
    connect(ec3_bump, &QCheckBox::clicked, this, &MainWindow::onBumpMapping);
    connect(ec4, &QCheckBox::clicked, this, &MainWindow::onExtraCredit4);
    connect(cameraPathCB, &QCheckBox::clicked, this, &MainWindow::onCameraPath);
    connect(colorGradeCB, &QCheckBox::clicked, this, &MainWindow::onColorGrading);
    connect(watercolorCB, &QCheckBox::clicked, this, &MainWindow::onWatercolor);
    connect(pixelatedCB, &QCheckBox::clicked, this, &MainWindow::onPixelated);

}

// From old Project 6
// void MainWindow::onPerPixelFilter() {
//     settings.perPixelFilter = !settings.perPixelFilter;
//     realtime->settingsChanged();
// }
// void MainWindow::onKernelBasedFilter() {
//     settings.kernelBasedFilter = !settings.kernelBasedFilter;
//     realtime->settingsChanged();
// }

void MainWindow::onUploadFile() {
    // Get abs path of scene file
    QString configFilePath = QFileDialog::getOpenFileName(this, tr("Upload File"),
                                                          QDir::currentPath()
                                                              .append(QDir::separator())
                                                              .append("scenefiles")
                                                              .append(QDir::separator())
                                                              .append("realtime")
                                                              .append(QDir::separator())
                                                              .append("required"), tr("Scene Files (*.json)"));
    if (configFilePath.isNull()) {
        std::cout << "Failed to load null scenefile." << std::endl;
        return;
    }

    settings.sceneFilePath = configFilePath.toStdString();

    std::cout << "Loaded scenefile: \"" << configFilePath.toStdString() << "\"." << std::endl;

    realtime->sceneChanged();
}

void MainWindow::onSaveImage() {
    if (settings.sceneFilePath.empty()) {
        std::cout << "No scene file loaded." << std::endl;
        return;
    }
    std::string sceneName = settings.sceneFilePath.substr(0, settings.sceneFilePath.find_last_of("."));
    sceneName = sceneName.substr(sceneName.find_last_of("/")+1);
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save Image"),
                                                    QDir::currentPath()
                                                        .append(QDir::separator())
                                                        .append("student_outputs")
                                                        .append(QDir::separator())
                                                        .append("realtime")
                                                        .append(QDir::separator())
                                                        .append("required")
                                                        .append(QDir::separator())
                                                        .append(sceneName), tr("Image Files (*.png)"));
    std::cout << "Saving image to: \"" << filePath.toStdString() << "\"." << std::endl;
    realtime->saveViewportImage(filePath.toStdString());
}

void MainWindow::onValChangeP1(int newValue) {
    p1Slider->setValue(newValue);
    p1Box->setValue(newValue);
    settings.shapeParameter1 = p1Slider->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeP2(int newValue) {
    p2Slider->setValue(newValue);
    p2Box->setValue(newValue);
    settings.shapeParameter2 = p2Slider->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeNearSlider(int newValue) {
    //nearSlider->setValue(newValue);
    nearBox->setValue(newValue/100.f);
    settings.nearPlane = nearBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeFarSlider(int newValue) {
    //farSlider->setValue(newValue);
    farBox->setValue(newValue/100.f);
    settings.farPlane = farBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeFocusSlider(int newValue) {
    //farSlider->setValue(newValue);
    focusBox->setValue(newValue/100.f);
    settings.focusPlane = focusBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeSpeedSlider(int newValue) {
    //farSlider->setValue(newValue);
    speedBox->setValue(newValue/10.f);
    settings.moveSpeed = speedBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeNearBox(double newValue) {
    nearSlider->setValue(int(newValue*100.f));
    //nearBox->setValue(newValue);
    settings.nearPlane = nearBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeFarBox(double newValue) {
    farSlider->setValue(int(newValue*100.f));
    //farBox->setValue(newValue);
    settings.farPlane = farBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeFocusBox(double newValue) {
    focusSlider->setValue(int(newValue*100.f));
    //farBox->setValue(newValue);
    settings.focusPlane = focusBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeSpeedBox(double newValue) {
    speedSlider->setValue(int(newValue*10.f));
    //farBox->setValue(newValue);
    settings.moveSpeed = speedBox->value();
    realtime->settingsChanged();
}

// Extra Credit:

void MainWindow::onShadowMapping() {
    settings.shadowMapping = !settings.shadowMapping;
    realtime->settingsChanged();
}

void MainWindow::onScreenSpaceDOF() {
    settings.screenSpaceDOF = !settings.screenSpaceDOF;
    realtime->settingsChanged();
}

void MainWindow::onExtraCredit3() {
    settings.normalMapping = !settings.normalMapping;
    realtime->settingsChanged();
}

void MainWindow::onBumpMapping() {
    settings.bumpMapping = !settings.bumpMapping;
    realtime->settingsChanged();
}

void MainWindow::onExtraCredit4() {
    settings.extraCredit4 = !settings.extraCredit4;
    realtime->settingsChanged();
}

void MainWindow::onCameraPath() {
    settings.cameraPath = !settings.cameraPath;
    realtime->settingsChanged();
}

void MainWindow::onColorGrading() {
    settings.colorGrading = !settings.colorGrading;
    realtime->settingsChanged();
}

void MainWindow::onWatercolor() {
    settings.watercolor = !settings.watercolor;
    realtime->settingsChanged();
}

void MainWindow::onPixelated() {
    settings.pixelated = !settings.pixelated;
    realtime->settingsChanged();
}

void MainWindow::onValChangePixelSizeSlider(int newValue) {
    pixelSizeBox->setValue(newValue);
    settings.pixelSize = pixelSizeBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangePixelSizeBox(double newValue) {
    pixelSizeSlider->setValue(int(newValue));
    settings.pixelSize = pixelSizeBox->value();
    realtime->settingsChanged();
}
