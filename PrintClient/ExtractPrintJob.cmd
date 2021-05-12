@echo off
setlocal
if %1. == . goto usage
c:
cd \Users\%USERNAME%\Downloads
tar -xf PrintJob00%1.tar
if errorlevel 1 goto done
cd PrintJobs\PrintJob00%1
echo -----------------------------------------------------------
dir /b *.pdf
cd ..
echo -----------------------------------------------------------
echo Your files are located at "Downloads\PrintJobs\PrintJobNNN"
goto done
:usage
echo usage: ExtractPrintJob job-number
echo  e.g.: ExtractPrintJob 930
echo 
echo You will find the extracted files in the directory
echo    C:\Downloads\PrintJobs\PrintJobNNN
:done
REM pause
endlocal
