cmake_minimum_required(VERSION 3.28.0)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
   atomic
)