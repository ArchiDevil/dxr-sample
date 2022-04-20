set(FILES
    imconfig.h
    imgui.cpp
    imgui.h
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_internal.h
    imgui_tables.cpp
    imgui_widgets.cpp
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
    backends/imgui_impl_dx12.cpp
    backends/imgui_impl_dx12.h
    backends/imgui_impl_win32.cpp
    backends/imgui_impl_win32.h
)

add_library(imgui STATIC ${FILES})
target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_LIST_DIR})
set_target_properties(imgui PROPERTIES FOLDER "3rdparty")
