#include "qPolygonSFEditor.h"
#include "PolygonSFEditAction.h"

#include <QtGui>
#include <QMessageBox>

#include <ccMainAppInterface.h>
#include <ccPointCloud.h>
#include <ccHObjectCaster.h>

qPolygonSFEditor::qPolygonSFEditor( QObject *parent )
    : QObject( parent )
    , ccStdPluginInterface( ":/CC/plugin/qPolygonSFEditor/info.json" )
    , m_action( nullptr )
{
}

void qPolygonSFEditor::onNewSelection( const ccHObject::Container &selectedEntities )
{
    if ( m_action == nullptr )
        return;
    
    bool enabled = checkSelection();
    m_action->setEnabled( enabled );
}

QList<QAction *> qPolygonSFEditor::getActions()
{
    if ( !m_action )
    {
        m_action = new QAction( getName(), this );
        m_action->setToolTip( getDescription() );
        m_action->setIcon( getIcon() );
        connect( m_action, &QAction::triggered, this, &qPolygonSFEditor::doAction );
    }
    return { m_action };
}

void qPolygonSFEditor::doAction()
{
    if ( !m_app )
        return;
    
    const ccHObject::Container& selectedEntities = m_app->getSelectedEntities();
    ccPointCloud* cloud = nullptr;
    
    for ( ccHObject* entity : selectedEntities )
    {
        cloud = ccHObjectCaster::ToPointCloud( entity );
        if ( cloud && cloud->getNumberOfScalarFields() > 0 )
            break;
    }
    
    if ( !cloud )
    {
        m_app->dispToConsole( "[qPolygonSFEditor] Please select a point cloud with scalar fields!", 
                             ccMainAppInterface::ERR_CONSOLE_MESSAGE );
        return;
    }
    
    int sfIndex = cloud->getCurrentDisplayedScalarFieldIndex();
    if ( sfIndex < 0 )
    {
        sfIndex = 0;
        cloud->setCurrentDisplayedScalarField( sfIndex );
        cloud->showSF( true );
    }
    
    PolygonSFEdit::performAction( m_app, cloud, sfIndex );
}

bool qPolygonSFEditor::checkSelection()
{
    if ( !m_app )
        return false;
    
    const ccHObject::Container& selectedEntities = m_app->getSelectedEntities();
    
    for ( ccHObject* entity : selectedEntities )
    {
        ccPointCloud* cloud = ccHObjectCaster::ToPointCloud( entity );
        if ( cloud && cloud->getNumberOfScalarFields() > 0 )
            return true;
    }
    
    return false;
}
