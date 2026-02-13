cmake_minimum_required(VERSION 3.10)
project(libmqtt VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Core library
add_library(mqtt STATIC
    src/core/mqtt.c
    src/core/mqtt_os.c
    src/core/mqtt_net.c
)

# POSIX port library
add_library(mqtt_posix STATIC
    src/port/posix_os.c
    src/port/posix_net.c
)
target_link_libraries(mqtt_posix pthread)

# Demo executable
add_executable(mqtt_demo
    examples/demo.c
)
target_link_libraries(mqtt_demo mqtt mqtt_posix)

# Install targets
install(TARGETS mqtt mqtt_posix
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/
    DESTINATION include
)
