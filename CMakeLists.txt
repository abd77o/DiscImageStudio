cmake_minimum_required(VERSION 3.5)
project(DiscImageStudio CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(STB_DIR   ${CMAKE_SOURCE_DIR}/third_party/stb)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
if(EXISTS ${STB_DIR}/stb_image.h)
  set(HAVE_STB TRUE)
else()
  set(HAVE_STB FALSE)
  message(WARNING "third_party/stb/stb_image.h not found: building WITHOUT PNG/JPEG support (BMP only). See third_party/README.md")
endif()

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0601 -DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)
endif()

# ---- core -------------------------------------------------------------------------------
set(DIS_CORE_SRC src/core/ImageSource.cpp src/core/ImageLoader.cpp src/core/PolarDitherEngine.cpp
                 src/core/PlatformFile.cpp src/core/Config.cpp)
if(HAVE_STB)
  list(APPEND DIS_CORE_SRC src/core/stb_impl.cpp)
endif()
add_library(dis_core ${DIS_CORE_SRC})
if(HAVE_STB)
  target_include_directories(dis_core PUBLIC ${STB_DIR})
else()
  target_compile_definitions(dis_core PUBLIC DIS_NO_STB)
endif()

# ---- burn -------------------------------------------------------------------------------
set(DIS_BURN_SRC
  src/burn/Scrambler.cpp src/burn/SectorComposer.cpp src/burn/SectorSource.cpp
  src/burn/Mmc.cpp src/burn/CueSheet.cpp src/burn/SafeBufferMonitor.cpp
  src/burn/RawDaoWriter.cpp src/calib/CalibrationSource.cpp src/gui/BurnJob.cpp)
if(WIN32)
  list(APPEND DIS_BURN_SRC src/burn/SptiDevice.cpp)
endif()
add_library(dis_burn ${DIS_BURN_SRC})
target_link_libraries(dis_burn dis_core)
find_package(Threads)
if(Threads_FOUND)
  target_link_libraries(dis_burn ${CMAKE_THREAD_LIBS_INIT})
endif()

add_executable(dither_preview tools/dither_preview.cpp)
target_link_libraries(dither_preview dis_core)

# ---- hardware-free tests (mock drive) ---------------------------------------------------
add_executable(test_burn_logic tests/test_burn_logic.cpp)
target_link_libraries(test_burn_logic dis_burn)
add_executable(test_image_burn tests/test_image_burn.cpp)
target_link_libraries(test_image_burn dis_burn)
add_executable(test_gui_logic tests/test_gui_logic.cpp)
target_link_libraries(test_gui_logic dis_burn)

# ---- GUI (Windows only, needs third_party/imgui) -----------------------------------------
if(WIN32)
  if(EXISTS ${IMGUI_DIR}/imgui.cpp AND EXISTS ${IMGUI_DIR}/imgui_impl_dx9.cpp AND EXISTS ${IMGUI_DIR}/imgui_impl_win32.cpp)
    add_executable(DiscImageStudio WIN32
      src/gui/main_win.cpp src/gui/App.cpp src/gui/Dialogs_win.cpp
      ${IMGUI_DIR}/imgui.cpp ${IMGUI_DIR}/imgui_draw.cpp ${IMGUI_DIR}/imgui_tables.cpp
      ${IMGUI_DIR}/imgui_widgets.cpp ${IMGUI_DIR}/imgui_impl_dx9.cpp ${IMGUI_DIR}/imgui_impl_win32.cpp)
    target_include_directories(DiscImageStudio PRIVATE ${IMGUI_DIR} src/gui)
    target_compile_definitions(DiscImageStudio PRIVATE IMGUI_IMPL_WIN32_DISABLE_GAMEPAD)
    target_link_libraries(DiscImageStudio dis_burn d3d9 comdlg32 user32 gdi32)
    # single exe, must run elevated (SPTI writes need administrator rights)
    set_target_properties(DiscImageStudio PROPERTIES
      LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")
  else()
    message(WARNING "third_party/imgui not populated: GUI target skipped. See third_party/README.md")
  endif()
endif()

# Static CRT (/MT): no Visual C++ redistributable needed. Use with  -T v141_xp
if(MSVC)
  foreach(f CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    string(REPLACE "/MD" "/MT" ${f} "${${f}}")
  endforeach()
endif()
