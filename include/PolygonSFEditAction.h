#ifndef POLYGON_SF_EDIT_ACTION_HEADER
#define POLYGON_SF_EDIT_ACTION_HEADER

#include <QObject>
#include <QPoint>
#include <vector>

class ccMainAppInterface;
class ccPointCloud;
class ccGLWindow;
class ccPolyline;
class QMouseEvent;

class PolygonSFEditAction : public QObject
{
    Q_OBJECT

public:
    PolygonSFEditAction(ccMainAppInterface* app, ccPointCloud* cloud, int sfIndex, QObject* parent = nullptr);
    ~PolygonSFEditAction();

    bool start();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void handleMousePress(QMouseEvent* event);
    void handleMouseMove(QMouseEvent* event);
    void updatePolyline(int x, int y, bool addPoint);
    void processPolygonSelection();
    void cleanup();

private:
    ccMainAppInterface* m_app;
    ccPointCloud* m_cloud;
    int m_sfIndex;
    ccGLWindow* m_window;
    ccPolyline* m_polyline;
    
    std::vector<QPoint> m_polygon2D;
    bool m_polygonComplete;
};

namespace PolygonSFEdit
{
    void performAction(ccMainAppInterface* appInterface, ccPointCloud* cloud, int sfIndex);
}

#endif