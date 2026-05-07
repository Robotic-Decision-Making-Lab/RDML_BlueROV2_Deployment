cmake_minimum_required(VERSION 3.8)
project(bluerov_description)

find_package(ament_cmake REQUIRED)

install(DIRECTORY config DESTINATION share/bluerov_description)

ament_package()
