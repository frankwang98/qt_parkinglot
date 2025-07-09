#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QTextEdit>
#include <QRandomGenerator>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void drawParkingLot(QPainter &painter);
    void drawCar(QPainter &painter, int x, int y, float angle);

    void startParkingProcess();
    void stopParkingProcess();
    void moveCarToSpot();

    // 泊车算法
    void closestPark();

    // 控件
    QPushButton *startButton;
    QPushButton *stopButton;
    QTextEdit *logTextBox;

    // 主车
    int carX;
    int carY;
    float carAngle;

    // 停车场的停车位
    QVector<QRect> parkingSpaces;

    // 泊车过程的定时器
    QTimer *parkingTimer;

    // 控制泊车状态
    bool isParking;
    int currentTargetSpot;
    // int parkingAlgorithm;  // 泊车算法选择
    float steeringAngle;  // 当前的转向角度
    float steeringRadius; // 当前转向半径

    float targetX, targetY;
    float rotationSpeed = 2.0;    // 角度调整速度
    float movementSpeed = 2.0;    // 位置移动速度
    int parkingState;             // 泊车状态：0=调整角度, 1=调整位置

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
