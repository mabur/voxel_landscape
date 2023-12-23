# This has worked at least once with
# cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/home/rasmusgo/src/vcpkg/scripts/buildsystems/vcpkg.cmake -DSDL2_DIR=/home/rasmusgo/src/vcpkg/packages/sdl2_x64-linux/share/sdl2/

# For Clion on Linux the full Cmake option line then becomes:
# -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_MAKE_PROGRAM=/home/mabur/.local/share/JetBrains/Toolbox/apps/CLion/ch-0/222.4345.21/bin/ninja/linux/ninja -DCMAKE_TOOLCHAIN_FILE=/home/mabur/repositories/vcpkg/scripts/buildsystems/vcpkg.cmake
# -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=/home/mabur/repositories/vcpkg/scripts/buildsystems/vcpkg.cmake

cmake_minimum_required(VERSION 3.13)
project(voxel_landscape)

find_package(SDL2 CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)

FILE(GLOB source "src/*.cpp" "src/*.hpp")

add_executable(voxel_landscape ${source})

include(FetchContent)
FetchContent_Declare(
        iglo
        GIT_REPOSITORY https://github.com/mabur/iglo.git
        GIT_TAG 2af10be4ad06d288039e1817bc17ec8a11fa9a32
)
FetchContent_MakeAvailable(iglo)
target_include_directories(voxel_landscape PRIVATE ${iglo_SOURCE_DIR})

target_link_libraries(voxel_landscape PRIVATE
        iglo
        Eigen3::Eigen
        $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
        $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
        )

target_compile_features(voxel_landscape PRIVATE cxx_std_17)

# Handle local resource paths:
file(COPY images/ DESTINATION ${CMAKE_BINARY_DIR}/images)
