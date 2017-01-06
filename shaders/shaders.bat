for /R "./ps/" %%f in (*.hlsl) do "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T ps_2_0 /nologo /E main /Fo ../resources/release/%%~nf.cso %%f
for /R "./vs/" %%f in (*.hlsl) do "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T vs_2_0 /nologo /E main /Fo ../resources/release/%%~nf.cso %%f

"%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T ps_2_0 /nologo /E mainPS /Fo ../resources/release/vcsReflPS.cso ./pvs/vcsReflPS.hlsl
"%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T vs_2_0 /nologo /E mainVS /Fo ../resources/release/vcsReflVS.cso ./pvs/vcsReflVS.hlsl

"%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T ps_2_0 /nologo /E mainPS /Fo ../resources/release/vehiclePS.cso ./pvs/vehiclePS.hlsl
"%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T vs_2_0 /nologo /E mainVS /Fo ../resources/release/vehicleVS.cso ./pvs/vehicleVS.hlsl