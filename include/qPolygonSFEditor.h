#ifndef Q_POLYGON_SF_EDITOR_PLUGIN_HEADER
#define Q_POLYGON_SF_EDITOR_PLUGIN_HEADER

#include "ccStdPluginInterface.h"
#include <QDebug>
#include <ccHObject.h>

class QAction;
class PolygonSFEditorTool;
class ccPointCloud;
class ccHObject;

//! 多边形SF编辑插件主类
class qPolygonSFEditor : public QObject, public ccStdPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(ccStdPluginInterface)
    Q_PLUGIN_METADATA(IID "cccorp.cloudcompare.plugin.qPolygonSFEditor" FILE "../info.json")

public:
    //! 构造函数
    explicit qPolygonSFEditor(QObject* parent = nullptr);

    //! 析构函数
    ~qPolygonSFEditor() override = default;

    // 继承自ccStdPluginInterface的接口
    virtual QString getName() const override { return "Polygon SF Editor"; }
    virtual QString getDescription() const override;
    virtual QIcon getIcon() const override;
    
    virtual void onNewSelection(const ccHObject::Container& selectedEntities) override;
    virtual QList<QAction*> getActions() override;

protected slots:
    //! 启动多边形SF编辑工具
    void doStartPolygonSFEditor();
    
    //! 工具完成回调
    void onToolFinished(bool accepted);

protected:
    //! 检查当前选择是否有效
    bool checkSelection();
    
    //! 获取选中的点云和SF索引
    bool getSelectedCloudAndSF(ccPointCloud*& cloud, int& sfIndex);

private:
    //! 主要动作
    QAction* m_action;
    
    //! 工具实例
    PolygonSFEditorTool* m_tool;
};

#endif // Q_POLYGON_SF_EDITOR_PLUGIN_HEADER
