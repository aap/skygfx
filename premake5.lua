GTA_SA_DIR = "C:/Users/aap/games/gtasa"
GTA_SA_EXE = "gta_sa.exe"
RWSDK_DIR = os.getenv("RWSDK36")
DXSDK_DIR = "%DXSDK_DIR%"
FXC_DIR = "/Utilities/bin/x86/fxc.exe"
PLUGIN_DIR = "/plugins/skygfx.dll" -- rename this to skygfx.asi if you want..

workspace "skygfx"
	configurations { "Release", "Debug" }
	location "build"
   
	defines { "rsc_CompanyName=\"aap\"" }
	defines { "rsc_LegalCopyright=\"\""} 
	defines { "rsc_FileVersion=\"4.1.0.0\"", "rsc_ProductVersion=\"4.1.0.0\"" }
	defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"%{prj.name}.dll\"" }
	defines { "rsc_FileDescription=\"https://github.com/aap\"" }
	defines { "rsc_UpdateUrl=\"https://github.com/aap/skygfx\"" }
   
	files { "external/*.*" }
	files { "resources/*.*" }
	files { "shaders/*.*" }
	files { "src/*.*" }
   
	includedirs { "external/injector/include" }
	includedirs { "external" }
	includedirs { "resources" }
	includedirs { "shaders" }
	includedirs { "src" }
	includedirs { RWSDK_DIR }
   
	prebuildcommands {
		"for /R \"../shaders/ps/\" %%f in (*.hlsl) do \"" .. DXSDK_DIR .. FXC_DIR .. "\" /T ps_2_0 /nologo /E main /Fo ../resources/cso/%%~nf.cso %%f",
		"for /R \"../shaders/vs/\" %%f in (*.hlsl) do \"" .. DXSDK_DIR .. FXC_DIR .. "\" /T vs_2_0 /nologo /E main /Fo ../resources/cso/%%~nf.cso %%f",
	}
      
project "skygfx"
	kind "SharedLib"
	language "C++"
	targetdir "bin/%{cfg.buildcfg}"
	targetextension ".dll"
	characterset ("MBCS")

	buildoptions { "/Zc:threadSafeInit-" }

	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"
		flags { "StaticRuntime" }
		debugdir(GTA_SA_DIR)
		debugcommand(GTA_SA_DIR .. GTA_SA_EXE)
		postbuildcommands { "copy /y \"$(TargetPath)\" \"" .. GTA_SA_DIR .. PLUGIN_DIR .. "\"" }

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"
		flags { "StaticRuntime" }
		debugdir(GTA_SA_DIR)
		debugcommand(GTA_SA_DIR .. GTA_SA_EXE)
		postbuildcommands { "copy /y \"$(TargetPath)\" \"" .. GTA_SA_DIR .. PLUGIN_DIR .. "\"" }
