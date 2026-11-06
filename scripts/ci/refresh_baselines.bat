@echo off
REM refresh_baselines.bat - sync screenshot baselines with CI-rendered captures.
REM Examples:
REM   scripts\ci\refresh_baselines.bat                    latest CMake workflow run
REM   scripts\ci\refresh_baselines.bat -RunId 123456      given run
REM   scripts\ci\refresh_baselines.bat -DryRun            list what would be copied
REM After copying: review `git diff --stat tests/visual/baseline`, commit, push.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0refresh_baselines.ps1" %*
exit /b %ERRORLEVEL%
