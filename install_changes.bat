@echo off
setlocal

echo.
echo === Nova Poshta backend/frontend integration ===
echo.

if not exist "front\CMakeLists.txt" (
    echo ERROR: Run this file from the NovaPochta_r repository root.
    pause
    exit /b 1
)

if not exist "nova_poshta_warehouse_backend\include\persistence\Database.h" (
    echo ERROR: New backend architecture was not found.
    pause
    exit /b 1
)

echo Creating folders...
if not exist "nova_poshta_warehouse_backend\include\application" mkdir "nova_poshta_warehouse_backend\include\application"
if not exist "nova_poshta_warehouse_backend\src\application" mkdir "nova_poshta_warehouse_backend\src\application"

echo Copying files...

copy /Y "files\nova_poshta_warehouse_backend\CMakeLists.txt" "nova_poshta_warehouse_backend\CMakeLists.txt" >nul
copy /Y "files\nova_poshta_warehouse_backend\include\application\BackendFacade.h" "nova_poshta_warehouse_backend\include\application\BackendFacade.h" >nul
copy /Y "files\nova_poshta_warehouse_backend\src\application\BackendFacade.cpp" "nova_poshta_warehouse_backend\src\application\BackendFacade.cpp" >nul

copy /Y "files\front\CMakeLists.txt" "front\CMakeLists.txt" >nul
copy /Y "files\front\include\Application.h" "front\include\Application.h" >nul
copy /Y "files\front\src\Application.cpp" "front\src\Application.cpp" >nul
copy /Y "files\front\include\ui\SimulationScreen.h" "front\include\ui\SimulationScreen.h" >nul
copy /Y "files\front\src\ui\SimulationScreen.cpp" "front\src\ui\SimulationScreen.cpp" >nul

echo.
echo DONE.
echo.
echo Next:
echo   git status
echo.
echo Then rebuild:
echo   rmdir /S /Q front\build
echo   cmake -S front -B front\build
echo   cmake --build front\build
echo.
pause
