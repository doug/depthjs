echo off
del /F /Q "..\chrome-extension\plugin"
rmdir /S /Q "..\chrome-extension\plugin"
mkdir "..\chrome-extension\plugin"
copy "build\bin\depthjsplugin\Debug\npdepthjsplugin.dll" "..\chrome-extension\plugin"
Debug\simple_templater.exe ..\chrome-extension\manifest.json.template $PLUGIN_PATH "{ ""path"": ""plugin/npdepthjsplugin.dll"" }" ..\chrome-extension\manifest.json
