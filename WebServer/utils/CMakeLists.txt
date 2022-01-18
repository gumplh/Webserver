file(GLOB LIB_SRC ./*cpp)
add_library(libserver_utils ${LIB_SRC})
target_link_libraries(libserver_utils)