set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(cpr 
    GIT_REPOSITORY https://github.com/libcpr/cpr.git 
    GIT_TAG 1.14.1
)
FetchContent_Declare(json 
    GIT_REPOSITORY https://github.com/nlohmann/json.git 
    GIT_TAG v3.11.3
)

FetchContent_MakeAvailable(cpr json)

add_library(logic_lib STATIC
    YandexData.cpp
)

find_package(Threads REQUIRED)

target_link_libraries(logic_lib
    PUBLIC
    cpr::cpr
    nlohmann_json::nlohmann_json
    ${CMAKE_THREAD_LIBS_INIT}
)

target_include_directories(logic_lib
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)