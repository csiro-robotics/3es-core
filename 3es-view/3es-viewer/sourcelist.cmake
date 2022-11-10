
list(APPEND PUBLIC_HEADERS
  # General headers
  3esboundsculler.h
  3esedleffect.h
  3esfboeffect.h
  3esframestamp.h
  3esviewablewindow.h
  camera/3escamera.h
  camera/3escontroller.h
  camera/3esfly.h
  handler/3esmessage.h
  handler/3esshape.h
  painter/3esshapecache.h
  painter/3esshapepainter.h
  painter/3esarrow.h
  painter/3esbox.h
  painter/3escapsule.h
  painter/3escylinder.h
  painter/3esplane.h
  painter/3espose.h
  painter/3essphere.h
  painter/3esstar.h
  mesh/3esconverter.h
  shaders/3esedl.h
  util/3esresourcelist.h
)

list(APPEND SOURCES
  3esboundsculler.cpp
  3esedleffect.cpp
  3esfboeffect.cpp
  camera/3escamera.cpp
  camera/3escontroller.cpp
  camera/3esfly.cpp
  handler/3esmessage.cpp
  handler/3esshape.cpp
  painter/3esshapecache.cpp
  painter/3esshapepainter.cpp
  painter/3esarrow.cpp
  painter/3esbox.cpp
  painter/3escapsule.cpp
  painter/3escylinder.cpp
  painter/3esplane.cpp
  painter/3espose.cpp
  painter/3essphere.cpp
  painter/3esstar.cpp
  mesh/3esconverter.cpp
  shaders/3esedl.cpp
  shaders/3esedl.frag
  shaders/3esedl.vert
  util/3esresourcelist.cpp
)

list(APPEND PRIVATE_SOURCES
)
