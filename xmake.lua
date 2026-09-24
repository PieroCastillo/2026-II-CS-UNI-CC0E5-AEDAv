set_languages("c++23")

-- valido si solo si existe un solo main() en el root directory
target("main")
    set_kind("binary")
    add_files("**.cpp")

-- configurar debug con: xmake f --mode=debug
-- esto corre con: xmake r main