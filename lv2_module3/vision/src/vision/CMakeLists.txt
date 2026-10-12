cmake_minimum_required(VERSION 3.20)
project(vision)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(realsense2 REQUIRED)
find_package(vision_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core imgproc)

add_executable(image src/image.cpp)
target_compile_features(image PUBLIC cxx_std_17)
target_link_libraries(image PUBLIC
  rclcpp::rclcpp
  ${sensor_msgs_TARGETS}
  ${realsense2_LIBRARY}
)

add_executable(mask src/mask.cpp)
target_compile_features(mask PUBLIC cxx_std_17)
target_link_libraries(mask PUBLIC
  rclcpp::rclcpp
  ${sensor_msgs_TARGETS}
  cv_bridge::cv_bridge
  ${OpenCV_LIBS}
)

add_executable(detection src/detection.cpp)
target_compile_features(detection PUBLIC cxx_std_17)
target_link_libraries(detection PUBLIC
  rclcpp::rclcpp
  ${sensor_msgs_TARGETS}
  ${vision_msgs_TARGETS}
  cv_bridge::cv_bridge
  ${OpenCV_LIBS}
)

install(TARGETS image mask detection
  DESTINATION lib/${PROJECT_NAME})


ament_package()
