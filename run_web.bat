@echo off
title MediPulse C++ Web Server
echo ========================================================
echo       MediPulse C++ Smart Medicine Reminder System
echo ========================================================
echo.
echo [1/2] Starting C++ REST API Server on port 8080...
start "" "http://localhost:8080"
echo [2/2] Opening your browser to http://localhost:8080/ ...
echo.
echo Server is actively running. Press Ctrl+C in this window to stop.
echo ========================================================
MedicineServer.exe

