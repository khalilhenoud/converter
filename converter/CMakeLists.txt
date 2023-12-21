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
        ./source/parsers/nodes.cpp
        ./source/parsers/meshes.cpp
        ./source/parsers/materials.cpp
        ./source/parsers/textures.cpp
				)
			
target_link_libraries(${PROJECT_NAME}
				PRIVATE serializer
        PRIVATE assimp
				)

target_include_directories(${PROJECT_NAME} PUBLIC 
							"${PROJECT_BINARY_DIR}"
							"${PROJECT_SOURCE_DIR}/include"
							)