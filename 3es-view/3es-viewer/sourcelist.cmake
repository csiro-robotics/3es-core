
list(APPEND PUBLIC_HEADERS
  # General headers
  3esboundsculler.h
  3esviewer.h
  3esedleffect.h
  3esfboeffect.h
  camera/3escamera.h
  camera/3escontroller.h
  camera/3esfly.h
  painter/3esshapecache.h
  painter/3esshapepainter.h
  painter/3esarrow.h
  painter/3esbox.h
  painter/3escapsule.h
  painter/3escylinder.h
  painter/3essphere.h
  mesh/3esconverter.h
  shaders/3esedl.h
)

list(APPEND SOURCES
  3esboundsculler.cpp
  3esviewer.cpp
  3esedleffect.cpp
  3esfboeffect.cpp
  camera/3escamera.cpp
  camera/3escontroller.cpp
  camera/3esfly.cpp
  painter/3esshapecache.cpp
  painter/3esshapepainter.cpp
  painter/3esarrow.cpp
  painter/3esbox.cpp
  painter/3escapsule.cpp
  painter/3escylinder.cpp
  painter/3essphere.cpp
  mesh/3esconverter.cpp
  shaders/3esedl.cpp
  shaders/3esedl.frag
  shaders/3esedl.vert
)

list(APPEND PRIVATE_SOURCES
)
