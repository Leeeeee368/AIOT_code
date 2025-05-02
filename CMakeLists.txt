cmake_minimum_required(VERSION 3.4.1)

project(AIOT_demo LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# 针对C文件的特殊处理
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS "-pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")

# skip 3rd-party lib dependencies
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--allow-shlib-undefined")

# install target and libraries
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/AIOT_demo_${CMAKE_SYSTEM_NAME})

set(CMAKE_SKIP_INSTALL_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# 查找OpenCV库
# find_package(OpenCV REQUIRED)  # 自动查找OpenCV路径‌
# if(OpenCV_FOUND)
    # include_directories(${OpenCV_INCLUDE_DIRS})  # 添加OpenCV头文件路径
# endif()

# 查找SDL2库
# set(SDL2_PATH ${CMAKE_SOURCE_DIR}/src/SDL2)
# set(SDL2_LIB ${SDL2_PATH}/lib/libSDL2-2.0.so.0.3200.5)

# 查找SDL2_ttf库
# set(SDL2_TTF_PATH ${CMAKE_SOURCE_DIR}/src/SDL_ttf)
# set(SDL2_TTF_LIB ${SDL2_TTF_PATH}/lib/libSDL2_ttf-2.0.so.0.2400.0)

# 查找Json-c库
set(Json_c_PATH ${CMAKE_SOURCE_DIR}/include/json-c)
set(Json_c_LIB ${Json_c_PATH}/lib/libjson-c.so.5)

# 查找paho_mqtt库
set(paho_mqtt_PATH ${CMAKE_SOURCE_DIR}/include/paho_mqtt)
set(paho_mqtt_LIB ${paho_mqtt_PATH}/lib/libpaho-mqtt3c.so)

# 查找sensor库
set(sensor_PATH ${CMAKE_SOURCE_DIR}/include/sensor)

# 查找rs485库
set(rs485_PATH ${CMAKE_SOURCE_DIR}/include/rs485)

# 查找my_mqtt库
set(my_mqtt_PATH ${CMAKE_SOURCE_DIR}/include/my_mqtt)

set(CMAKE_INSTALL_RPATH "lib")

# rknn_yolov5_demo
include_directories(
  ${CMAKE_SOURCE_DIR}/include
  ${Json_c_PATH}/include
  ${paho_mqtt_PATH}/include
  ${sensor_PATH}/include
  ${rs485_PATH}/include
  ${my_mqtt_PATH}/include
  # ${SDL2_TTF_PATH}/include
  )

add_executable(AIOT_demo
  src/main.cc
  ${sensor_PATH}/src/sensor.cc
  ${rs485_PATH}/src/RS485.cc
  ${my_mqtt_PATH}/src/mqtt_client.cc
  # src/rs485_test.c
  # src/relay_music.c
)

# 链接库配置
target_link_libraries(AIOT_demo
    # ${OpenCV_LIBS}  # OpenCV库‌
    # -Wl,--disable-new-dtags
    # -Wl,-rpath=${SDL2_TTF_PATH}/lib:${SDL2_PATH}/lib
    # ${SDL2_TTF_LIB}
    # ${SDL2_LIB}
    ${Json_c_LIB}
    ${paho_mqtt_LIB}
)


# install target and libraries
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/install/AIOT_demo_${CMAKE_SYSTEM_NAME})
install(TARGETS AIOT_demo DESTINATION ./)
# install(PROGRAMS ${Json_c_LIB} DESTINATION lib)
# install(PROGRAMS ${SDL2_TTF_LIB} DESTINATION lib)
# install(FILES ${SDL2_TTF_PATH}/fonts/simkai.ttf DESTINATION ./)
