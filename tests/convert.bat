@ECHO OFF
IF EXIST build-vs10 SET BUILD=build-vs10\eressea\Debug
IF EXIST build-vs11 SET BUILD=build-vs11\eressea\Debug
IF EXIST build-vs12 SET BUILD=build-vs12\eressea\Debug
IF EXIST build-vs14 SET BUILD=build-vs14\eressea\Debug
SET EXE=%BUILD%\convert.exe
CD conf\e2
..\..\%EXE% rules rules.xml catalog.xml rules.dat
CD ..\e3
..\..\%EXE% rules rules.xml catalog.xml rules.dat
PAUSE
