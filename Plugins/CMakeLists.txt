message(STATUS "Building plugins with radium libs : ${RADIUM_LIBRARIES} ")

set( CMAKE_DEBUG_POSTFIX "")

#add_subdirectory( Animation )
#add_subdirectory( Skinning )
#add_subdirectory( MeshFeatureTracking )
#press Q to show/hide the QDockWidgetadd_subdirectory( MeshPaint )
add_subdirectory( CameraManip )
