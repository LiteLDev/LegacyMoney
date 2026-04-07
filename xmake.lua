add_rules("mode.debug", "mode.release")

add_repositories("levimc-repo " .. (get_config("levimc_repo") or "https://github.com/LiteLDev/xmake-repo.git"))

if is_config("target_type", "server") then
    add_requires("levilamina 26.10.*", {configs = {target_type = "server"}})
else
    add_requires("levilamina 26.10.*", {configs = {target_type = "client"}})
end

add_requires("levibuildscript")
add_requires("sqlitecpp 3.3.3")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

option("levimc_repo")
    set_default("https://github.com/LiteLDev/xmake-repo.git")
    set_showmenu(true)
    set_description("Set the levimc-repo path or url")
option_end()

option("target_type")
    set_default("server")
    set_showmenu(true)
    set_values("server", "client")
option_end()

target("LegacyMoney")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    add_cxflags( "/EHa", "/utf-8", "/W4", "/w44265", "/w44289", "/w44296", "/w45263", "/w44738", "/w45204")
    add_defines("NOMINMAX", "UNICODE", "LLMONEY_EXPORTS")
    add_packages("levilamina", "sqlitecpp")
    set_exceptions("none")
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")
    add_files("src/**.cpp")
    add_includedirs("src")
    add_headerfiles("src/LLMoney.h")
    after_build(function (target)
        local bindir = path.join(os.projectdir(), "bin")
        os.cp(path.join(os.projectdir(), "assets", "lang"), path.join(path.join(bindir, target:name()), "lang"))
    end)
