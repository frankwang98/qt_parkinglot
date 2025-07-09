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
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), carX(150), carY(50), carAngle(0),
      isParking(false), currentTargetSpot(-1), currentParkingCarIndex(-1),
      steeringAngle(0), steeringRadius(0)
{
    ui->setupUi(this);
    setFixedSize(1200, 800);

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

    // 初始化三辆车
    cars.append({150, 50, 0, -1, 0, 0, 0, {}, false, Qt::blue});
    cars.append({350, 300, 0, -1, 0, 0, 0, {}, false, Qt::red});
    cars.append({550, 100, 0, -1, 0, 0, 0, {}, false, Qt::green});

    // 定时器用于模拟泊车过程
    parkingTimer = new QTimer(this);
    connect(parkingTimer, &QTimer::timeout, this, &MainWindow::moveCarsToSpots);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    drawParkingLot(painter);

    // 绘制所有车辆及其轨迹
    for (const Car &car : cars) {
        // 绘制轨迹
        painter.setPen(QPen(car.color, 3, Qt::DotLine));
        for (int i = 1; i < car.trajectory.size(); i++) {
            painter.drawLine(car.trajectory[i-1], car.trajectory[i]);
        }
        
        // 绘制车辆
        drawCar(painter, car);
    }
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

void MainWindow::drawCar(QPainter &painter, const Car &car)
{
    painter.save(); // 保存当前状态
    
    painter.setBrush(car.color);
    
    // 设置旋转中心为车辆中心
    QPoint center(car.x + 50, car.y + 90);
    painter.translate(center);
    painter.rotate(car.angle);
    painter.translate(-center);
    
    // 绘制车身
    painter.drawRect(car.x, car.y, 100, 180);
    
    // 绘制方向指示器
    painter.setPen(QPen(Qt::yellow, 2));
    painter.drawLine(center, QPoint(car.x + 50 + 50 * cos(car.angle * M_PI/180), 
                     car.y + 90 + 50 * sin(car.angle * M_PI/180)));
    
    painter.restore(); // 恢复状态
}

void MainWindow::startParkingProcess()
{
    logTextBox->clear();
    occupiedSpots.clear();
    currentParkingCarIndex = 0;

    // 重置所有车辆
    for (int i = 0; i < cars.size(); i++) {
        cars[i].x = QRandomGenerator::global()->bounded(50, 850);
        cars[i].y = QRandomGenerator::global()->bounded(50, 550);
        cars[i].angle = QRandomGenerator::global()->bounded(-45, 45);
        cars[i].targetSpot = -1;
        cars[i].trajectory.clear();
        cars[i].isParking = (i == 0); // 只有第一辆车开始停车
        cars[i].parkingState = 0;
    }

    // 为第一辆车寻找停车位
    if (!cars.isEmpty()) {
        closestPark(0);
        parkingTimer->start(50); // 加快更新频率
    }
}

void MainWindow::stopParkingProcess()
{
    for (Car &car : cars) {
        car.isParking = false;
    }
    parkingTimer->stop();
}

void MainWindow::moveCarsToSpots()
{
    if (currentParkingCarIndex < 0 || currentParkingCarIndex >= cars.size()) 
        return;
    
    Car &car = cars[currentParkingCarIndex];
    if (!car.isParking) return;
    
    // 记录轨迹点（每5帧记录一次）
    static int frameCount = 0;
    if (frameCount++ % 5 == 0) {
        car.trajectory.append(QPoint(car.x + 50, car.y + 90));
    }
    
    switch (car.parkingState) {
    case 0: // 调整角度
        if (fabs(car.angle) > rotationSpeed) {
            car.angle += (car.angle > 0 ? -rotationSpeed : rotationSpeed);
        } else {
            car.angle = 0;
            car.parkingState = 1;
            logTextBox->append(QString("车辆%1完成角度调整，开始位置调整")
                               .arg(currentParkingCarIndex));
        }
        break;
    
    case 1: // 调整位置
        float dx = (car.targetX - car.x) * 0.1;
        float dy = (car.targetY - car.y) * 0.1;
        car.x += dx;
        car.y += dy;

        // 检查碰撞
        bool collision = false;
        for (int i = 0; i < cars.size(); i++) {
            if (i == currentParkingCarIndex) continue;
            
            // 简单矩形碰撞检测
            QRect carRect1(car.x, car.y, 100, 180);
            QRect carRect2(cars[i].x, cars[i].y, 100, 180);
            
            if (carRect1.intersects(carRect2)) {
                collision = true;
                logTextBox->append(QString("警告：车辆%1与车辆%2可能发生碰撞！")
                                  .arg(currentParkingCarIndex).arg(i));
                break;
            }
        }

        // 检查是否到达目标位置
        if (fabs(car.x - car.targetX) < 1 && fabs(car.y - car.targetY) < 1) {
            car.x = car.targetX;
            car.y = car.targetY;
            car.isParking = false;
            
            // 标记停车位为已占用
            occupiedSpots.append(parkingSpaces[car.targetSpot]);
            
            logTextBox->append(QString("车辆%1已成功停入车位%2！")
                              .arg(currentParkingCarIndex).arg(car.targetSpot));
            
            // 开始下一辆车的停车
            currentParkingCarIndex++;
            if (currentParkingCarIndex < cars.size()) {
                cars[currentParkingCarIndex].isParking = true;
                closestPark(currentParkingCarIndex);
                logTextBox->append(QString("开始为车辆%1寻找车位...")
                                  .arg(currentParkingCarIndex));
            } else {
                logTextBox->append("所有车辆已完成停车！");
                parkingTimer->stop();
            }
        }
        break;
    }

    update();
}

void MainWindow::closestPark(int carIndex)
{
    Car &car = cars[carIndex];
    logTextBox->append(QString("为车辆%1寻找最近车位...").arg(carIndex));
    
    float minDistance = std::numeric_limits<float>::max();
    int closestIndex = -1;
    
    // 计算车辆中心点
    float carCenterX = car.x + 50;
    float carCenterY = car.y + 90;
    
    // 寻找最近的可用停车位
    for (int i = 0; i < parkingSpaces.size(); ++i) {
        const QRect &spot = parkingSpaces[i];
        
        // 跳过已占用停车位
        bool isOccupied = false;
        for (const QRect &occupied : occupiedSpots) {
            if (spot == occupied) {
                isOccupied = true;
                break;
            }
        }
        if (isOccupied) continue;
        
        // 计算停车位中心点
        float spotCenterX = spot.x() + spot.width() / 2.0;
        float spotCenterY = spot.y() + spot.height() / 2.0;
        
        // 计算欧几里得距离
        float dx = spotCenterX - carCenterX;
        float dy = spotCenterY - carCenterY;
        float distance = std::sqrt(dx*dx + dy*dy);
        
        // 更新最近停车位
        if (distance < minDistance) {
            minDistance = distance;
            closestIndex = i;
        }
    }
    
    if (closestIndex != -1) {
        car.targetSpot = closestIndex;
        const QRect &targetSpot = parkingSpaces[closestIndex];
        
        // 计算目标位置（车辆在停车位内居中）
        car.targetX = targetSpot.x() + (targetSpot.width() - 100) / 2.0;
        car.targetY = targetSpot.y() + (targetSpot.height() - 180) / 2.0;
        
        logTextBox->append(QString("车辆%1选择车位%2，目标位置：(%3, %4)")
                          .arg(carIndex)
                          .arg(closestIndex)
                          .arg(car.targetX, 0, 'f', 1)
                          .arg(car.targetY, 0, 'f', 1));
        
        car.parkingState = 0; // 初始状态为调整角度
    } else {
        logTextBox->append(QString("错误：未找到车辆%1的有效停车位").arg(carIndex));
        car.isParking = false;
    }
}
