@echo off
rem Downloads Dear ImGui (MIT) and stb_image (public domain) into third_party\. Needs git in PATH.
setlocal
set ROOT=%~dp0..
set DEPS=%TEMP%\dis_deps
if exist "%DEPS%" rmdir /s /q "%DEPS%"
if not exist "%ROOT%\third_party\imgui" mkdir "%ROOT%\third_party\imgui"
if not exist "%ROOT%\third_party\stb" mkdir "%ROOT%\third_party\stb"
git clone --depth 1 --branch v1.90.9 https://github.com/ocornut/imgui.git "%DEPS%\imgui" || goto :err
git clone --depth 1 https://github.com/nothings/stb.git "%DEPS%\stb" || goto :err
for %%F in (imgui.h imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp imgui_internal.h imconfig.h imstb_rectpack.h imstb_textedit.h imstb_truetype.h) do copy /y "%DEPS%\imgui\%%F" "%ROOT%\third_party\imgui\" >nul || goto :err
for %%F in (imgui_impl_dx9.h imgui_impl_dx9.cpp imgui_impl_win32.h imgui_impl_win32.cpp) do copy /y "%DEPS%\imgui\backends\%%F" "%ROOT%\third_party\imgui\" >nul || goto :err
copy /y "%DEPS%\stb\stb_image.h" "%ROOT%\third_party\stb\" >nul || goto :err
rmdir /s /q "%DEPS%"
echo Dependencies installed in third_party\
exit /b 0
:err
echo FAILED to fetch dependencies
exit /b 1
