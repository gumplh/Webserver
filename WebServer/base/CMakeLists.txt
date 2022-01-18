file(GLOB LIB_SRC ./*cpp)
add_library(libserver_base ${LIB_SRC})
target_link_libraries(libserver_base pthread)