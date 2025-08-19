#include "qPolygonSFEditor.h"
#include "PolygonSFEditorTool.h"

// Qt
#include <QAction>
#include <QApplication>
#include <QMessageBox>
#include <QStyle>
#include <QMainWindow>
#include <QDebug>  // 添加这行

// CC includes
#include <ccPointCloud.h>
#include <ccHObjectCaster.h>
#include <ccMainAppInterface.h>
#include <ccLog.h>

qPolygonSFEditor::qPolygonSFEditor(QObject* parent)
    : QObject(parent)
    , ccStdPluginInterface(":/CC/plugin/qPolygonSFEditor/info.json")  // 改回使用资源文件
    , m_action(nullptr)
    , m_tool(nullptr)
{
}


QString qPolygonSFEditor::getDescription() const
{
    qDebug() << "qPolygonSFEditor::getDescription() called";
    return "Interactive polygon-based scalar field editing tool\n"
           "Select points using a polygon and modify their scalar field values";
}

QIcon qPolygonSFEditor::getIcon() const
{
    qDebug() << "qPolygonSFEditor::getIcon() called";
    // 返回空图标，避免资源问题
    return QIcon();
}

void qPolygonSFEditor::onNewSelection(const ccHObject::Container& selectedEntities)
{
    qDebug() << "qPolygonSFEditor::onNewSelection() called";
    if (m_action) {
        // 检查选择对象，启用/禁用工具
        bool validSelection = checkSelection();
        m_action->setEnabled(validSelection && !m_tool);
        
        // 更新工具提示
        if (validSelection) {
            ccPointCloud* cloud = nullptr;
            int sfIndex = -1;
            if (getSelectedCloudAndSF(cloud, sfIndex)) {
                CCCoreLib::ScalarField* sf = cloud->getScalarField(sfIndex);
                QString sfName = sf ? QString::fromStdString(sf->getName()) : "Unknown";
                m_action->setToolTip(QString("Edit scalar field '%1' using polygon selection").arg(sfName));
            }
        } else {
            m_action->setToolTip("Select a point cloud with an active scalar field");
        }
    }
}

QList<QAction*> qPolygonSFEditor::getActions()
{
    qDebug() << "qPolygonSFEditor::getActions() called";
    if (!m_action) {
        m_action = new QAction(getName(), this);
        m_action->setToolTip(getDescription());
        m_action->setIcon(getIcon());
        m_action->setEnabled(false); // 默认禁用，直到有合适的选择
        
        // 连接槽函数
        connect(m_action, &QAction::triggered, this, &qPolygonSFEditor::doStartPolygonSFEditor);
    }

    return QList<QAction*>{ m_action };
}

void qPolygonSFEditor::doStartPolygonSFEditor()
{
    // 防止重复启动
    if (m_tool) {
        ccLog::Warning("[PolygonSFEditor] Tool is already active");
        return;
    }
    
    // 检查选择
    ccPointCloud* cloud = nullptr;
    int sfIndex = -1;
    if (!getSelectedCloudAndSF(cloud, sfIndex)) {
        return;
    }
    
    // 显示操作说明
    ccLog::Print("[PolygonSFEditor] Starting polygon selection tool");
    ccLog::Print("[PolygonSFEditor] Instructions:");
    ccLog::Print("  - Left click: add polygon vertex");
    ccLog::Print("  - Right click: close polygon");
    ccLog::Print("  - Enter: confirm selection and input new value");
    ccLog::Print("  - Escape: cancel operation");
    ccLog::Print("  - Delete: reset polygon");
    
    // 创建并启动工具
    m_tool = new PolygonSFEditorTool(cloud, sfIndex, m_app);
    connect(m_tool, &PolygonSFEditorTool::finished, this, &qPolygonSFEditor::onToolFinished);
    
    if (!m_tool->start()) {
        delete m_tool;
        m_tool = nullptr;
        ccLog::Error("[PolygonSFEditor] Failed to start polygon selection tool");
        return;
    }
    
    // 禁用动作直到工具完成
    if (m_action) {
        m_action->setEnabled(false);
    }
}

void qPolygonSFEditor::onToolFinished(bool accepted)
{
    if (m_tool) {
        m_tool->deleteLater();
        m_tool = nullptr;
    }
    
    // 重新启用动作
    if (m_action) {
        m_action->setEnabled(checkSelection());
    }
    
    if (accepted) {
        ccLog::Print("[PolygonSFEditor] Scalar field modification completed successfully");
        
        // 刷新实体属性显示
        if (m_app) {
            m_app->updateUI();
            m_app->refreshAll();
        }
    } else {
        ccLog::Print("[PolygonSFEditor] Operation cancelled");
    }
}

bool qPolygonSFEditor::checkSelection()
{
    ccPointCloud* cloud = nullptr;
    int sfIndex = -1;
    return getSelectedCloudAndSF(cloud, sfIndex);
}

bool qPolygonSFEditor::getSelectedCloudAndSF(ccPointCloud*& cloud, int& sfIndex)
{
    cloud = nullptr;
    sfIndex = -1;
    
    if (!m_app) {
        return false;
    }

    const ccHObject::Container& selectedEntities = m_app->getSelectedEntities();
    if (selectedEntities.empty()) {
        return false;
    }
    
    // 查找第一个点云
    for (ccHObject* entity : selectedEntities) {
        ccPointCloud* pc = ccHObjectCaster::ToPointCloud(entity);
        if (pc) {
            cloud = pc;
            break;
        }
    }
    
    if (!cloud) {
        return false;
    }
    
    // 检查是否有标量场
    if (cloud->getNumberOfScalarFields() == 0) {
        if (m_app && m_app->getMainWindow()) {
            QMessageBox::information(m_app->getMainWindow(), 
                                   QString("No Scalar Fields"), 
                                   QString("The selected point cloud has no scalar fields.\n"
                                          "Please add a scalar field first."));
        }
        return false;
    }
    
    // 获取当前显示的SF
    sfIndex = cloud->getCurrentDisplayedScalarFieldIndex();
    if (sfIndex < 0) {
        // 如果没有显示的SF，尝试使用第一个
        if (cloud->getNumberOfScalarFields() > 0) {
            sfIndex = 0;
            cloud->setCurrentDisplayedScalarField(sfIndex);
            cloud->showSF(true);
        } else {
            return false;
        }
    }
    
    return true;
}