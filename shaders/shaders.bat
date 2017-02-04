for /R "./ps/" %%f in (*.hlsl) do "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T ps_3_0 /nologo /E main /Fo ../resources/cso/%%~nf.cso %%f
for /R "./vs/" %%f in (*.hlsl) do "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T vs_3_0 /nologo /E main /Fo ../resources/cso/%%~nf.cso %%f

"%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T ps_3_0 /nologo /E mainPS /Fo ../resources/cso/vcsReflPS.cso ./pvs/vcsReflPS.hlsl
"%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T vs_3_0 /nologo /E mainVS /Fo ../resources/cso/vcsReflVS.cso ./pvs/vcsReflVS.hlsl

"%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T ps_3_0 /nologo /E mainPS /Fo ../resources/cso/vehiclePS.cso ./pvs/vehiclePS.hlsl
"%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T vs_3_0 /nologo /E mainVS /Fo ../resources/cso/vehicleVS.cso ./pvs/vehicleVS.hlsl