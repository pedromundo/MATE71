-- Conditionally include freeglut for non-macOS platforms
if not is_plat("macosx") then
    add_requires("freeglut")
end

add_requires("glew", "soil2", "glm")

target("RevolutionSolids")
    set_kind("binary")
    add_files("*.cpp")
    add_headerfiles("*.h")
    add_packages("glew", "soil2", "glm")
    set_languages("c++11")

    -- Add macOS frameworks for GLUT and OpenGL
    if is_plat("macosx") then
        add_frameworks("GLUT", "OpenGL")
    else
        add_packages("freeglut")
    end

    -- Shader/texture deployment
    after_build(function (target)
        os.cp("*.vert", target:targetdir())
        os.cp("*.frag", target:targetdir())
        os.cp("*.jpg", target:targetdir())
    end)
