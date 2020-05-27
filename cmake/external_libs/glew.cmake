if(NOT TARGET GLEW::GLEW)
    message(FATAL_ERROR "Could not find glew")
endif()

BoB_find_package(GLEW)
BoB_external_libraries(opengl)
