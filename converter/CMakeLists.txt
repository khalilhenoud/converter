cmake_minimum_required(VERSION 3.22)

# set the project name
project(converter VERSION 1.0)

# TODO: should the module dependency be done here, should I download the module
# here, questions questions...

# specify the cpp standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add the executable
add_executable(${PROJECT_NAME} 
				./source/main.cpp
        ./source/parsers/quake/topology/point.cpp
        ./source/parsers/quake/topology/brush.cpp
        ./source/parsers/quake/topology/poly_brush.cpp
        ./source/parsers/quake/topology/polygon.cpp
        ./source/parsers/quake/string_utils.cpp
        ./source/parsers/quake/bvh_utils.cpp
        ./source/parsers/quake/loader.cpp
        ./source/parsers/quake/map.cpp
        ./source/parsers/assimp/loader.cpp
        ./source/parsers/assimp/bvhs.cpp
        ./source/parsers/assimp/fonts.cpp
        ./source/parsers/assimp/nodes.cpp
        ./source/parsers/assimp/meshes.cpp
        ./source/parsers/assimp/materials.cpp
        ./source/parsers/assimp/textures.cpp
        ./source/parsers/assimp/lights.cpp
        ./source/parsers/assimp/cameras.cpp
				)
			
target_link_libraries(${PROJECT_NAME}
				PRIVATE serializer
        PRIVATE assimp
        PRIVATE loaders
        PRIVATE entity
				)

target_include_directories(${PROJECT_NAME} PUBLIC 
							"${PROJECT_BINARY_DIR}"
							"${PROJECT_SOURCE_DIR}/include"
							)