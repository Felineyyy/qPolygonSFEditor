#ifndef Q_POLYGON_SF_EDITOR_PLUGIN_HEADER
#define Q_POLYGON_SF_EDITOR_PLUGIN_HEADER

#include "ccStdPluginInterface.h"

class QAction;
class ccPointCloud;

class qPolygonSFEditor : public QObject, public ccStdPluginInterface
{
    Q_OBJECT
    Q_INTERFACES( ccPluginInterface ccStdPluginInterface )
    Q_PLUGIN_METADATA( IID "cccorp.cloudcompare.plugin.qPolygonSFEditor" FILE "../info.json" )

public:
    explicit qPolygonSFEditor( QObject *parent = nullptr );
    ~qPolygonSFEditor() override = default;

    void onNewSelection( const ccHObject::Container &selectedEntities ) override;
    QList<QAction *> getActions() override;

private:
    void doAction();
    bool checkSelection();

private:
    QAction* m_action;
};

#endif