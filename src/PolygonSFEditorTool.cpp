#include "PolygonSFEditorTool.h"
#include "SFValueInputDialog.h"

// Qt
#include <QToolBar>
#include <QToolButton>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QMessageBox>
#include <QStyle>
#include <QMainWindow>

// CC includes
#include <ccPointCloud.h>
#include <ccMainAppInterface.h>
#include <ccGLWindowInterface.h>
#include <ccLog.h>
#include <ccPolyline.h>

// CCCoreLib
#include <ManualSegmentationTools.h>

// System
#include <set>

PolygonSFEditorTool::PolygonSFEditorTool(ccPointCloud* cloud, int sfIndex, ccMainAppInterface* app)
    : QObject()
    , m_cloud(cloud)
    , m_sfIndex(sfIndex)
    , m_app(app)
    , m_glWindow(nullptr)
    , m_toolBar(nullptr)
    , m_acceptButton(nullptr)
    , m_cancelButton(nullptr)
    , m_resetButton(nullptr)
    , m_isActive(false)
    , m_previousInteractionMode(0)
    , m_segmentationPolyline(nullptr)
{
}

PolygonSFEditorTool::~PolygonSFEditorTool()
{
    stop(false);
}

bool PolygonSFEditorTool::start()
{
    if (!m_cloud || !m_app) return false;
    
    // 获取活动的GL窗口
    ccGLWindowInterface* glInterface = m_app->getActiveGLWindow();
    if (!glInterface) return false;
    
    m_glWindow = dynamic_cast<ccGLWindow*>(glInterface);
    if (!m_glWindow) return false;
    
    // 创建分割多边形
    ccPointCloud* polyVertices = new ccPointCloud("vertices");
    m_segmentationPolyline = new ccPolyline(polyVertices);
    m_segmentationPolyline->setForeground(true);
    m_segmentationPolyline->setColor(ccColor::green);
    m_segmentationPolyline->showColors(true);
    m_segmentationPolyline->set2DMode(true);
    m_segmentationPolyline->setDisplay(m_glWindow);
    
    // 添加到GL窗口
    m_glWindow->addToOwnDB(m_segmentationPolyline);
    
    // 设置交互模式
    m_glWindow->setInteractionMode(ccGLWindowInterface::INTERACT_SEND_ALL_SIGNALS);
    m_glWindow->setPickingMode(ccGLWindowInterface::NO_PICKING);
    
    // 安装事件过滤器
    m_glWindow->installEventFilter(this);
    
    // 设置工具栏
    setupToolBar();
    
    m_isActive = true;
    
    // 更新状态消息
    CCCoreLib::ScalarField* sf = m_cloud->getScalarField(m_sfIndex);
    QString sfName = sf ? QString::fromStdString(sf->getName()) : "Unknown";
    
    ccLog::Print(QString("[PolygonSFEditor] Editing SF: %1").arg(sfName));
    ccLog::Print("[PolygonSFEditor] Left-click: add vertex | Right-click: close polygon | Enter: confirm | Escape: cancel");
    
    // 设置鼠标光标为十字准星
    m_glWindow->setCursor(Qt::CrossCursor);
    
    // 显示消息
    m_glWindow->displayNewMessage("Polygon SF Editor [ON]", 
                                  ccGLWindowInterface::UPPER_CENTER_MESSAGE, 
                                  false, 3600, 
                                  ccGLWindowInterface::MANUAL_SEGMENTATION_MESSAGE);
    m_glWindow->displayNewMessage("Left click: add vertex | Right click: close", 
                                  ccGLWindowInterface::UPPER_CENTER_MESSAGE, 
                                  true, 3600, 
                                  ccGLWindowInterface::MANUAL_SEGMENTATION_MESSAGE);
    
    return true;
}

void PolygonSFEditorTool::stop(bool accepted)
{
    if (!m_isActive) return;
    
    m_isActive = false;
    
    // 移除事件过滤器
    if (m_glWindow) {
        m_glWindow->removeEventFilter(this);
        
        // 恢复交互模式
        m_glWindow->setInteractionMode(ccGLWindowInterface::MODE_TRANSFORM_CAMERA);
        m_glWindow->setPickingMode(ccGLWindowInterface::DEFAULT_PICKING);
        
        // 恢复鼠标光标
        m_glWindow->setCursor(Qt::ArrowCursor);
        
        // 清理显示消息
        m_glWindow->displayNewMessage("Polygon SF Editor [OFF]", 
                                      ccGLWindowInterface::UPPER_CENTER_MESSAGE, 
                                      false, 2, 
                                      ccGLWindowInterface::MANUAL_SEGMENTATION_MESSAGE);
        
        // 移除多边形
        if (m_segmentationPolyline) {
            m_glWindow->removeFromOwnDB(m_segmentationPolyline);
            delete m_segmentationPolyline;
            m_segmentationPolyline = nullptr;
        }
        
        // 请求重绘
        m_glWindow->redraw(true);
    }
    
    // 清理工具栏
    cleanupToolBar();
    
    // 清理状态
    m_polygonPoints.clear();
    
    emit finished(accepted);
}

void PolygonSFEditorTool::setupToolBar()
{
    if (!m_app) return;
    
    // 创建工具栏
    m_toolBar = new QToolBar("Polygon SF Editor", nullptr);
    m_toolBar->setObjectName("PolygonSFEditorToolBar");
    
    // 确认按钮
    m_acceptButton = new QToolButton();
    m_acceptButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogOkButton));
    m_acceptButton->setToolTip("Accept selection and modify SF values (Enter)");
    m_acceptButton->setEnabled(false);
    connect(m_acceptButton, &QToolButton::clicked, this, &PolygonSFEditorTool::onAccept);
    
    // 取消按钮
    m_cancelButton = new QToolButton();
    m_cancelButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton));
    m_cancelButton->setToolTip("Cancel and exit (Escape)");
    connect(m_cancelButton, &QToolButton::clicked, this, &PolygonSFEditorTool::onCancel);
    
    // 重置按钮
    m_resetButton = new QToolButton();
    m_resetButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogResetButton));
    m_resetButton->setToolTip("Reset polygon (Delete)");
    connect(m_resetButton, &QToolButton::clicked, this, &PolygonSFEditorTool::onReset);
    
    // 添加到工具栏
    m_toolBar->addWidget(m_acceptButton);
    m_toolBar->addWidget(m_cancelButton);
    m_toolBar->addSeparator();
    m_toolBar->addWidget(m_resetButton);
    
    // 添加到主窗口
    if (m_app->getMainWindow()) {
        QMainWindow* mainWin = dynamic_cast<QMainWindow*>(m_app->getMainWindow());
        if (mainWin) {
            mainWin->addToolBar(Qt::TopToolBarArea, m_toolBar);
            m_toolBar->show();
        }
    }
}

void PolygonSFEditorTool::cleanupToolBar()
{
    if (m_toolBar) {
        // 断开所有信号连接
        if (m_acceptButton) {
            disconnect(m_acceptButton, nullptr, this, nullptr);
        }
        if (m_cancelButton) {
            disconnect(m_cancelButton, nullptr, this, nullptr);
        }
        if (m_resetButton) {
            disconnect(m_resetButton, nullptr, this, nullptr);
        }
        
        // 从主窗口移除工具栏
        if (m_app && m_app->getMainWindow()) {
            QMainWindow* mainWin = dynamic_cast<QMainWindow*>(m_app->getMainWindow());
            if (mainWin) {
                mainWin->removeToolBar(m_toolBar);
            }
        }
        
        // 隐藏并删除工具栏
        m_toolBar->hide();
        m_toolBar->deleteLater();
        m_toolBar = nullptr;
    }
    
    // 清空按钮指针
    m_acceptButton = nullptr;
    m_cancelButton = nullptr;
    m_resetButton = nullptr;
}

bool PolygonSFEditorTool::eventFilter(QObject* obj, QEvent* event)
{
    if (!m_isActive || obj != m_glWindow) {
        return QObject::eventFilter(obj, event);
    }
    
    switch (event->type()) {
        case QEvent::MouseButtonPress:
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            handleMousePress(mouseEvent);
            return true;
        }
        case QEvent::MouseMove:
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            handleMouseMove(mouseEvent);
            return false;
        }
        case QEvent::KeyPress:
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            handleKeyPress(keyEvent);
            return true;
        }
        default:
            break;
    }
    
    return false;
}

void PolygonSFEditorTool::handleMousePress(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 添加多边形顶点
        addPointToPolyline(event->x(), event->y());
    }
    else if (event->button() == Qt::RightButton) {
        // 右键关闭多边形
        closePolyline();
    }
}

void PolygonSFEditorTool::handleMouseMove(QMouseEvent* event)
{
    if (!m_segmentationPolyline || m_polygonPoints.empty()) return;
    
    // 更新预览线
    ccPointCloud* vertices = dynamic_cast<ccPointCloud*>(m_segmentationPolyline->getAssociatedCloud());
    if (!vertices) return;
    
    unsigned vertCount = vertices->size();
    if (vertCount < 2) return;
    
    // 更新最后一个点位置
    QPointF pos2D = m_glWindow->toCenteredGLCoordinates(event->x(), event->y());
    CCVector3 P(static_cast<PointCoordinateType>(pos2D.x()),
                static_cast<PointCoordinateType>(pos2D.y()),
                0);
    
    CCVector3* lastP = const_cast<CCVector3*>(vertices->getPointPersistentPtr(vertCount - 1));
    *lastP = P;
    
    m_glWindow->redraw(true, false);
}

void PolygonSFEditorTool::handleKeyPress(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        if (m_polygonPoints.size() >= 3) {
            onAccept();
        }
        break;
    case Qt::Key_Escape:
        onCancel();
        break;
    case Qt::Key_Delete:
        onReset();
        break;
    case Qt::Key_Backspace:
        // 删除最后一个点
        if (!m_polygonPoints.empty()) {
            removeLastPoint();
        }
        break;
    default:
        break;
    }
}

void PolygonSFEditorTool::addPointToPolyline(int x, int y)
{
    if (!m_segmentationPolyline) return;
    
    // 转换为GL坐标
    QPointF pos2D = m_glWindow->toCenteredGLCoordinates(x, y);
    CCVector3 P(static_cast<PointCoordinateType>(pos2D.x()),
                static_cast<PointCoordinateType>(pos2D.y()),
                0);
    
    ccPointCloud* vertices = dynamic_cast<ccPointCloud*>(m_segmentationPolyline->getAssociatedCloud());
    if (!vertices) return;
    
    unsigned vertCount = vertices->size();
    
    // 第一个点：添加两个相同的点（第二个用于预览）
    if (vertCount == 0) {
        if (!vertices->reserve(2)) {
            ccLog::Error("Out of memory!");
            return;
        }
        vertices->addPoint(P);
        vertices->addPoint(P);
        m_segmentationPolyline->clear();
        if (!m_segmentationPolyline->addPointIndex(0, 2)) {
            ccLog::Error("Out of memory!");
            return;
        }
        
        // 添加到屏幕坐标列表
        m_polygonPoints.push_back(QPointF(x, y));
    }
    else {
        // 后续点：替换最后一个点，添加新的预览点
        if (!vertices->reserve(vertCount + 1)) {
            ccLog::Error("Out of memory!");
            return;
        }
        
        CCVector3* lastP = const_cast<CCVector3*>(vertices->getPointPersistentPtr(vertCount - 1));
        *lastP = P;
        vertices->addPoint(P);
        
        if (!m_segmentationPolyline->addPointIndex(vertCount)) {
            ccLog::Error("Out of memory!");
            return;
        }
        
        // 添加到屏幕坐标列表
        m_polygonPoints.push_back(QPointF(x, y));
    }
    
    updateDisplay();
    updateButtonStates();
    
    ccLog::Print(QString("[PolygonSFEditor] Added vertex %1 at (%2, %3)")
                .arg(m_polygonPoints.size())
                .arg(x).arg(y));
}

void PolygonSFEditorTool::removeLastPoint()
{
    if (m_polygonPoints.empty() || !m_segmentationPolyline) return;
    
    ccPointCloud* vertices = dynamic_cast<ccPointCloud*>(m_segmentationPolyline->getAssociatedCloud());
    if (!vertices) return;
    
    unsigned vertCount = vertices->size();
    if (vertCount > 2) {
        // 移除一个顶点
        vertices->resize(vertCount - 1);
        m_segmentationPolyline->resize(m_segmentationPolyline->size() - 1);
        m_polygonPoints.pop_back();
        
        updateDisplay();
        updateButtonStates();
        
        ccLog::Print("[PolygonSFEditor] Removed last vertex");
    }
}

void PolygonSFEditorTool::closePolyline()
{
    if (!m_segmentationPolyline || m_polygonPoints.size() < 3) return;
    
    ccPointCloud* vertices = dynamic_cast<ccPointCloud*>(m_segmentationPolyline->getAssociatedCloud());
    if (!vertices) return;
    
    unsigned vertCount = vertices->size();
    if (vertCount > 0) {
        // 移除最后一个预览点
        vertices->resize(vertCount - 1);
        m_segmentationPolyline->resize(m_segmentationPolyline->size() - 1);
        m_segmentationPolyline->setClosed(true);
    }
    
    updateDisplay();
    updateButtonStates();
    
    ccLog::Print("[PolygonSFEditor] Polygon closed");
}

void PolygonSFEditorTool::updateDisplay()
{
    if (m_glWindow) {
        m_glWindow->redraw(false);
    }
}

void PolygonSFEditorTool::updateButtonStates()
{
    if (m_acceptButton) {
        // 需要至少3个点且多边形已关闭
        bool canAccept = (m_polygonPoints.size() >= 3) && 
                        (m_segmentationPolyline && m_segmentationPolyline->isClosed());
        m_acceptButton->setEnabled(canAccept);
    }
    
    if (m_resetButton) {
        m_resetButton->setEnabled(!m_polygonPoints.empty());
    }
}

void PolygonSFEditorTool::draw(ccGLWindow* win)
{
    // 绘制功能由 ccPolyline 自动处理
}

void PolygonSFEditorTool::drawPolygon2D(ccGLWindow* win)
{
    // 由 ccPolyline 处理
}

void PolygonSFEditorTool::drawVertices2D(ccGLWindow* win)
{
    // 由 ccPolyline 处理
}

void PolygonSFEditorTool::onAccept()
{
    if (m_polygonPoints.size() < 3) {
        QString msg = QString("Need at least 3 vertices to define a polygon.\nCurrent vertices: %1")
                     .arg(m_polygonPoints.size());
        
        if (m_app && m_app->getMainWindow()) {
            QWidget* mainWin = dynamic_cast<QWidget*>(m_app->getMainWindow());
            QMessageBox::warning(mainWin, QString("Insufficient Vertices"), msg);
        }
        return;
    }
    
    if (!m_segmentationPolyline || !m_segmentationPolyline->isClosed()) {
        QWidget* mainWin = dynamic_cast<QWidget*>(m_app->getMainWindow());
        QMessageBox::warning(mainWin, QString("Polygon Not Closed"), 
                           QString("Please close the polygon first (right-click)."));
        return;
    }
    
    // 计算选中的点
    std::vector<unsigned> selectedPoints = getPointsInPolygon();
    
    if (selectedPoints.empty()) {
        QString msg = "No points found inside the polygon.\n"
                     "Try adjusting the polygon or check if the point cloud is visible.";
        
        if (m_app && m_app->getMainWindow()) {
            QWidget* mainWin = dynamic_cast<QWidget*>(m_app->getMainWindow());
            QMessageBox::information(mainWin, QString("No Points Selected"), msg);
        }
        return;
    }
    
    ccLog::Print(QString("[PolygonSFEditor] Found %1 points inside polygon")
                .arg(selectedPoints.size()));
    
    // 显示数值输入对话框
    showValueInputDialog(selectedPoints);
}

void PolygonSFEditorTool::onCancel()
{
    stop(false);
}

void PolygonSFEditorTool::onReset()
{
    if (m_segmentationPolyline) {
        ccPointCloud* vertices = dynamic_cast<ccPointCloud*>(m_segmentationPolyline->getAssociatedCloud());
        if (vertices) {
            vertices->clear();
        }
        m_segmentationPolyline->clear();
    }
    
    m_polygonPoints.clear();
    m_currentMousePos = QPointF();
    updateDisplay();
    updateButtonStates();
    ccLog::Print("[PolygonSFEditor] Polygon reset");
}

std::vector<unsigned> PolygonSFEditorTool::getPointsInPolygon()
{
    std::vector<unsigned> selectedPoints;
    
    if (m_polygonPoints.size() < 3 || !m_cloud || !m_segmentationPolyline) {
        return selectedPoints;
    }
    
    // 获取GL相机参数
    ccGLCameraParameters camera;
    m_glWindow->getGLCameraParameters(camera);
    
    const double half_w = camera.viewport[2] / 2.0;
    const double half_h = camera.viewport[3] / 2.0;
    
    // 使用set避免重复
    std::set<unsigned> selectedSet;
    
    // 遍历所有点
    unsigned cloudSize = m_cloud->size();
    for (unsigned i = 0; i < cloudSize; ++i) {
        const CCVector3* point3D = m_cloud->getPoint(i);
        
        // 3D点投影到屏幕坐标
        CCVector3d projected;
        bool pointInFrustum = false;
        camera.project(*point3D, projected, &pointInFrustum);
        
        if (pointInFrustum) {
            // 转换为多边形坐标系统
            CCVector2 screenPoint(static_cast<PointCoordinateType>(projected.x - half_w),
                                 static_cast<PointCoordinateType>(projected.y - half_h));
            
            // 判断是否在多边形内
            if (CCCoreLib::ManualSegmentationTools::isPointInsidePoly(screenPoint, m_segmentationPolyline)) {
                selectedSet.insert(i);
            }
        }
    }
    
    // 转换为vector
    selectedPoints.assign(selectedSet.begin(), selectedSet.end());
    
    return selectedPoints;
}

bool PolygonSFEditorTool::isPointInPolygon(const QPointF& point)
{
    // 这个方法现在由 CCCoreLib::ManualSegmentationTools::isPointInsidePoly 处理
    // 保留接口以备将来需要
    return false;
}

void PolygonSFEditorTool::showValueInputDialog(const std::vector<unsigned>& selectedPoints)
{
    if (!m_cloud || selectedPoints.empty()) return;
    
    // 获取SF信息
    CCCoreLib::ScalarField* sf = m_cloud->getScalarField(m_sfIndex);
    if (!sf) return;
    
    QString sfName = QString::fromStdString(sf->getName());
    ScalarType minVal, maxVal;
    sf->computeMinAndMax();
    minVal = sf->getMin();
    maxVal = sf->getMax();
    
    // 创建输入对话框
    QWidget* parentWin = dynamic_cast<QWidget*>(m_app ? m_app->getMainWindow() : nullptr);
    SFValueInputDialog dialog(sfName, selectedPoints.size(), minVal, maxVal, parentWin);
    
    if (dialog.exec() == QDialog::Accepted) {
        double newValue = dialog.getNewValue();
        applySFModification(selectedPoints, newValue);
        
        // 成功后关闭工具
        stop(true);
    }
}

void PolygonSFEditorTool::applySFModification(const std::vector<unsigned>& pointIndices, double newValue)
{
    if (!m_cloud || pointIndices.empty()) return;
    
    CCCoreLib::ScalarField* sf = m_cloud->getScalarField(m_sfIndex);
    if (!sf) return;
    
    // 修改SF值
    for (unsigned index : pointIndices) {
        sf->setValue(index, static_cast<ScalarType>(newValue));
    }
    
    // 重新计算SF统计
    sf->computeMinAndMax();
    
    // 刷新显示
    m_cloud->setCurrentDisplayedScalarField(m_sfIndex);
    m_cloud->showSF(true);
    
    // 通知主程序刷新
    if (m_app) {
        m_app->updateUI();
        if (m_app->getActiveGLWindow()) {
            m_app->getActiveGLWindow()->redraw(true);
        }
    }
    
    ccLog::Print(QString("[PolygonSFEditor] Modified %1 points with value %2")
                .arg(pointIndices.size())
                .arg(newValue));
}