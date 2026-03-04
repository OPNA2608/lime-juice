#include "config.h"
#include "sexp_writer.h"
#include "sexp_reader.h"
#include "engine/ai5/loader.h"
#include "engine/ai1/loader.h"
#include "engine/adv/loader.h"
#include "engine/ai1/compiler.h"
#include "engine/ai5/compiler.h"
#include "engine/adv/compiler.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

static const char* TITLE = "juice";
static const char* VERSION = "v0.1.4 (lime-juice)";

enum class Command {
    None,
    Decompile,
    Compile,
    ShowPreset,
    ShowVersion,
};

// ansi color helpers
static void print_color(const std::string& color, const std::string& text) {
    // color codes: b-white=97, b-green=92, b-red=91, b-yellow=93, b-blue=94, b-cyan=96, b-magenta=95
    std::string code;

    if (color == "b-white")   { code = "97"; }
    else if (color == "b-green")   { code = "92"; }
    else if (color == "b-red")     { code = "91"; }
    else if (color == "b-yellow")  { code = "93"; }
    else if (color == "b-blue")    { code = "94"; }
    else if (color == "b-cyan")    { code = "96"; }
    else if (color == "b-magenta") { code = "95"; }

    if (!code.empty()) {
        std::cout << "\033[" << code << "m" << text << "\033[0m";
    } else {
        std::cout << text;
    }
}

static void println_color(const std::string& color, const std::string& text) {
    print_color(color, text);
    std::cout << std::endl;
}

static bool has_extension(const std::string& path, const std::string& ext) {
    std::string lower_path = path;
    std::transform(lower_path.begin(), lower_path.end(), lower_path.begin(), ::tolower);
    std::string lower_ext = ext;
    std::transform(lower_ext.begin(), lower_ext.end(), lower_ext.begin(), ::tolower);
    return lower_path.size() >= lower_ext.size() &&
           lower_path.compare(lower_path.size() - lower_ext.size(), lower_ext.size(), lower_ext) == 0;
}

static void print_usage() {
    std::cerr << "usage: " << TITLE << " <command> [options] <files...>" << std::endl;
    std::cerr << std::endl;
    std::cerr << "commands:" << std::endl;
    std::cerr << "  -d, --decompile     decompile MES bytecode into rkt source" << std::endl;
    std::cerr << "  -c, --compile       compile rkt source into MES bytecode" << std::endl;
    std::cerr << "  -P, --show-preset   show supported presets" << std::endl;
    std::cerr << "  -v, --version       show version" << std::endl;
    std::cerr << std::endl;
    std::cerr << "options:" << std::endl;
    std::cerr << "  -f, --force         force overwrite output files" << std::endl;
    std::cerr << "  -o, --output PATH   output file path (default: input with swapped extension)" << std::endl;
    std::cerr << "  -p, --preset NAME   preset for a specific game; see --show-preset" << std::endl;
    std::cerr << "  -e, --engine TYPE   engine type (AI5*, AI1, ADV)" << std::endl;
    std::cerr << "  -C, --charset NAME  charset encoding (pc98*, english, europe, korean-..)" << std::endl;
    std::cerr << "  -D, --dictbase HEX  [AI5] dictionary base (80*, D0)" << std::endl;
    std::cerr << "  -E, --extraop       [AI5/ADV] support newer opcodes" << std::endl;
    std::cerr << "  --protag SPEC       {decompile} proc/call(s) fused in text" << std::endl;
    std::cerr << "  --no-decode         {decompile} skip SJIS character decoding" << std::endl;
    std::cerr << "  --no-resolve        {decompile} skip cmd/sys name resolution" << std::endl;
    std::cerr << "  --no-compress       {compile} skip AI5 dictionary compression" << std::endl;
}

static void show_presets() {
    auto presets = get_presets();

    for (const auto& p : presets) {
        // pad key to 6 chars
        std::string padded = p.key;

        while (padded.size() < 6) {
            padded += ' ';
        }

        print_color("b-white", padded);
        std::cout << " : " << p.title << std::endl;
    }
}

// for globbing: applies multiple replacements in a single pass (first match wins at each position)
static void replace_all(std::string& s,
    const std::vector<std::pair<std::string, std::string>>& ops) {
    std::string buf;
    buf.reserve(s.size());

    for (std::size_t i = 0; i < s.size(); ) {
        bool matched = false;

        for (const auto& [needle, repl] : ops) {

            if (s.compare(i, needle.size(), needle) == 0) {
                buf += repl;
                i += needle.size();
                matched = true;
                break;
            }
        }

        if (!matched) {
            buf += s[i];
            i++;
        }
    }

    s.swap(buf);
}

// expand glob patterns in file arguments
static std::vector<std::string> expand_globs(const std::vector<std::string>& args) {
    std::vector<std::string> result;

    for (const auto& arg : args) {

        // check if it contains glob characters
        if (arg.find('*') != std::string::npos || arg.find('?') != std::string::npos) {
            // use filesystem to expand
            fs::path dir = fs::path(arg).parent_path();

            if (dir.empty()) {
                dir = ".";
            }

            std::string pattern = fs::path(arg).filename().string();
            // convert glob to regex: wildcards first, then escape regex metacharacters
            std::string patternGlob2Reg = pattern;
            replace_all(patternGlob2Reg, {
                {"*", ".*"},
                {"?", "."},
                {"\\", "\\\\"},
                {".", "\\."},
                {"+", "\\+"},
                {"(", "\\("},
                {")", "\\)"},
                {"[", "\\["},
                {"]", "\\]"},
                {"{", "\\{"},
                {"}", "\\}"},
                {"^", "\\^"},
                {"$", "\\$"},
                {"|", "\\|"},
            });

            std::regex re(patternGlob2Reg);

            if (fs::exists(dir) && fs::is_directory(dir)) {

                for (const auto& entry : fs::directory_iterator(dir)) {

                    if (!entry.is_regular_file()) {
                        continue;
                    }

                    std::string name = entry.path().filename().string();

                    if (std::regex_match(name, re)) {
                        result.push_back(entry.path().string());
                    }
                }
            }
        } else {
            result.push_back(arg);
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

static void decompile_file(const std::string& path, Config& cfg, bool force,
                           const std::string& output_override = "") {
    // output: use override or replace .mes with .rkt
    std::string outname;

    // non .mes-extension files are valid if for some reason the original dev renamed them (does this exist?)
    if (!output_override.empty()) {
        outname = output_override;
    } else if(has_extension(path, ".mes")) {
        outname = path.substr(0, path.size() - 4) + ".rkt";
    } else {
        outname = path + ".rkt";
    }

    // display the output filename stem, status suffix follows
    std::string out_stem = fs::path(outname).stem().string();
    print_color("b-white", out_stem);
    std::cout.flush();

    // check if output exists
    if (!force && fs::exists(outname)) {
        println_color("b-red", " ! output file already exists (use -f to overwrite)");
        return;
    }

    try {
        AstNode ast = AstNode::make_list("mes", {});
        std::vector<std::string> parse_warnings;

        if (cfg.engine == EngineType::ADV) {
            auto result = adv::load_mes(path, cfg);
            ast = std::move(result.ast);
            parse_warnings = std::move(result.warnings);
        } else if (cfg.engine == EngineType::AI1) {
            ast = ai1::load_mes(path, cfg);
        } else {
            ast = ai5::load_mes(path, cfg);
        }

        // check for suspiciously empty output, the mes node should
        // contain segments. if it only has the meta wrapper and nothing
        // else, the parser likely failed silently.
        size_t input_size = fs::file_size(path);
        size_t segment_count = 0;

        for (const auto& child : ast.children) {

            if (child.tag != "meta") {
                segment_count++;
            }
        }

        SexpWriter writer;
        std::string output = writer.format(ast);

        std::ofstream out(outname);

        if (!out.is_open()) {
            throw std::runtime_error("cannot write to: " + outname);
        }

        out << output;
        out.close();

        if (segment_count == 0 && input_size > 4) {
            println_color("b-yellow", ".rkt (warning: no segments parsed from " +
                std::to_string(input_size) + " byte input - wrong --engine or missing --extraop?)");
        } else if (output.size() < 256 && input_size > 64) {
            println_color("b-yellow", ".rkt (warning: output is suspiciously small. only " +
                std::to_string(output.size()) + " bytes from " +
                std::to_string(input_size) + " byte input)");
        // arbitrary threshold where it becomes an error
        } else if (parse_warnings.size() > 10) {
            println_color("b-yellow", ".rkt (warning: \033[31m" +
                std::to_string(parse_warnings.size()) +
                " recovery skip(s) during parse, likely wrong --engine or --extraop setting\033[93m)");
        } else if (!parse_warnings.empty()) {
            println_color("b-yellow", ".rkt (warning: " +
                std::to_string(parse_warnings.size()) + " recovery skip(s) during parse)");
        } else {
            println_color("b-green", ".rkt");
        }

        // print recovery warnings after the status line (to stderr so
        // they don't interfere with machine-parseable status output)
        for (const auto& w : parse_warnings) {
            std::cerr << "\033[93m  " << w << "\033[0m" << std::endl;
        }
    } catch (const std::exception& e) {
        println_color("b-red", "!");
        std::cerr << "  " << e.what() << std::endl;
    }
}

// extract engine/charset/extraop from the (meta ...) node in an RKT AST
static void apply_meta_config(const AstNode& ast, Config& cfg) {

    for (const auto& child : ast.children) {

        if (!child.is_list("meta")) {
            continue;
        }

        for (const auto& m : child.children) {

            // (engine 'ADV)
            if (m.is_list("engine") && !m.children.empty() &&
                m.children[0].is_quote() && !m.children[0].children.empty() &&
                m.children[0].children[0].is_symbol()) {
                cfg.set_engine(m.children[0].children[0].str_val);
            }

            // (charset "pc98")
            if (m.is_list("charset") && !m.children.empty() &&
                m.children[0].is_string()) {
                cfg.charset_name = m.children[0].str_val;
            }

            // (extraop #t)
            if (m.is_list("extraop") && !m.children.empty() &&
                m.children[0].is_boolean() && m.children[0].bool_val) {
                cfg.extra_op = true;
            }
        }

        break;
    }
}

static void compile_file(const std::string& path, Config& cfg, bool force,
                         const std::string& output_override = "") {

    if (!has_extension(path, ".rkt")) {
        print_color("b-white", fs::path(path).filename().string());
        println_color("b-yellow", "?");
        return;
    }

    // output: use override or replace .rkt with .mes
    std::string outname = output_override.empty()
        ? path.substr(0, path.size() - 4) + ".mes"
        : output_override;

    std::string out_stem = fs::path(outname).stem().string();
    print_color("b-white", out_stem);
    std::cout.flush();

    if (!force && fs::exists(outname)) {
        println_color("b-red", " ! output file already exists (use -f to overwrite)");
        return;
    }

    try {
        // read rkt source
        std::ifstream f(path);

        if (!f.is_open()) {
            throw std::runtime_error("cannot open: " + path);
        }

        std::ostringstream ss;
        ss << f.rdbuf();
        std::string rkt_text = ss.str();

        // parse s-expression
        SexpReader reader;
        AstNode ast = reader.parse(rkt_text);

        // apply meta settings (engine, charset, extraop) from the rkt file
        // cli flags override these if explicitly set before the file args
        Config compile_cfg = cfg;
        apply_meta_config(ast, compile_cfg);

        // compile based on engine type
        std::vector<uint8_t> compiled;

        if (compile_cfg.engine == EngineType::ADV) {
            compiled = adv::compile_mes(ast, compile_cfg);
        } else if (compile_cfg.engine == EngineType::AI1) {
            compiled = ai1::compile_mes(ast, compile_cfg);
        } else if (compile_cfg.engine == EngineType::AI5) {
            compiled = ai5::compile_mes(ast, compile_cfg);
        } else {
            throw std::runtime_error("compiler not implemented for this engine");
        }

        // write output
        std::ofstream out(outname, std::ios::binary);

        if (!out.is_open()) {
            throw std::runtime_error("cannot write to: " + outname);
        }

        out.write(reinterpret_cast<const char*>(compiled.data()), compiled.size());
        out.close();

        println_color("b-green", ".mes");
    } catch (const std::exception& e) {
        println_color("b-red", "!");
        std::cerr << "  " << e.what() << std::endl;
    }
}

int main(int argc, char* argv[]) {

#ifdef _WIN32
    // enable ANSI escape sequences in Windows console (cmd.exe, PowerShell)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;

        if (GetConsoleMode(hOut, &mode)) {
            SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
    }
#endif

    Command command = Command::None;
    Config cfg;
    bool force = false;
    std::string output_path;
    std::vector<std::string> file_args;

    // parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-d" || arg == "--decompile") {
            command = Command::Decompile;
        } else if (arg == "-c" || arg == "--compile") {
            command = Command::Compile;
        } else if (arg == "-P" || arg == "--show-preset") {
            command = Command::ShowPreset;
        } else if (arg == "-v" || arg == "--version") {
            command = Command::ShowVersion;
        } else if (arg == "-f" || arg == "--force") {
            force = true;
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            i++;
            output_path = argv[i];
        } else if ((arg == "-p" || arg == "--preset") && i + 1 < argc) {
            i++;
            cfg.use_preset(argv[i]);
        } else if ((arg == "-e" || arg == "--engine") && i + 1 < argc) {
            i++;
            cfg.set_engine(argv[i]);
        } else if ((arg == "-C" || arg == "--charset") && i + 1 < argc) {
            i++;
            cfg.charset_name = argv[i];
        } else if ((arg == "-D" || arg == "--dictbase") && i + 1 < argc) {
            i++;
            cfg.dict_base = static_cast<uint8_t>(std::stoul(argv[i], nullptr, 16));
        } else if (arg == "-E" || arg == "--extraop") {
            cfg.extra_op = true;
        } else if (arg == "--no-decode") {
            cfg.decode = false;
        } else if (arg == "--no-resolve") {
            cfg.resolve = false;
        } else if (arg == "--no-compress") {
            cfg.compress = false;
        } else if (arg == "--protag" && i + 1 < argc) {
            i++;
            cfg.set_protag(argv[i]);
        } else if (arg == "-h" || arg == "--help") {
            print_usage();
            return 0;
        } else if (arg[0] == '-') {
            std::cerr << "unknown option: " << arg << std::endl;
            print_usage();
            return 1;
        } else {
            file_args.push_back(arg);
        }
    }

    switch (command) {
        case Command::ShowVersion:
            std::cout << TITLE << " " << VERSION << " (C++ rewrite)" << std::endl;
            return 0;

        case Command::ShowPreset:
            show_presets();
            return 0;

        case Command::Decompile: {
            auto paths = expand_globs(file_args);

            if (paths.empty()) {
                std::cerr << "no input files" << std::endl;
                return 1;
            }

            if (!output_path.empty() && paths.size() > 1) {
                std::cerr << "-o cannot be used with multiple input files" << std::endl;
                return 1;
            }

            for (const auto& path : paths) {
                decompile_file(path, cfg, force, output_path);
            }

            return 0;
        }

        case Command::Compile: {
            auto paths = expand_globs(file_args);

            if (paths.empty()) {
                std::cerr << "no input files" << std::endl;
                return 1;
            }

            if (!output_path.empty() && paths.size() > 1) {
                std::cerr << "-o cannot be used with multiple input files" << std::endl;
                return 1;
            }

            for (const auto& path : paths) {
                compile_file(path, cfg, force, output_path);
            }

            return 0;
        }

        case Command::None:
        default:
            std::cout << "type `" << TITLE << " -h` for help" << std::endl;
            return 0;
    }
}
