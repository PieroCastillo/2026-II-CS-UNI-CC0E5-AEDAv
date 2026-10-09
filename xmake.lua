set_languages("c++23")

target("main")
    set_kind("binary")
    add_files("**.cpp")
    if is_mode("debug") then
        set_policy("build.sanitizer.address", true);
    end