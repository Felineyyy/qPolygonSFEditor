target_sources( ${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/qPolygonSFEditor.h
        ${CMAKE_CURRENT_LIST_DIR}/PolygonSFEditorTool.h
        ${CMAKE_CURRENT_LIST_DIR}/SFValueInputDialog.h
)

target_include_directories( ${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)