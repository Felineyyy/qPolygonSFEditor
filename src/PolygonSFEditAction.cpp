#include "PolygonSFEditAction.h"

// Qt
#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>
#include <QMouseEvent>
#include <QKeyEvent>

// CC includes
#include <ccMainAppInterface.h>
#include <ccGLWindow.h>
#include <ccPointCloud.h>
#include <ccPolyline.h>
#include <ManualSegmentationTools.h>

// System
#include <set>

// Track the current active action
static PolygonSFEditAction* s_currentAction = nullptr;

PolygonSFEditAction::PolygonSFEditAction(ccMainAppInterface* app, ccPointCloud* cloud, int sfIndex, QObject* parent)
    : QObject(parent)
    , m_app(app)
    , m_cloud(cloud)
    , m_sfIndex(sfIndex)
    , m_window(nullptr)
    , m_polyline(nullptr)
    , m_polygonComplete(false)
{
}

PolygonSFEditAction::~PolygonSFEditAction()
{
    cleanup();
}

void PolygonSFEditAction::cleanup()
{
    if (m_polyline && m_window)
    {
        m_window->removeFromOwnDB(m_polyline);
        delete m_polyline;
        m_polyline = nullptr;
    }
    
    if (m_window)
    {
        // Remove event filter
        m_window->removeEventFilter(this);
        
        // Restore normal interaction
        m_window->setInteractionMode(ccGLWindow::MODE_TRANSFORM_CAMERA);
        m_window->setPickingMode(ccGLWindow::DEFAULT_PICKING);
        m_window->displayNewMessage("", ccGLWindow::UPPER_CENTER_MESSAGE, false, 0, ccGLWindow::MANUAL_SEGMENTATION_MESSAGE);
        m_window->redraw();
        m_window = nullptr;
    }
    
    if (s_currentAction == this)
        s_currentAction = nullptr;
}

bool PolygonSFEditAction::start()
{
    if (!m_app || !m_cloud)
        return false;
    
    // Check if another action is active
    if (s_currentAction != nullptr)
    {
        m_app->dispToConsole("[PolygonSFEditor] Another action is already active!", 
                            ccMainAppInterface::WRN_CONSOLE_MESSAGE);
        return false;
    }
    
    // Get GL window
    ccGLWindowInterface* glWin = m_app->getActiveGLWindow();
    if (!glWin)
    {
        m_app->dispToConsole("[PolygonSFEditor] No active 3D view!", ccMainAppInterface::ERR_CONSOLE_MESSAGE);
        return false;
    }
    
    m_window = static_cast<ccGLWindow*>(glWin);
    
    // Create polyline for visualization
    ccPointCloud* vertices = new ccPointCloud("vertices");
    m_polyline = new ccPolyline(vertices);
    m_polyline->setForeground(true);
    m_polyline->setColor(ccColor::green);
    m_polyline->showColors(true);
    m_polyline->set2DMode(true);
    m_polyline->setDisplay(m_window);
    
    m_window->addToOwnDB(m_polyline);
    
    // Set interaction mode
    m_window->setInteractionMode(ccGLWindow::INTERACT_SEND_ALL_SIGNALS);
    m_window->setPickingMode(ccGLWindow::NO_PICKING);
    
    // Install event filter
    m_window->installEventFilter(this);
    
    // Set as current action
    s_currentAction = this;
    
    // Display instructions
    m_window->displayNewMessage("Left click: add point | Right click: close | ESC: cancel", 
                                ccGLWindow::UPPER_CENTER_MESSAGE, 
                                true, 3600, 
                                ccGLWindow::MANUAL_SEGMENTATION_MESSAGE);
    
    m_app->dispToConsole("[PolygonSFEditor] Started - Left: add vertex, Right: close polygon, ESC: cancel", 
                         ccMainAppInterface::STD_CONSOLE_MESSAGE);
    m_app->dispToConsole("[PolygonSFEditor] Tip: Press 'P' to quickly activate this tool", 
                         ccMainAppInterface::STD_CONSOLE_MESSAGE);
    
    return true;
}

bool PolygonSFEditAction::eventFilter(QObject* obj, QEvent* event)
{
    if (obj != m_window)
        return QObject::eventFilter(obj, event);
    
    switch (event->type())
    {
        case QEvent::MouseButtonPress:
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            handleMousePress(mouseEvent);
            return true; // Event consumed
        }
        case QEvent::MouseMove:
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            handleMouseMove(mouseEvent);
            return false; // Let event pass through for display updates
        }
        case QEvent::KeyPress:
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Escape)
            {
                m_app->dispToConsole("[PolygonSFEditor] Cancelled by user", 
                                    ccMainAppInterface::STD_CONSOLE_MESSAGE);
                cleanup();
                deleteLater();
                return true;
            }
            return false;
        }
        default:
            break;
    }
    
    return QObject::eventFilter(obj, event);
}

void PolygonSFEditAction::handleMousePress(QMouseEvent* event)
{
    if (!m_window || m_polygonComplete)
        return;
    
    if (event->button() == Qt::LeftButton)
    {
        // Add point to polygon
        m_polygon2D.push_back(QPoint(event->x(), event->y()));
        updatePolyline(event->x(), event->y(), true);
        
        m_app->dispToConsole(QString("[PolygonSFEditor] Added vertex %1 at (%2,%3)")
                            .arg(m_polygon2D.size()).arg(event->x()).arg(event->y()), 
                            ccMainAppInterface::STD_CONSOLE_MESSAGE);
    }
    else if (event->button() == Qt::RightButton)
    {
        // Close polygon
        if (m_polygon2D.size() >= 3)
        {
            m_polygonComplete = true;
            
            // Close the polyline properly
            if (m_polyline)
            {
                m_polyline->setClosed(true);
                
                // Remove any preview vertices
                ccPointCloud* vertices = dynamic_cast<ccPointCloud*>(m_polyline->getAssociatedCloud());
                if (vertices)
                {
                    // Keep only the actual polygon points
                    vertices->resize(m_polygon2D.size());
                    
                    // Rebuild polyline with just the actual points
                    m_polyline->clear();
                    for (unsigned i = 0; i < vertices->size(); ++i)
                        m_polyline->addPointIndex(i);
                }
            }
            
            m_window->redraw();
            
            // Process the selection
            processPolygonSelection();
        }
        else
        {
            m_app->dispToConsole("[PolygonSFEditor] Need at least 3 points to close polygon!", 
                                ccMainAppInterface::WRN_CONSOLE_MESSAGE);
        }
    }
    else if (event->button() == Qt::MiddleButton)
    {
        // Cancel
        m_app->dispToConsole("[PolygonSFEditor] Cancelled by user", 
                            ccMainAppInterface::STD_CONSOLE_MESSAGE);
        cleanup();
        deleteLater();
    }
}

void PolygonSFEditAction::handleMouseMove(QMouseEvent* event)
{
    if (!m_window || m_polygonComplete || m_polygon2D.empty())
        return;
    
    // Update preview line
    updatePolyline(event->x(), event->y(), false);
}

void PolygonSFEditAction::updatePolyline(int x, int y, bool addPoint)
{
    if (!m_polyline)
        return;
    
    ccPointCloud* vertices = dynamic_cast<ccPointCloud*>(m_polyline->getAssociatedCloud());
    if (!vertices)
        return;
    
    // Convert to GL coordinates
    QPointF pos2D = m_window->toCenteredGLCoordinates(x, y);
    CCVector3 P(static_cast<PointCoordinateType>(pos2D.x()),
                static_cast<PointCoordinateType>(pos2D.y()),
                0);
    
    if (addPoint)
    {
        // Add new vertex to the polygon
        if (m_polygon2D.size() == 1)  // First point
        {
            vertices->reserve(2);
            vertices->addPoint(P);     // Actual first point
            vertices->addPoint(P);     // Preview point (will move with mouse)
            
            m_polyline->clear();
            m_polyline->addPointIndex(0, 2);
        }
        else
        {
            // Replace the preview point with actual point
            // The preview point is always the last one
            if (vertices->size() > 0)
            {
                unsigned lastIdx = vertices->size() - 1;
                CCVector3* lastP = const_cast<CCVector3*>(vertices->getPointPersistentPtr(lastIdx));
                *lastP = P;
            }
            
            // Add new preview point
            vertices->addPoint(P);
            
            // Rebuild polyline to show the closing preview
            m_polyline->clear();
            
            // Add all actual vertices
            for (size_t i = 0; i < m_polygon2D.size(); ++i)
            {
                m_polyline->addPointIndex(i);
            }
            
            // Add preview vertex
            m_polyline->addPointIndex(vertices->size() - 1);
            
            // Add closing line back to first point (to show how polygon will close)
            if (m_polygon2D.size() >= 2)
            {
                m_polyline->addPointIndex(0);
            }
        }
    }
    else
    {
        // Just update the preview point position (mouse move)
        if (vertices->size() > m_polygon2D.size() && vertices->size() > 0)
        {
            // Update the last vertex (preview point)
            unsigned lastIdx = vertices->size() - 1;
            CCVector3* lastP = const_cast<CCVector3*>(vertices->getPointPersistentPtr(lastIdx));
            *lastP = P;
            
            // Rebuild the polyline to ensure the closing line is shown
            if (m_polygon2D.size() >= 1)
            {
                m_polyline->clear();
                
                // Add all actual vertices
                for (size_t i = 0; i < m_polygon2D.size(); ++i)
                {
                    m_polyline->addPointIndex(i);
                }
                
                // Add preview vertex
                m_polyline->addPointIndex(lastIdx);
                
                // Add closing line back to first point if we have at least 2 actual points
                // This shows how the polygon will look when closed
                if (m_polygon2D.size() >= 2)
                {
                    m_polyline->addPointIndex(0);
                }
            }
        }
    }
    
    m_window->redraw();
}

void PolygonSFEditAction::processPolygonSelection()
{
    if (!m_cloud || !m_polyline || m_polygon2D.size() < 3)
    {
        cleanup();
        deleteLater();
        return;
    }
    
    m_app->dispToConsole("[PolygonSFEditor] Processing polygon selection...", 
                        ccMainAppInterface::STD_CONSOLE_MESSAGE);
    
    // Get camera parameters for projection
    ccGLCameraParameters camera;
    m_window->getGLCameraParameters(camera);
    
    const double half_w = camera.viewport[2] / 2.0;
    const double half_h = camera.viewport[3] / 2.0;
    
    // Find points inside polygon
    std::set<unsigned> selectedPoints;
    unsigned cloudSize = m_cloud->size();
    
    for (unsigned i = 0; i < cloudSize; ++i)
    {
        const CCVector3* point3D = m_cloud->getPoint(i);
        
        // Project 3D point to screen
        CCVector3d projected;
        bool pointInFrustum = false;
        camera.project(*point3D, projected, &pointInFrustum);
        
        if (pointInFrustum)
        {
            // Convert to centered GL coordinates (same as polyline)
            CCVector2 screenPoint(static_cast<PointCoordinateType>(projected.x - half_w),
                                 static_cast<PointCoordinateType>(projected.y - half_h));
            
            // Check if point is inside polygon
            if (CCCoreLib::ManualSegmentationTools::isPointInsidePoly(screenPoint, m_polyline))
            {
                selectedPoints.insert(i);
            }
        }
    }
    
    m_app->dispToConsole(QString("[PolygonSFEditor] Found %1 points inside polygon").arg(selectedPoints.size()), 
                        ccMainAppInterface::STD_CONSOLE_MESSAGE);
    
    if (selectedPoints.empty())
    {
        QMessageBox::warning(nullptr, "No Points Selected", 
                           "No points found inside the polygon.\nTry adjusting the view or drawing a larger polygon.");
        cleanup();
        deleteLater();
        return;
    }
    
    // Get scalar field
    CCCoreLib::ScalarField* sf = m_cloud->getScalarField(m_sfIndex);
    if (!sf)
    {
        m_app->dispToConsole("[PolygonSFEditor] Invalid scalar field!", ccMainAppInterface::ERR_CONSOLE_MESSAGE);
        cleanup();
        deleteLater();
        return;
    }
    
    QString sfName = QString::fromStdString(sf->getName());
    
    // Get current min/max for reference
    sf->computeMinAndMax();
    ScalarType currentMin = sf->getMin();
    ScalarType currentMax = sf->getMax();
    
    // Get new value from user
    bool ok;
    double newValue = QInputDialog::getDouble(nullptr, 
        "Set Scalar Field Value",
        QString("Enter new value for %1 points in scalar field '%2':\n"
                "(Current range: %3 to %4)")
            .arg(selectedPoints.size())
            .arg(sfName)
            .arg(currentMin)
            .arg(currentMax),
        0.0, -1e9, 1e9, 6, &ok);
    
    if (ok)
    {
        // Apply modification
        for (unsigned idx : selectedPoints)
        {
            sf->setValue(idx, static_cast<ScalarType>(newValue));
        }
        
        // Update scalar field display
        sf->computeMinAndMax();
        m_cloud->setCurrentDisplayedScalarField(m_sfIndex);
        m_cloud->showSF(true);
        
        // Refresh display
        m_app->updateUI();
        if (m_window)
            m_window->redraw();
        
        m_app->dispToConsole(QString("[PolygonSFEditor] Successfully modified %1 points with value %2")
                           .arg(selectedPoints.size()).arg(newValue), 
                           ccMainAppInterface::STD_CONSOLE_MESSAGE);
        
        // Show new range
        ScalarType newMin = sf->getMin();
        ScalarType newMax = sf->getMax();
        m_app->dispToConsole(QString("[PolygonSFEditor] New SF range: [%1, %2]")
                           .arg(newMin).arg(newMax), 
                           ccMainAppInterface::STD_CONSOLE_MESSAGE);
    }
    
    // Clean up
    cleanup();
    deleteLater();
}

namespace PolygonSFEdit
{
    void performAction(ccMainAppInterface* appInterface, ccPointCloud* cloud, int sfIndex)
    {
        if (!appInterface || !cloud)
            return;
        
        // Create and start the action
        // Note: The action will delete itself after completion
        PolygonSFEditAction* action = new PolygonSFEditAction(appInterface, cloud, sfIndex);
        if (!action->start())
        {
            delete action;
            appInterface->dispToConsole("[PolygonSFEditor] Failed to start polygon editor", 
                                      ccMainAppInterface::ERR_CONSOLE_MESSAGE);
        }
    }
}