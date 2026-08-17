cmake_minimum_required(VERSION 3.24)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

project(autonomy_teensy)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include(GNUInstallDirs)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(generate_parameter_library REQUIRED)
find_package(Boost REQUIRED)

generate_parameter_library(teensy_driver_parameters
    src/teensy_driver_parameters.yaml
)

add_library(autonomy_teensy_protocol SHARED)
target_sources(
    autonomy_teensy_protocol
    PRIVATE src/client.cpp src/cobs.cpp src/crc.cpp src/packet.cpp
    PUBLIC
        FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
            FILES
                ${CMAKE_CURRENT_SOURCE_DIR}/include/autonomy_teensy/client.hpp
                ${CMAKE_CURRENT_SOURCE_DIR}/include/autonomy_teensy/device_id.hpp
                ${CMAKE_CURRENT_SOURCE_DIR}/include/autonomy_teensy/driver.hpp
                ${CMAKE_CURRENT_SOURCE_DIR}/include/autonomy_teensy/packet.hpp
                ${CMAKE_CURRENT_SOURCE_DIR}/include/autonomy_teensy/packet_id.hpp
)
target_compile_features(autonomy_teensy_protocol PUBLIC cxx_std_23)
target_link_libraries(autonomy_teensy_protocol PUBLIC Boost::boost)
set_target_properties(autonomy_teensy_protocol PROPERTIES PREFIX "")

add_library(autonomy_teensy_library SHARED)
target_sources(
    autonomy_teensy_library
    PRIVATE src/driver.cpp
    PUBLIC
        FILE_SET HEADERS
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
            FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/autonomy_teensy/driver.hpp
)
target_compile_features(autonomy_teensy_library PUBLIC cxx_std_23)
target_link_libraries(
    autonomy_teensy_library
    PRIVATE autonomy_teensy_protocol teensy_driver_parameters
    PUBLIC
        rclcpp::rclcpp
        rclcpp_lifecycle::rclcpp_lifecycle
        rclcpp_components::component
        rclcpp_components::component_manager
        ${sensor_msgs_TARGETS}
)

rclcpp_components_register_node(autonomy_teensy_library
    PLUGIN "teensy::TeensyDriver"
    EXECUTABLE teensy_driver
)

install(
    TARGETS
        autonomy_teensy_library
        autonomy_teensy_protocol
        teensy_driver_parameters
    EXPORT export_autonomy_teensy_library
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    FILE_SET HEADERS
)

install(DIRECTORY launch DESTINATION share/autonomy_teensy)

ament_export_targets(export_autonomy_teensy_library HAS_LIBRARY_TARGET)
ament_export_dependencies(
    "rclcpp"
    "rclcpp_lifecycle"
    "rclcpp_components"
    "sensor_msgs"
    "Boost"
)

ament_package()
