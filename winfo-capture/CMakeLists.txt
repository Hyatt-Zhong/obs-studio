project(wcpature)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Include)
file(GLOB SOURCES_FILE *.cpp *.h)

add_executable(wcpature ${SOURCES_FILE})

target_link_libraries(wcpature libobs)
define_graphic_modules(wcpature)
