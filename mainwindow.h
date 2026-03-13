#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QTextEdit>
#include <QProcess>
#include <QTimer>
#include <QSplitter>
#include <QProgressBar>
#include <QSlider>

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onBtnSelectImage();
    void onBtnDetectImage();
    void onBtnSelectVideo();
    void onBtnDetectVideo();
    void onBtnOpenCamera();
    void onBtnDetectCamera();
    void onBtnCloseCamera();  // 新增：关闭摄像头
    void onVideoPlayTimer();
    void onCameraTimer();
    void onPythonFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setupUI();
    void setupImageTab();
    void setupVideoTab();
    void setupCameraTab();
    void displayImage(const QString &imgPath, QLabel *target);
    void runDetection(const QString &inputPath, const QString &outputPath);
    void closeCameraProcess();  // 新增：清理摄像头进程

    // UI Widgets
    QTabWidget *tabWidget;
    QTextEdit *resultText;

    // Image Tab
    QWidget *imageTab;
    QSplitter *imageSplitter;
    QLabel *imageBeforeLabel;
    QLabel *imageAfterLabel;
    QPushButton *btnSelectImage;
    QPushButton *btnDetectImage;

    // Video Tab
    QWidget *videoTab;
    QSplitter *videoSplitter;
    QLabel *videoBeforeLabel;
    QLabel *videoAfterLabel;
    QPushButton *btnSelectVideo;
    QPushButton *btnDetectVideo;
    QProgressBar *videoProgress;
    QSlider *videoSlider;
    QPushButton *btnVideoPause;
    QLabel *videoTimeLabel;

    // Camera Tab
    QWidget *cameraTab;
    QSplitter *cameraSplitter;
    QLabel *cameraBeforeLabel;
    QLabel *cameraAfterLabel;
    QPushButton *btnOpenCamera;
    QPushButton *btnDetectCamera;
    QPushButton *btnCloseCamera;  // 新增：关闭按钮

    // Processes
    QProcess *pyProcess;
    QProcess *videoProcess;
    QProcess *cameraProcess;
    QTimer *videoPlayTimer;
    QTimer *cameraTimer;

    // Paths
    QString modelPath;
    QString pythonDir;
    QString outputDir;
    QString pythonExe;
    QString inputFile;
    QString outputFile;
    QString videoFilePath;

    // Video state
    int currentVideoFrame;
    int totalVideoFrames;

    // Camera state
    bool cameraRunning;  // 新增：摄像头状态标志
};

#endif // MAINWINDOW_H
