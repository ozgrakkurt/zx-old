#![allow(clippy::single_match)]

use smol::prelude::*;
use std::cell::RefCell;
use std::collections::HashMap;
use std::path::{Path, PathBuf};

fn get_bool_arg(args: &HashMap<String, String>, name: &str, default: bool) -> bool {
    let positive = format!("--{}", name);
    let negative = format!("--no-{}", name);

    match args.get(&positive) {
        Some(v) => {
            assert_eq!(v.as_str(), "");
            assert!(args.get(&negative).is_none());
            return true;
        }
        None => {}
    }

    match args.get(&negative) {
        Some(v) => {
            assert_eq!(v.as_str(), "");
            return false;
        }
        None => {}
    }

    default
}

fn make_path_absolute(cwd: &Path, path: &Path) -> PathBuf {
    if !path.is_absolute() {
        cwd.join(path)
    } else {
        path.to_owned()
    }
}

struct FlagCfg {
    opt_level: String,
    dev_mode: bool,
    debug_info: bool,
    fuzz: bool,
    include_path: PathBuf,
}

fn make_cflags(cfg: &FlagCfg) -> Vec<&'static str> {
    let mut cflags = vec![
        // Warnings.
        "-Wall",
        "-Wextra",
        "-Wconversion",
        "-Wsign-conversion",
        "-Wmissing-field-initializers",
        "-Wshadow",
        "-Wimplicit-fallthrough",
        // We only use C style structs so this is not an issue. This can cause bugs
        // if we ever use types with constructors?
        "-Wno-reorder-init-list",
        // This causes annoying warnings with templated code.
        // Could try to enable it and then check if there are any interesting catches later.
        "-Wno-tautological-compare",
        // warn on variable length array usage
        "-Wvla",
        // these don't seem to catch much but enable anyways
        "-Wpointer-bool-conversion",
        "-Wbool-operation",
        // make sure switch on enum has a default
        "-Wswitch-default",
        // Treat warnings as errors.
        "-Werror",
        // Target micro architecture.
        "-march=native",
        // Use cpp standard.
        "-std=c++23",
        // we don't use libc but this might effect some compiler builtins?
        "-fno-math-errno",
        // this caused compilation errors so is disabled. could be an idea to re-enable it
        "-fno-stack-protector",
        // enable linker garbage collection
        "-ffunction-sections",
        "-fdata-sections",
        // disable strict aliasing
        "-fno-strict-aliasing",
        // disable undefined behavior on signed integer overflow
        "-fno-strict-overflow",
        // signed integer overflow is defined to wrap.
        "-fwrapv",
        // Don't delete `ptr == nullptr` kind of checks. And don't interpret passing nullptr on some places as
        // undefined behavior.
        "-fno-delete-null-pointer-checks",
        // disable some c++ features
        "-fno-exceptions",
        "-fno-rtti",
        "-nostdlib++",
        "-fno-threadsafe-statics",
        "-fomit-frame-pointer",
        "-momit-leaf-frame-pointer",
        // disable libc
        "-nostdlib",
        "-nostdinc",
        "--target=x86_64-linux-none",
    ];

    cflags.push(format!("-O{}", &cfg.opt_level).leak());

    cflags.push(format!("-I{}", &cfg.include_path.display()).leak());

    if cfg.debug_info {
        cflags.push("-g1");
        cflags.push("-gdwarf64");
        cflags.push("-gdwarf-5");
        cflags.push("-funwind-tables");
        cflags.push("-fasynchronous-unwind-tables");
        cflags.push("-DZX_DEBUG_INFO");
    } else {
        cflags.push("-fno-unwind-tables");
        cflags.push("-fno-asynchronous-unwind-tables");
    }

    if cfg.dev_mode {
        cflags.push("-DZX_DEV");
    }

    if cfg.fuzz {
        cflags.push("-DZX_FUZZ");
        cflags.push("-fsanitize-coverage=trace-pc-guard");
    }

    cflags
}

fn make_ldflags(cfg: &FlagCfg) -> Vec<&'static str> {
    let mut ldflags = vec![
        "-march=native",
        // use llvm linker
        "-fuse-ld=lld",
        // don't allow undefined references
        "-Wl,-z,defs",
        // remove unused sections
        "-Wl,--gc-sections",
        // disable some c++ features
        "-fno-exceptions",
        "-fno-rtti",
        "-nostdlib++",
        "-fno-threadsafe-statics",
        "-fomit-frame-pointer",
        "-momit-leaf-frame-pointer",
        "-static",
        // disable libc
        "-nostdlib",
        "-nostdinc",
        "--target=x86_64-linux-none",
    ];

    ldflags.push(format!("-O{}", cfg.opt_level).leak());

    if cfg.debug_info {
        ldflags.push("-g1");
        ldflags.push("-gdwarf64");
        ldflags.push("-gdwarf-5");
        ldflags.push("-funwind-tables");
        ldflags.push("-fasynchronous-unwind-tables");
        ldflags.push("-DZX_DEBUG_INFO");
    } else {
        ldflags.push("-fno-unwind-tables");
        ldflags.push("-fno-asynchronous-unwind-tables");
        ldflags.push("-Wl,-s");
    }

    ldflags
}

#[derive(Debug)]
struct Config {
    cxx: &'static str,
    ar: &'static str,
    clang_scan_deps: &'static str,
    cflags: &'static [&'static str],
    ldflags: &'static [&'static str],
    build_path: &'static Path,
    module_path: &'static Path,
    lib_path: &'static Path,
    bin_path: &'static Path,
}

fn parse_args() -> Config {
    let mut args = HashMap::<String, String>::new();

    for arg in std::env::args().skip(1) {
        let parts = arg.split('=').collect::<Vec<&str>>();

        match parts.len() {
            1 => {
                if args.insert(arg.clone(), "".to_owned()).is_some() {
                    panic!("Duplicate argument: {}", arg);
                }
            }
            2 => {
                if args
                    .insert(parts[0].to_owned(), parts[1].to_owned())
                    .is_some()
                {
                    panic!("Duplicate argument: {}", parts[0]);
                }
            }
            _ => {
                panic!("Invalid CLI argument: {}", arg);
            }
        }
    }

    let cxx = args.get("--cxx").map(|s| s.leak()).unwrap_or("clang++");
    let ar = args.get("--ar").map(|s| s.leak()).unwrap_or("llvm-ar");
    let clang_scan_deps = args
        .get("--clang-scan-deps")
        .map(|s| s.leak())
        .unwrap_or("clang-scan-deps");
    let opt_level = args.get("--opt-level").map(|s| s.leak()).unwrap_or("2");
    let dev_mode = get_bool_arg(&args, "dev-mode", true);
    let debug_info = get_bool_arg(&args, "debug-info", true);
    let fuzz = get_bool_arg(&args, "fuzz", false);
    let proj_dir = args.get("--proj-dir").map(|s| s.leak()).unwrap_or(".");
    let build_dir = args.get("--build-dir").map(|s| s.leak()).unwrap_or("build");

    let current_path = std::env::current_dir().unwrap();
    let proj_path = make_path_absolute(&current_path, Path::new(proj_dir));
    let build_path = make_path_absolute(&current_path, Path::new(build_dir));
    let include_path = proj_path.join("include");
    let module_path = proj_path.join("module").leak();
    let lib_path = proj_path.join("lib").leak();
    let bin_path = proj_path.join("bin").leak();

    let flag_cfg = FlagCfg {
        opt_level: opt_level.to_owned(),
        dev_mode,
        debug_info,
        fuzz,
        include_path,
    };
    let cflags = make_cflags(&flag_cfg).leak();
    let ldflags = make_ldflags(&flag_cfg).leak();

    return Config {
        cxx: cxx.to_owned(),
        ar: ar.to_owned(),
        clang_scan_deps: clang_scan_deps.to_owned(),
        build_path,
        module_path,
        lib_path,
        bin_path,
        cflags,
        ldflags,
    };
}

trait Target {}

struct Module {}
impl Target for Module {}

struct ModuleFile {}
impl Target for ModuleFile {}

struct ModuleTest {}
impl Target for ModuleTest {}

struct Lib {}
impl Target for Lib {}

struct LibFile {}
impl Target for LibFile {}

struct LibTest {}

struct Bin {}
struct BinFile {}

struct State {
    cfg: Config,
    path_map: HashMap<&'static str, u32>,
    entities: Vec<&'static dyn Target>,
    deps: Vec<Vec<u32>>,
    rev_deps: Vec<Vec<u32>>,
    is_ready: Vec<bool>,
    path: Vec<&'static str>,
}

type StateRef = &'static RefCell<State>;
type LocalExRef = &'static smol::LocalExecutor<'static>;

fn parse_module_name(base_name: &'static str, file_path: &Path) -> &'static str {
    let ext = file_path.extension().unwrap();
    assert_eq!(ext, "ccm");

    let name = file_path.file_prefix().unwrap();

    if name == base_name {
        base_name
    } else {
        format!("{}:{}", base_name, name).leak()
    }
}

async fn parse_module_file(local_ex: LocalExRef, state: StateRef) {
    parse_module_name();
    run_clang_deps();
    parse_clang_deps();
    calculate_paths();
}

async fn parse_module_test(local_ex: LocalExRef, state: StateRef) {}

async fn parse_modules(local_ex: LocalExRef, state: StateRef) {
    let mut mods_dir = smol::fs::read_dir(state.cfg.module_path).await.unwrap();

    let concurrency = num_cpus::get();
    let mut futs = futures::FuturesUnordered::new();

    while let Some(mod_entry) = mods_dir.next().await {
        let mod_entry = mod_entry.unwrap();

        let src_path = mod_entry.path().join("src");
        let test_path = mod_entry.path().join("test");
        let deps_json_path = mod_entry.path().join("deps.json");

        futs.push(local_ex.spawn(parse_module_deps_json));

        let mut src_dir = smol::fs::read_dir(src_path).await.unwrap();

        while let Some(entry) = dir.next().await {
            if futs.len() >= concurrency {
                futs.next().await.unwrap();
            }
            futs.push(local_ex.spawn(parse_module_file));
        }

        let mut test_dir = smol::fs::read_dir(test_path).await.unwrap();

        while let Some(entry) = dir.next().await {
            if futs.len() >= concurrency {
                futs.next().await.unwrap();
            }
            futs.push(local_ex.spawn(parse_module_test));
        }
    }

    while futs.len() > 0 {
        futs.next().await.unwrap();
    }
}

async fn run(local_ex: LocalExRef, state: StateRef) {
    parse_modules(local_ex, state).await;
}

fn main() {
    let cfg = parse_args();
    dbg!(&cfg);

    let state = Box::leak(Box::new(RefCell::new(State {
        cfg,
        path_map: HashMap::new(),
        entities: Vec::new(),
        deps: Vec::new(),
        rev_deps: Vec::new(),
        is_ready: Vec::new(),
        path: Vec::new(),
    })));

    let local_ex = Box::leak(Box::new(smol::LocalExecutor::new()));

    local_ex.run(run(local_ex, state));
}
