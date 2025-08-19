option( PLUGIN_STANDARD_QPOLYGONSF_EDITOR "Install qPolygonSFEditor plugin" ON )

if( PLUGIN_STANDARD_QPOLYGONSF_EDITOR )
    
    project( QPOLYGONSF_EDITOR_PLUGIN )
    
    AddPlugin( NAME ${PROJECT_NAME} )
    
    add_subdirectory( include )
    add_subdirectory( src )
    add_subdirectory( ui )
    
endif()
