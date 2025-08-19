#ifndef POLYGON_SF_EDITOR_TOOL_HEADER
#define POLYGON_SF_EDITOR_TOOL_HEADER

#include <ccGLWindowInterface.h>
#include <ccGLWindow.h>
#include <QObject>
#include <QPointF>
#include <vector>

// 前向声明
class ccPointCloud;
class ccMainAppInterface;
class ccPolyline;
class QToolBar;
class QToolButton;
class QMouseEvent;
class QKeyEvent;
struct ccGLCameraParameters;

//! 多边形SF编辑工具类
class PolygonSFEditorTool : public QObject
{
    Q_OBJECT

public:
    //! 构造函数
    PolygonSFEditorTool(ccPointCloud* cloud, int sfIndex, ccMainAppInterface* app);
    
    //! 析构函数
    virtual ~PolygonSFEditorTool();

    //! 启动工具
    bool start();
    
    //! 停止工具
    void stop(bool accepted = false);

    //! 绘制方法（由ccPolyline自动处理，保留接口）
    void draw(ccGLWindow* win);
    
    //! 重写QObject的事件过滤器
    virtual bool eventFilter(QObject* obj, QEvent* event) override;

signals:
    //! 工具完成信号
    void finished(bool accepted);

private slots:
    void onAccept();    // 确认按钮
    void onCancel();    // 取消按钮
    void onReset();     // 重置按钮

private:
    // 数据成员
    ccPointCloud* m_cloud;                 //!< 目标点云
    int m_sfIndex;                         //!< 要编辑的SF索引
    ccMainAppInterface* m_app;             //!< 主应用接口
    ccGLWindow* m_glWindow;                //!< GL窗口
    
    // 多边形顶点（屏幕坐标）
    std::vector<QPointF> m_polygonPoints;  //!< 多边形顶点列表（屏幕坐标）
    QPointF m_currentMousePos;             //!< 当前鼠标位置
    
    // UI元素
    QToolBar* m_toolBar;                   //!< 工具栏
    QToolButton* m_acceptButton;           //!< 确认按钮
    QToolButton* m_cancelButton;           //!< 取消按钮
    QToolButton* m_resetButton;            //!< 重置按钮
    
    // 状态
    bool m_isActive;                       //!< 工具是否激活
    int m_previousInteractionMode;         //!< 之前的交互模式
    
    // 显示对象
    ccPolyline* m_segmentationPolyline;    //!< 分割多边形线
    
    // 核心方法
    void setupToolBar();                   //!< 设置工具栏
    void cleanupToolBar();                 //!< 清理工具栏
    void updateDisplay();                  //!< 更新显示
    void updateButtonStates();             //!< 更新按钮状态
    
    // 事件处理
    void handleMousePress(QMouseEvent* event);  //!< 处理鼠标按下
    void handleMouseMove(QMouseEvent* event);   //!< 处理鼠标移动
    void handleKeyPress(QKeyEvent* event);      //!< 处理键盘按下
    
    // 多边形操作
    void addPointToPolyline(int x, int y);      //!< 添加点到多边形
    void removeLastPoint();                     //!< 移除最后一个点
    void closePolyline();                       //!< 关闭多边形
    
    // 绘制方法（由ccPolyline处理，保留接口）
    void drawPolygon2D(ccGLWindow* win);        //!< 2D绘制多边形
    void drawVertices2D(ccGLWindow* win);       //!< 2D绘制顶点
    
    // 几何计算
    std::vector<unsigned> getPointsInPolygon();    //!< 获取多边形内的点
    bool isPointInPolygon(const QPointF& point);   //!< 判断点是否在多边形内
    
    // SF操作
    void showValueInputDialog(const std::vector<unsigned>& selectedPoints);  //!< 显示值输入对话框
    void applySFModification(const std::vector<unsigned>& pointIndices, double newValue); //!< 应用SF修改
};

#endif // POLYGON_SF_EDITOR_TOOL_HEADER