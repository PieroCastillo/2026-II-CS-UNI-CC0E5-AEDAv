set_languages("c++23")

target("main")
    set_kind("binary")
    add_includedirs("./")
    add_headerfiles("**.h")
    add_files("**.cpp")
    if is_mode("debug") then
        set_policy("build.sanitizer.address", true);
    end