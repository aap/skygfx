workspace "skygfx"
	configurations { "Release", "Debug" }
	location "build"
   
	defines { "rsc_CompanyName=\"aap\"" }
	defines { "rsc_LegalCopyright=\"\""} 
	defines { "rsc_FileVersion=\"4.0.0.0\"", "rsc_ProductVersion=\"4.0.0.0\"" }
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
	includedirs { os.getenv("RWSDK36") }
   
	prebuildcommands {
		"for /R \"../shaders/ps/\" %%f in (*.hlsl) do \"%DXSDK_DIR%/Utilities/bin/x86/fxc.exe\" /T ps_2_0 /nologo /E main /Fo ../resources/cso/%%~nf.cso %%f",
		"for /R \"../shaders/vs/\" %%f in (*.hlsl) do \"%DXSDK_DIR%/Utilities/bin/x86/fxc.exe\" /T vs_2_0 /nologo /E main /Fo ../resources/cso/%%~nf.cso %%f",
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
		debugdir "C:/Users/aap/games/gtasa"
		debugcommand "C:/Users/aap/games/gtasa/gta_sa.exe"
		postbuildcommands "copy /y \"$(TargetPath)\" \"C:\\Users\\aap\\games\\gtasa\\plugins\\skygfx.dll\""

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"
		flags { "StaticRuntime" }
		debugdir "C:/Users/aap/games/gtasa"
		debugcommand "C:/Users/aap/games/gtasa/gta_sa.exe"
		postbuildcommands "copy /y \"$(TargetPath)\" \"C:\\Users\\aap\\games\\gtasa\\plugins\\skygfx.dll\""
