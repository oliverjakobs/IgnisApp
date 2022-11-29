project "Ignis"
    kind "StaticLib"
    language "C"
    
    targetdir ("build/bin/" .. output_dir .. "/%{prj.name}")
    objdir ("build/bin-int/" .. output_dir .. "/%{prj.name}")

    files
    {
        "Ignis/src/**.h",
        "Ignis/src/**.c"
    }

    links
    {
        "opengl32"
    }

    includedirs
    {
        "Ignis/src"
    }

    filter "system:windows"
        systemversion "latest"
        staticruntime "On"
        defines { "_CRT_SECURE_NO_WARNINGS" }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        runtime "Release"
        optimize "On"