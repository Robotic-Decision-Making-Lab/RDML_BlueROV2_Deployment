cmake_minimum_required(VERSION 3.8)
project(autonomy_description)

find_package(ament_cmake REQUIRED)

install(DIRECTORY config description DESTINATION share/autonomy_description)

ament_package()
