#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QApplication>
#include <QDir>
#include <QDebug>
#include <QTimer>
#include <QThread>
#include <QProcess>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , pyProcess(nullptr)
    , videoProcess(nullptr)
    , cameraProcess(nullptr)
    , videoPlayTimer(nullptr)
    , cameraTimer(nullptr)
    , currentVideoFrame(0)
    , totalVideoFrames(0)
    , cameraRunning(false)
{
    // Get application directory
    QString appDir = QApplication::applicationDirPath();

    // Find Resources directory
    QString resourceDir = appDir + "/../Resources";
    if (!QDir(resourceDir).exists()) {
        resourceDir = "./Resources";
    }
    if (!QDir(resourceDir).exists()) {
        resourceDir = QApplication::applicationDirPath() + "/../../Resources";
    }

    // Set paths
    modelPath = resourceDir + "/models/best.pt";
    pythonDir = resourceDir + "/python";
    outputDir = appDir + "/output";
    pythonExe = "D:\\develop\\anaconda\\envs\\yolov8\\python.exe";

    // Create output directory
    QDir dir;
    if (!dir.exists(outputDir)) {
        dir.mkpath(outputDir);
    }

    qDebug() << "App Dir:" << appDir;
    qDebug() << "Resource Dir:" << resourceDir;
    qDebug() << "Model Path:" << modelPath;
    qDebug() << "Python Dir:" << pythonDir;
    qDebug() << "Output Dir:" << outputDir;
    qDebug() << "Python Exe:" << pythonExe;

    // Setup UI
    setupUI();

    // Initialize processes
    pyProcess = new QProcess(this);
    videoProcess = new QProcess(this);
    cameraProcess = new QProcess(this);

    connect(pyProcess, static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
            this, &MainWindow::onPythonFinished);

    // Setup timers
    videoPlayTimer = new QTimer(this);
    connect(videoPlayTimer, &QTimer::timeout, this, &MainWindow::onVideoPlayTimer);

    cameraTimer = new QTimer(this);
    connect(cameraTimer, &QTimer::timeout, this, &MainWindow::onCameraTimer);

    // Window settings
    setWindowTitle("YOLOv8 非机动车头盔佩戴检测系统");
    resize(1400, 900);

    resultText->append("✓ 系统启动成功!");
    resultText->append(QString("📁 输出目录: %1").arg(outputDir));
}

MainWindow::~MainWindow()
{
    closeCameraProcess();

    if (pyProcess && pyProcess->state() == QProcess::Running) {
        pyProcess->kill();
        pyProcess->waitForFinished();
    }
    if (videoProcess && videoProcess->state() == QProcess::Running) {
        videoProcess->kill();
        videoProcess->waitForFinished();
    }
}

void MainWindow::setupUI()
{
    tabWidget = new QTabWidget(this);

    setupImageTab();
    setupVideoTab();
    setupCameraTab();

    resultText = new QTextEdit;
    resultText->setReadOnly(true);
    resultText->setFixedHeight(100);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    mainLayout->addWidget(tabWidget, 1);
    mainLayout->addWidget(new QLabel("📋 检测结果信息:"), 0);
    mainLayout->addWidget(resultText, 0);
    setLayout(mainLayout);
}

void MainWindow::setupImageTab()
{
    imageTab = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout(imageTab);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnSelectImage = new QPushButton("📁 选择图片");
    btnDetectImage = new QPushButton("🚀 开始检测");
    btnDetectImage->setEnabled(false);
    btnSelectImage->setMinimumHeight(40);
    btnDetectImage->setMinimumHeight(40);
    btnLayout->addWidget(btnSelectImage);
    btnLayout->addWidget(btnDetectImage);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout, 0);

    imageSplitter = new QSplitter(Qt::Horizontal);
    imageSplitter->setStyleSheet("QSplitter::handle { background-color: #ccc; }");

    QWidget *leftWidget = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(5, 5, 5, 5);
    QLabel *leftTitle = new QLabel("📸 检测前:");
    leftTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    leftLayout->addWidget(leftTitle, 0);
    imageBeforeLabel = new QLabel;
    imageBeforeLabel->setMinimumSize(200, 200);
    imageBeforeLabel->setStyleSheet("border: 2px solid #3498db; background-color: #ecf0f1;");
    imageBeforeLabel->setAlignment(Qt::AlignCenter);
    imageBeforeLabel->setText("图片预览");
    leftLayout->addWidget(imageBeforeLabel, 1);

    QWidget *rightWidget = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(5, 5, 5, 5);
    QLabel *rightTitle = new QLabel("✅ 检测后:");
    rightTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    rightLayout->addWidget(rightTitle, 0);
    imageAfterLabel = new QLabel;
    imageAfterLabel->setMinimumSize(200, 200);
    imageAfterLabel->setStyleSheet("border: 2px solid #2ecc71; background-color: #ecf0f1;");
    imageAfterLabel->setAlignment(Qt::AlignCenter);
    imageAfterLabel->setText("检测结果");
    rightLayout->addWidget(imageAfterLabel, 1);

    imageSplitter->addWidget(leftWidget);
    imageSplitter->addWidget(rightWidget);
    imageSplitter->setCollapsible(0, false);
    imageSplitter->setCollapsible(1, false);
    imageSplitter->setSizes({700, 700});
    mainLayout->addWidget(imageSplitter, 1);

    connect(btnSelectImage, &QPushButton::clicked, this, &MainWindow::onBtnSelectImage);
    connect(btnDetectImage, &QPushButton::clicked, this, &MainWindow::onBtnDetectImage);

    tabWidget->addTab(imageTab, "📷 图片检测");
}

void MainWindow::setupVideoTab()
{
    videoTab = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout(videoTab);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnSelectVideo = new QPushButton("📁 选择视频");
    btnDetectVideo = new QPushButton("🚀 开始检测");
    btnDetectVideo->setEnabled(false);
    btnVideoPause = new QPushButton("⏸️  播放");
    btnVideoPause->setEnabled(false);
    btnSelectVideo->setMinimumHeight(40);
    btnDetectVideo->setMinimumHeight(40);
    btnVideoPause->setMinimumHeight(40);
    btnLayout->addWidget(btnSelectVideo);
    btnLayout->addWidget(btnDetectVideo);
    btnLayout->addWidget(btnVideoPause);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout, 0);

    videoSplitter = new QSplitter(Qt::Horizontal);
    videoSplitter->setStyleSheet("QSplitter::handle { background-color: #ccc; }");

    QWidget *leftWidget = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(5, 5, 5, 5);
    QLabel *leftTitle = new QLabel("🎬 检测前:");
    leftTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    leftLayout->addWidget(leftTitle, 0);
    videoBeforeLabel = new QLabel;
    videoBeforeLabel->setMinimumSize(200, 200);
    videoBeforeLabel->setStyleSheet("border: 2px solid #3498db; background-color: #ecf0f1;");
    videoBeforeLabel->setAlignment(Qt::AlignCenter);
    videoBeforeLabel->setText("视频首帧");
    leftLayout->addWidget(videoBeforeLabel, 1);

    QWidget *rightWidget = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(5, 5, 5, 5);
    QLabel *rightTitle = new QLabel("✅ 检测后:");
    rightTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    rightLayout->addWidget(rightTitle, 0);
    videoAfterLabel = new QLabel;
    videoAfterLabel->setMinimumSize(200, 200);
    videoAfterLabel->setStyleSheet("border: 2px solid #2ecc71; background-color: #ecf0f1;");
    videoAfterLabel->setAlignment(Qt::AlignCenter);
    videoAfterLabel->setText("检测结果");
    rightLayout->addWidget(videoAfterLabel, 1);

    videoSplitter->addWidget(leftWidget);
    videoSplitter->addWidget(rightWidget);
    videoSplitter->setCollapsible(0, false);
    videoSplitter->setCollapsible(1, false);
    videoSplitter->setSizes({700, 700});
    mainLayout->addWidget(videoSplitter, 1);

    QHBoxLayout *progressLayout = new QHBoxLayout;
    videoProgress = new QProgressBar;
    videoProgress->setValue(0);
    videoProgress->setMaximum(100);
    videoTimeLabel = new QLabel("0/0 帧");
    videoTimeLabel->setStyleSheet("font-size: 12px;");
    progressLayout->addWidget(new QLabel("进度:"));
    progressLayout->addWidget(videoProgress, 1);
    progressLayout->addWidget(videoTimeLabel);
    mainLayout->addLayout(progressLayout, 0);

    videoSlider = new QSlider(Qt::Horizontal);
    videoSlider->setSliderPosition(0);
    videoSlider->setEnabled(false);
    mainLayout->addWidget(videoSlider, 0);

    connect(btnSelectVideo, &QPushButton::clicked, this, &MainWindow::onBtnSelectVideo);
    connect(btnDetectVideo, &QPushButton::clicked, this, &MainWindow::onBtnDetectVideo);
    connect(btnVideoPause, &QPushButton::clicked, this, [this]() {
        if (videoPlayTimer->isActive()) {
            videoPlayTimer->stop();
            btnVideoPause->setText("▶️  播放");
        } else {
            videoPlayTimer->start(33);
            btnVideoPause->setText("⏸️  暂停");
        }
    });

    tabWidget->addTab(videoTab, "🎥 视频检测");
}

void MainWindow::setupCameraTab()
{
    cameraTab = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout(cameraTab);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnOpenCamera = new QPushButton("📹 打开摄像头");
    btnDetectCamera = new QPushButton("🚀 开始检测");
    btnDetectCamera->setEnabled(false);
    btnCloseCamera = new QPushButton("❌ 关闭摄像头");
    btnCloseCamera->setEnabled(false);
    btnOpenCamera->setMinimumHeight(40);
    btnDetectCamera->setMinimumHeight(40);
    btnCloseCamera->setMinimumHeight(40);
    btnLayout->addWidget(btnOpenCamera);
    btnLayout->addWidget(btnDetectCamera);
    btnLayout->addWidget(btnCloseCamera);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout, 0);

    cameraSplitter = new QSplitter(Qt::Horizontal);
    cameraSplitter->setStyleSheet("QSplitter::handle { background-color: #ccc; }");

    QWidget *leftWidget = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(5, 5, 5, 5);
    QLabel *leftTitle = new QLabel("📷 实时画面:");
    leftTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    leftLayout->addWidget(leftTitle, 0);
    cameraBeforeLabel = new QLabel;
    cameraBeforeLabel->setMinimumSize(200, 200);
    cameraBeforeLabel->setStyleSheet("border: 2px solid #3498db; background-color: #ecf0f1;");
    cameraBeforeLabel->setAlignment(Qt::AlignCenter);
    cameraBeforeLabel->setText("摄像头画面");
    leftLayout->addWidget(cameraBeforeLabel, 1);

    QWidget *rightWidget = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(5, 5, 5, 5);
    QLabel *rightTitle = new QLabel("✅ 检测结果:");
    rightTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    rightLayout->addWidget(rightTitle, 0);
    cameraAfterLabel = new QLabel;
    cameraAfterLabel->setMinimumSize(200, 200);
    cameraAfterLabel->setStyleSheet("border: 2px solid #2ecc71; background-color: #ecf0f1;");
    cameraAfterLabel->setAlignment(Qt::AlignCenter);
    cameraAfterLabel->setText("检测画面");
    rightLayout->addWidget(cameraAfterLabel, 1);

    cameraSplitter->addWidget(leftWidget);
    cameraSplitter->addWidget(rightWidget);
    cameraSplitter->setCollapsible(0, false);
    cameraSplitter->setCollapsible(1, false);
    cameraSplitter->setSizes({700, 700});
    mainLayout->addWidget(cameraSplitter, 1);

    connect(btnOpenCamera, &QPushButton::clicked, this, &MainWindow::onBtnOpenCamera);
    connect(btnDetectCamera, &QPushButton::clicked, this, &MainWindow::onBtnDetectCamera);
    connect(btnCloseCamera, &QPushButton::clicked, this, &MainWindow::onBtnCloseCamera);

    tabWidget->addTab(cameraTab, "📹 摄像头检测");
}

void MainWindow::onBtnSelectImage()
{
    QString file = QFileDialog::getOpenFileName(this, "选择图片", "",
                                                "Images (*.png *.jpg *.bmp *.jpeg);;All Files (*)");
    if (!file.isEmpty()) {
        inputFile = file;
        btnDetectImage->setEnabled(true);
        displayImage(file, imageBeforeLabel);
        resultText->append("✓ 已选择: " + file);
        imageAfterLabel->clear();
        imageAfterLabel->setText("等待检测...");
    }
}

void MainWindow::onBtnDetectImage()
{
    if (inputFile.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择图片!");
        return;
    }

    QFileInfo fileInfo(inputFile);
    outputFile = outputDir + "/" + fileInfo.baseName() + "_detected.jpg";

    resultText->append("⏳ 正在检测...");
    resultText->append("💾 保存到: " + outputFile);
    runDetection(inputFile, outputFile);
}

void MainWindow::onBtnSelectVideo()
{
    QString file = QFileDialog::getOpenFileName(this, "选择视频", "",
                                                "Videos (*.mp4 *.avi *.mov *.mkv);;All Files (*)");
    if (!file.isEmpty()) {
        videoFilePath = file;
        btnDetectVideo->setEnabled(true);
        resultText->append("✓ 已选择: " + file);

        // 提取视频第一帧
        QString tempFirstFrame = outputDir + "/temp_first_frame.jpg";
        QString scriptPath = pythonDir + "/get_first_frame.py";

        resultText->append("⏳ 正在提取第一帧...");

        QProcess process;
        QStringList args;
        args << scriptPath << videoFilePath << tempFirstFrame;

        process.start(pythonExe, args);

        if (process.waitForFinished(5000)) {
            QThread::msleep(200);

            QPixmap pixmap(tempFirstFrame);
            if (!pixmap.isNull()) {
                videoBeforeLabel->setPixmap(pixmap.scaledToWidth(videoBeforeLabel->width(),
                                                                 Qt::SmoothTransformation));
                resultText->append("✓ 已显示视频第一帧");
            } else {
                resultText->append("⚠️  无法加载第一帧图像");
            }
        } else {
            resultText->append("⚠️  提取第一帧超时或失败");
            QString err = QString::fromLocal8Bit(process.readAllStandardError());
            if (!err.isEmpty()) {
                resultText->append("错误: " + err);
            }
        }
    }
}

void MainWindow::onBtnDetectVideo()
{
    if (videoFilePath.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择视频!");
        return;
    }

    resultText->append("⏳ 正在处理视频...");
    resultText->append("💾 保存到: " + outputDir);

    QString scriptPath = pythonDir + "/process_video.py";

    QStringList args;
    args << scriptPath << videoFilePath << modelPath << outputDir;

    resultText->append("执行: " + args.join(" "));

    disconnect(videoProcess, nullptr, nullptr, nullptr);

    connect(videoProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        QString output = QString::fromLocal8Bit(videoProcess->readAllStandardOutput());
        resultText->append(output);
    });

    connect(videoProcess, &QProcess::readyReadStandardError, this, [this]() {
        QString error = QString::fromLocal8Bit(videoProcess->readAllStandardError());
        resultText->append("⚠️  " + error);
    });

    connect(videoProcess, static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
                resultText->append(QString("========== Python进程结束 (退出码: %1) ==========").arg(exitCode));

                if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
                    resultText->append("❌ 视频处理失败!");
                    return;
                }

                QDir dir(outputDir);
                QStringList frames = dir.entryList(QStringList() << "frame_*.jpg", QDir::Files);

                resultText->append(QString("📁 输出目录文件数: %1").arg(dir.entryList().count()));
                resultText->append(QString("🎬 检测帧数: %1").arg(frames.count()));

                totalVideoFrames = frames.count();

                if (totalVideoFrames > 0) {
                    resultText->append(QString("✓ 完成! 共 %1 帧").arg(totalVideoFrames));
                    currentVideoFrame = 0;
                    videoSlider->setMaximum(totalVideoFrames - 1);
                    videoSlider->setEnabled(true);
                    btnVideoPause->setEnabled(true);
                    videoPlayTimer->start(33);
                } else {
                    resultText->append("❌ 未生成任何帧!");
                }
            });

    videoProcess->start(pythonExe, args);
}

void MainWindow::onVideoPlayTimer()
{
    if (currentVideoFrame >= totalVideoFrames) {
        videoPlayTimer->stop();
        btnVideoPause->setText("▶️  播放");
        currentVideoFrame = 0;
        return;
    }

    QString originalFile = QString("%1/original_%2.jpg").arg(outputDir).arg(currentVideoFrame, 4, 10, QChar('0'));
    QString detectedFile = QString("%1/frame_%2.jpg").arg(outputDir).arg(currentVideoFrame, 4, 10, QChar('0'));

    displayImage(originalFile, videoBeforeLabel);
    displayImage(detectedFile, videoAfterLabel);

    videoSlider->setValue(currentVideoFrame);
    int progress = (currentVideoFrame * 100) / totalVideoFrames;
    videoProgress->setValue(progress);
    videoTimeLabel->setText(QString("%1/%2").arg(currentVideoFrame + 1).arg(totalVideoFrames));

    currentVideoFrame++;
}

void MainWindow::onBtnOpenCamera()
{
    if (cameraRunning) {
        QMessageBox::information(this, "提示", "摄像头已经打开!");
        return;
    }

    // 清空旧文件
    QFile::remove(outputDir + "/latest.jpg");
    QFile::remove(outputDir + "/latest_detected.jpg");
    QFile::remove(outputDir + "/camera_mode.txt");

    QString scriptPath = pythonDir + "/process_camera.py";
    QStringList args;
    args << scriptPath << modelPath << outputDir;

    resultText->append("=== 摄像头初始化 ===");
    resultText->append("启动中...");

    disconnect(cameraProcess, nullptr, nullptr, nullptr);

    // 监听输出
    connect(cameraProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        QString output = QString::fromLocal8Bit(cameraProcess->readAllStandardOutput()).trimmed();
        if (output.contains("SUCCESS") || output.contains("ERROR")) {
            resultText->append(output);
        }
    });

    connect(cameraProcess, &QProcess::readyReadStandardError, this, [this]() {
        QString error = QString::fromLocal8Bit(cameraProcess->readAllStandardError()).trimmed();
        if (!error.isEmpty() && !error.contains("[DEBUG]")) {
            resultText->append("⚠️  " + error);
        }
    });

    connect(cameraProcess, static_cast<void(QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
            this, [this](int exitCode) {
                cameraRunning = false;
                btnDetectCamera->setEnabled(false);
                btnDetectCamera->setText("🚀 开始检测");
                btnCloseCamera->setEnabled(false);
                cameraTimer->stop();
                cameraBeforeLabel->clear();
                cameraBeforeLabel->setText("摄像头已关闭");
                cameraAfterLabel->clear();
                cameraAfterLabel->setText("摄像头已关闭");
                resultText->append("摄像头进程已关闭");
            });

    // 启动进程
    cameraProcess->start(pythonExe, args);

    if (!cameraProcess->waitForStarted(3000)) {
        resultText->append("❌ 无法启动摄像头!");
        return;
    }

    cameraRunning = true;
    btnDetectCamera->setEnabled(true);
    btnCloseCamera->setEnabled(true);

    // 写入显示模式
    QString modeFile = outputDir + "/camera_mode.txt";
    QFile file(modeFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write("display");
        file.close();
    }

    // 启动显示定时器
    cameraTimer->start(200);

    resultText->append("✓ 摄像头已启动");
    cameraBeforeLabel->clear();
    cameraBeforeLabel->setText("摄像头初始化中...");
    cameraAfterLabel->clear();
    cameraAfterLabel->setText("点击'开始检测'启动检测");
}

void MainWindow::onBtnDetectCamera()
{
    if (!cameraRunning || !cameraProcess || cameraProcess->state() != QProcess::Running) {
        QMessageBox::warning(this, "警告", "请先打开摄像头!");
        return;
    }

    QString modeFile = outputDir + "/camera_mode.txt";

    if (btnDetectCamera->text().contains("停止")) {
        // 停止检测 - 切换为显示模式
        resultText->append("⏹️  停止检测");
        btnDetectCamera->setText("🚀 开始检测");

        // 写入显示模式
        QFile file(modeFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write("display");
            file.close();
        }

        cameraAfterLabel->clear();
        cameraAfterLabel->setText("点击'开始检测'启动检测");

    } else {
        // 开始检测 - 切换为检测模式
        resultText->append("⏳ 启动实时检测...");
        btnDetectCamera->setText("⏹️  停止检测");

        // 写入检测模式
        QFile file(modeFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write("detect");
            file.close();
        }

        cameraAfterLabel->clear();
        cameraAfterLabel->setText("检测初始化中...");
        resultText->append("✓ 检测模式已激活");
    }
}

void MainWindow::onBtnCloseCamera()
{
    resultText->append("正在关闭摄像头...");
    closeCameraProcess();
}

void MainWindow::closeCameraProcess()
{
    if (!cameraRunning) {
        return;
    }

    // 停止定时器
    if (cameraTimer && cameraTimer->isActive()) {
        cameraTimer->stop();
    }

    // 关闭进程
    if (cameraProcess && cameraProcess->state() == QProcess::Running) {
        cameraProcess->terminate();

        if (!cameraProcess->waitForFinished(2000)) {
            cameraProcess->kill();
            cameraProcess->waitForFinished();
        }
    }

    // 更新UI状态
    cameraRunning = false;
    btnDetectCamera->setEnabled(false);
    btnDetectCamera->setText("🚀 开始检测");
    btnCloseCamera->setEnabled(false);

    cameraBeforeLabel->clear();
    cameraBeforeLabel->setText("摄像头已关闭");
    cameraAfterLabel->clear();
    cameraAfterLabel->setText("摄像头已关闭");

    // 清理文件
    QFile::remove(outputDir + "/camera_mode.txt");

    resultText->append("✓ 摄像头已关闭");
}

void MainWindow::onCameraTimer()
{
    QString latestFile = outputDir + "/latest.jpg";
    QString detectedFile = outputDir + "/latest_detected.jpg";

    // 显示原始帧
    QPixmap pixmapBefore(latestFile);
    if (!pixmapBefore.isNull()) {
        static QPixmap lastPixmap;
        if (pixmapBefore.cacheKey() != lastPixmap.cacheKey()) {
            lastPixmap = pixmapBefore;
            QPixmap scaled = pixmapBefore.scaledToWidth(cameraBeforeLabel->width(), Qt::SmoothTransformation);
            cameraBeforeLabel->setPixmap(scaled);
        }
    }

    // 显示检测结果（如果存在）
    if (btnDetectCamera->text().contains("停止")) {
        QPixmap pixmapAfter(detectedFile);
        if (!pixmapAfter.isNull()) {
            static QPixmap lastDetectedPixmap;
            if (pixmapAfter.cacheKey() != lastDetectedPixmap.cacheKey()) {
                lastDetectedPixmap = pixmapAfter;
                QPixmap scaled = pixmapAfter.scaledToWidth(cameraAfterLabel->width(), Qt::SmoothTransformation);
                cameraAfterLabel->setPixmap(scaled);
            }
        }
    }
}

void MainWindow::runDetection(const QString &inputPath, const QString &outputPath)
{
    if (pyProcess->state() == QProcess::Running) {
        return;
    }

    QString scriptPath = pythonDir + "/detect.py";

    QStringList args;
    args << scriptPath << modelPath << inputPath << outputPath;

    resultText->append("执行: " + args.join(" "));

    pyProcess->start(pythonExe, args);
}

void MainWindow::onPythonFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        if (tabWidget->currentIndex() == 0) {
            QTimer::singleShot(500, this, [this]() {
                displayImage(outputFile, imageAfterLabel);
                resultText->append("✓ 检测完成!");
                resultText->append("📁 文件: " + outputFile);
            });
        }
    } else {
        QString err = QString::fromLocal8Bit(pyProcess->readAllStandardError());
        resultText->append("❌ 错误: " + err);
    }
}

void MainWindow::displayImage(const QString &imgPath, QLabel *target)
{
    QPixmap pixmap(imgPath);
    if (!pixmap.isNull()) {
        target->setPixmap(pixmap.scaledToWidth(target->width(), Qt::SmoothTransformation));
    }
}
