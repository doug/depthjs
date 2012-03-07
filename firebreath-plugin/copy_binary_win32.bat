echo off

del /F /Q "..\chrome-extension\plugin"
rmdir /S /Q "..\chrome-extension\plugin"
mkdir "..\chrome-extension\plugin"
copy build\bin\depthjsplugin\%1\npdepthjsplugin.dll "..\chrome-extension\plugin\"

del /F /Q ..\chrome-extension\manifest.json
REM %1\simple_templater.exe ..\chrome-extension\manifest.json.template $PLUGIN_PATH "{ ""path"": ""plugin/npdepthjsplugin.dll"" }" ..\chrome-extension\manifest.json
copy ..\chrome-extension\manifest.json.WIN32 ..\chrome-extension\manifest.json