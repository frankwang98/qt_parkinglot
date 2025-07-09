#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QKeyEvent>
#include <QTimerEvent>
#include <QPushButton>
#include <QTextEdit>
#include <QRandomGenerator>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), carX(150), carY(50), carAngle(0),
      isParking(false), currentTargetSpot(-1),
      steeringAngle(0), steeringRadius(0)
{
    ui->setupUi(this);
    setFixedSize(800, 600);

    // 创建按钮和文本框
    startButton = new QPushButton("start", this);
    startButton->setGeometry(550, 450, 150, 50);
    connect(startButton, &QPushButton::clicked, this, &MainWindow::startParkingProcess);

    stopButton = new QPushButton("stop", this);
    stopButton->setGeometry(550, 510, 150, 50);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopParkingProcess);

    logTextBox = new QTextEdit(this);
    logTextBox->setGeometry(50, 450, 400, 120);
    logTextBox->setReadOnly(true);  // 只读

    // 初始化停车场的停车位
    for (int i = 0; i < 5; ++i) {
        parkingSpaces.append(QRect(60 + i * 140, 60, 120, 200));  // 每个停车位的位置
    }

    // 定时器用于模拟泊车过程
    parkingTimer = new QTimer(this);
    connect(parkingTimer, &QTimer::timeout, this, &MainWindow::moveCarToSpot);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    // 设置线条颜色和宽度
    QPen pen;
    pen.setColor(Qt::black);
    pen.setWidth(3);
    painter.setPen(pen);

    // 设置填充颜色
    QBrush brush(Qt::lightGray);
    painter.setBrush(brush);

    drawParkingLot(painter);
    drawCar(painter, carX, carY, carAngle);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    int moveStep = 10; // 每次按键移动的步长

    if (event->key() == Qt::Key_Up)
    {
        carY -= moveStep; // 向上移动主车
    }
    else if (event->key() == Qt::Key_Down)
    {
        carY += moveStep; // 向下移动主车
    }
    else if (event->key() == Qt::Key_Left)
    {
        carX -= moveStep; // 向左移动主车
    }
    else if (event->key() == Qt::Key_Right)
    {
        carX += moveStep; // 向右移动主车
    }

    // 确保车不会移动出停车场区域
    if (carX < 50)
        carX = 50;
    if (carY < 50)
        carY = 50;
    if (carX > 50 + 700 - 100)
        carX = 50 + 700 - 100; // 停车场宽度 - 车宽
    if (carY > 50 + 400 - 180)
        carY = 50 + 400 - 180; // 停车场高度 - 车高

    // 触发重新绘制
    update();
}

void MainWindow::drawParkingLot(QPainter &painter)
{
    // 停车场尺寸
    int lotWidth = 700;
    int lotHeight = 400;

    painter.setBrush(Qt::lightGray);
    painter.drawRect(50, 50, lotWidth, lotHeight);

    // 绘制停车位
    for (const auto &spot : parkingSpaces) {
        painter.drawRect(spot);
    }
}

void MainWindow::drawCar(QPainter &painter, int x, int y, float angle)
{
    painter.setBrush(Qt::blue);

    QMatrix matrix;
    matrix.rotate(angle);
    painter.setTransform(QTransform(matrix));

    painter.drawRect(x, y, 100, 180);
}

void MainWindow::startParkingProcess()
{
    logTextBox->clear();

    carX = QRandomGenerator::global()->bounded(50, 650);
    carY = QRandomGenerator::global()->bounded(50, 250);

    isParking = true; // 开始泊车过程
    closestPark();

    parkingTimer->start(100);
}

void MainWindow::stopParkingProcess()
{
    // 停止泊车过程
    isParking = false;
    parkingTimer->stop();
}

void MainWindow::moveCarToSpot()
{
    if (!isParking || currentTargetSpot < 0) return;

    switch (parkingState) {
    case 0: // 调整角度
        if (fabs(carAngle) > rotationSpeed) {
            // 平滑调整角度至0度
            carAngle += (carAngle > 0 ? -rotationSpeed : rotationSpeed);
        } else {
            carAngle = 0; // 完成角度调整
            parkingState = 1; // 进入位置调整阶段
        }
        break;
    
    case 1: // 调整位置
        float dx = (targetX - carX) * 0.1;
        float dy = (targetY - carY) * 0.1;
        carX += dx;
        carY += dy;

        // 检查是否到达目标位置
        if (fabs(carX - targetX) < 1 && fabs(carY - targetY) < 1) {
            carX = targetX;
            carY = targetY;
            logTextBox->append("主车已成功入库！");
            parkingTimer->stop();
        }
        break;
    }

    update();
}

void MainWindow::closestPark()
{
    logTextBox->append("closest泊车开始...");
    float minDistance = std::numeric_limits<float>::max();
    int closestIndex = -1;

    // 计算车辆中心点
    float carCenterX = carX + 50;  // 车宽100，中心=carX+50
    float carCenterY = carY + 90;  // 车高180，中心=carY+90

    // 寻找最近的停车位
    for (int i = 0; i < parkingSpaces.size(); ++i) {
        const QRect &spot = parkingSpaces[i];
        
        // 计算停车位中心点
        float spotCenterX = spot.x() + spot.width() / 2.0;
        float spotCenterY = spot.y() + spot.height() / 2.0;
        
        // 计算欧几里得距离
        float dx = spotCenterX - carCenterX;
        float dy = spotCenterY - carCenterY;
        float distance = std::sqrt(dx*dx + dy*dy);
        
        // 检查边界冲突
        bool collision = false;
        for (int j = 0; j < parkingSpaces.size(); ++j) {
            if (j == i) continue;
            if (spot.intersects(parkingSpaces[j].adjusted(-10, -10, 10, 10))) {
                collision = true;
                break;
            }
        }
        
        // 更新最近且无冲突的停车位
        if (distance < minDistance && !collision) {
            minDistance = distance;
            closestIndex = i;
        }
    }

    if (closestIndex != -1) {
        currentTargetSpot = closestIndex;
        const QRect &targetSpot = parkingSpaces[closestIndex];
        
        // 计算目标位置（车辆在停车位内居中）
        targetX = targetSpot.x() + (targetSpot.width() - 100) / 2.0;
        targetY = targetSpot.y() + (targetSpot.height() - 180) / 2.0;
        
        logTextBox->append(QString("选择停车位%1，目标位置：(%2, %3)")
                          .arg(closestIndex)
                          .arg(targetX, 0, 'f', 1)
                          .arg(targetY, 0, 'f', 1));
        
        parkingState = 0; // 初始状态为调整角度
    } else {
        logTextBox->append("错误：未找到有效停车位");
        stopParkingProcess();
    }
}
