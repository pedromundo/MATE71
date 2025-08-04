add_requires("opencv")

target("StainedGlass")
    set_kind("binary")
    add_files("StainedGlass/*.cpp")
    add_headerfiles("StainedGlass/*.h")
    add_packages("opencv")
    set_languages("c++11")

    -- For macOS, add required frameworks
    if is_plat("macosx") then
        add_frameworks("Foundation", "CoreFoundation", "CoreGraphics", "AppKit")
    end

    -- Copy image files to target directory
    after_build(function (target)
        os.cp("StainedGlass/*.jpg", target:targetdir())
    end)
