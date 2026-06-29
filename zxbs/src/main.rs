#![allow(clippy::single_match)]

use serde::Deserialize;
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

struct ModuleFile {
    module_name: &'static str,
}
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
    path_intern_set: HashSet<&'static str>,
    path_map: HashMap<&'static str, u32>,
    targets: Vec<Option<&'static dyn Target>>,
    deps: Vec<Vec<u32>>,
    rev_deps: Vec<Vec<u32>>,
    is_ready: Vec<bool>,
    path: Vec<&'static str>,
}

impl State {
    fn push_or_get_path(&mut self, path: &'static str) -> u32 {
        if let Some(id) = self.path_map.get(path) {
            return *id;
        }
        let id = u32::try_from(self.targets.len()).unwrap();
        self.path_map.insert(path, id);
        self.targets.push(None);
        self.deps.push(Vec::new());
        self.rev_deps.push(Vec::new());
        self.is_ready.push(false);
        self.path.push(path);
        return id;
    }

    fn push_target(&mut self, tgt: &'static dyn Target, path: &'static str, deps: Vec<u32>) -> u32 {
        let id = self.push_or_get_path(path);
        assert!(self.targets[id].is_none());
        self.targets[id] = Some(tgt);
        self.deps[id] = deps;
        return id;
    }
}

type StateRef = &'static RefCell<State>;
type LocalExRef = &'static smol::LocalExecutor<'static>;

#[derive(Deserialize)]
struct DepsFile {
    dependencies: Vec<String>,
    test_dependencies: Vec<String>,
}

async fn read_deps_file(file_path: &Path) -> DepsFile {
    let contents = smol::fs::read(file_path).await.unwrap();
    let val = serde_json::from_slice::<serde_json::Value>(&contents).unwrap();
    let obj = val.as_obj().unwrap();
    let mut dependencies = Vec::<String>::new();
    let mut test_dependencies = Vec::<String>::new();
    for val in obj.get("dependencies").unwrap().as_array().unwrap() {
        dependencies.push(val.as_str().unwrap().to_owned());
    }

    for val in obj.get("test_dependencies").unwrap().as_array().unwrap() {
        test_dependencies.push(val.as_str().unwrap().to_owned());
    }

    return DepsFile {
        dependencies,
        test_dependencies,
    };
}

async fn parse_dependencies(
    state: StateRef,
    file_path: &Path,
    expected_to_provide: Option<&str>,
) -> Vec<String> {
    let cmd = {
        let state = state.borrow().unwrap();
        let mut cmd = smol::process::Command::new(state.cfg.clang_scan_deps);

        cmd.arg("-format=p1689");
        cmd.arg("--");
        cmd.arg(state.cfg.cxx);

        cmd.args(state.cfg.cflags);
        cmd.args(file_path.to_str().unwrap());

        cmd
    };

    let output = cmd.output().await.unwrap();
    assert_eq!(output.status.code().unwrap(), 0);
    let json = serde_json::from_slice::<serde_json::Value>(output.stdout.as_slice()).unwrap();

    let rules = json
        .as_obj()
        .unwrap()
        .get("rules")
        .unwrap()
        .as_arr()
        .unwrap();

    if let Some(expected) = expected_to_provide {
        let mut found = false;
        for rule in rules {
            let rule = rule.as_obj().unwrap();

            if let Some(provides) = rule.get("provides") {
                for val in provides.as_arr().unwrap() {
                    let name = val.as_obj().unwrap().get("logical-name").unwrap();

                    if name == expected {
                        found = true;
                        break;
                    }
                }
            }
        }
        assert!(found);
    }

    let mut deps = Vec::<String>::new();

    for rule in rules {
        let rule = rule.as_obj().unwrap();

        if let Some(requires) = rules.get("requires") {
            for val in requires.as_arr().unwrap() {
                let name = val.as_obj().unwrap().get("logical-name").unwrap();
                deps.push(name.to_owned());
            }
        }
    }

    return deps;
}

fn parse_cc_name(file_path: &Path) -> &'static str {
    let ext = file_path.extension().unwrap();
    assert_eq!(ext, "cc");
    file_path.file_prefix().unwrap().to_owned().leak()
}

fn parse_module_name(base_name: &'static str, file_path: &Path) -> &'static str {
    let ext = file_path.extension().unwrap();
    assert_eq!(ext, "ccm");

    let name = file_path.file_prefix().unwrap();

    assert!(!name.contains(':'));
    assert!(!name.contains('-'));
    assert!(!name.is_empty());

    if name == base_name {
        base_name
    } else {
        format!("{}:{}", base_name, name).leak()
    }
}

async fn parse_and_register__module_file(
    local_ex: LocalExRef,
    state: StateRef,
    base_name: &'static str,
    path: &'static str,
) {
    let file_path = Path::new(path);
    let mod_name = parse_module_name(base_name, file_path);
    let dependencies = parse_dependencies(state, file_path, Some(mod_name)).await;

    let target: &'static dyn Target = Box::leak(Box::new(ModuleFile {
        module_name: mod_name,
    }));

    let state = state.borrow_mut().unwrap();

    let mut deps = Vec::<u32>::new();

    let self_id = state.push_or_get_path(path);

    for dep in dependencies {
        let dep_path: &'static str = if let Some(p) = state.path_intern_set.get(dep.as_str()) {
            p
        } else {
            let dl = dep.leak();
            state.path_intern_set.insert(dl);
            dl
        };
        let dep_id = state.push_or_get_path(dep_path);
        state.rev_deps[dep_id].push(self_id);
        deps.push(dep_id);
    }

    assert_eq!(self_id, state.push_target(target, path, deps));
}

async fn parse_and_register_module_test(
    local_ex: LocalExRef,
    state: StateRef,
    base_name: &'static str,
    path: &'static str,
) {
    let file_path = Path::new(path);
}

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
            let path = file_path.to_str().unwrap().to_owned().leak();
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
