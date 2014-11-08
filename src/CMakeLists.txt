INCLUDE_DIRECTORIES("${sidewinderd_SOURCE_DIR}/lib/tinyxml2")
LINK_DIRECTORIES("${sidewinderd_SOURCE_DIR}/lib/tinyxml2")

ADD_EXECUTABLE(sidewinderd main.cpp keyboard.cpp)

TARGET_LINK_LIBRARIES(sidewinderd tinyxml2 udev config++)
