set(FILES
    DDS.h
    DDSTextureLoader.cpp
    DDSTextureLoader.h
    PlatformHelpers.h
)

add_library(DDSTextureLoader STATIC ${FILES})

target_include_directories(DDSTextureLoader PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(DDSTextureLoader PRIVATE d3dx12)
set_target_properties(DDSTextureLoader PROPERTIES FOLDER "3rdparty")
