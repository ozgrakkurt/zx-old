#![allow(clippy::single_match)]

use smol::prelude::*;
use std::cell::RefCell;
use std::collections::{HashMap, HashSet};
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
    src_path: &'static Path,
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

    let cxx = args
        .get("--cxx")
        .map(|s| -> &'static str { s.leak() })
        .unwrap_or("clang++");
    let ar = args.get("--ar").map(|s| s.leak()).unwrap_or("llvm-ar");
    let clang_scan_deps = args
        .get("--clang-scan-deps")
        .map(|s| s.leak())
        .unwrap_or("clang-scan-deps");
    let opt_level = args.get("--opt-level").map(|s| s.leak()).unwrap_or("2");
    let dev_mode = get_bool_arg(&args, "dev-mode", true);
    let debug_info = get_bool_arg(&args, "debug-info", true);
    let fuzz = get_bool_arg(&args, "fuzz", false);
    let src_dir = args.get("--src-dir").map(|s| s.leak()).unwrap_or("src");
    let build_dir: &'static str = args.get("--build-dir").map(|s| s.leak()).unwrap_or("build");

    let current_path = std::env::current_dir().unwrap();
    let src_path = make_path_absolute(&current_path, Path::new(src_dir)).leak();
    let build_path = make_path_absolute(&current_path, Path::new(build_dir)).leak();

    let flag_cfg = FlagCfg {
        opt_level: opt_level.to_owned(),
        dev_mode,
        debug_info,
        fuzz,
    };
    let cflags = make_cflags(&flag_cfg).leak();
    let ldflags = make_ldflags(&flag_cfg).leak();

    return Config {
        cxx: cxx.to_owned().leak(),
        ar: ar.to_owned().leak(),
        clang_scan_deps: clang_scan_deps.to_owned().leak(),
        build_path,
        src_path,
        cflags,
        ldflags,
    };
}

struct Mod {
    name: &'static str,
    cppm: Vec<String>,
    cpp: Vec<String>,
    bin: Vec<String>,
    test: Vec<String>,
    deps: Vec<u32>,
    bin_deps: Vec<u32>,
    test_deps: Vec<u32>,
}

struct State {
    cfg: Config,
    name_map: HashTable<&'static str, u32>,
    mods: Vec<Mod>,
    deps: Vec<Vec<u32>>,
    rev_deps: Vec<Vec<u32>>,
    is_built: Vec<bool>,
    is_discovered: Vec<bool>,
}

impl State {
    fn get_or_push_mod(&mut self, name: &str) -> u32 {
        if let Some(index) = self.name_map.get(name) {
            return *index;
        }
        let index = u32::try_from(self.mods.len()).unwrap();
        let name = name.to_owned().leak();
        self.name_map.insert(name, id);
        self.mods.push(Mod {
            name: name,
            cppm: Vec::new(),
            cpp: Vec::new(),
            asm: Vec::new(),
            bin: Vec::new(),
            test: Vec::new(),
            deps: Vec::new(),
            test_deps: Vec::new(),
        });

        self.deps.push(Vec::new());
        self.rev_deps.push(Vec::new());
        self.is_built.push(false);
        self.is_discovered.push(false);
        return index;
    }

    fn push_mod(&mut self, module: Mod) -> u32 {
        let id = self.get_or_push_mod(module.name);
        let idx = usize::try_from(id).unwrap();
        assert!(!self.is_discovered[idx]);
        self.is_discovered[idx] = true;

        let mut deps = Vec::<u32>::with_capacity(
            module.deps.len() + module.bin_deps.len() + module.test_deps.len(),
        );
        deps.extend_from_slice(module.deps.as_slice());
        deps.extend_from_slice(module.bin_deps.as_slice());
        deps.extend_from_slice(module.test_deps.as_slice());
        self.deps[idx] = deps;

        return id;
    }
}

type StateRef = &'static RefCell<State>;
type LocalExRef = &'static smol::LocalExecutor<'static>;

#[derive(Deserialize)]
struct ZXJson {
    deps: Vec<u32>,
    bin_deps: Vec<u32>,
    test_deps: Vec<u32>,
}

async fn read_and_parse_zx_json(state: StateRef, file_path: &Path) -> ZXJson {
    let contents = smol::fs::read(file_path).await.unwrap();
    let val = serde_json::from_slice::<serde_json::Value>(&contents).unwrap();
    let obj = val.as_obj().unwrap();
    let mut deps = Vec::<u32>::new();
    let mut bin_deps = Vec::<u32>::new();
    let mut test_deps = Vec::<u32>::new();

    let state = state.borrow().unwrap();

    for val in obj.get("deps").unwrap().as_array().unwrap() {
        let name = val.as_str().unwrap();
        deps.push(state.get_or_push_mod(name));
    }

    for val in obj.get("bin_deps").unwrap().as_array().unwrap() {
        let name = val.as_str().unwrap();
        bin_deps.push(state.get_or_push_mod(name));
    }

    for val in obj.get("test_deps").unwrap().as_array().unwrap() {
        let name = val.as_str().unwrap();
        test_deps.push(state.get_or_push_mod(name));
    }

    return ZXJson {
        deps,
        bin_deps,
        test_deps,
    };
}

async fn scan_mod_deps(
    state: StateRef,
    index: usize,
    file_path: PathBuf,
    expected_to_provide: Option<String>,
) -> (usize, Vec<String>) {
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

    (index, deps)
}

fn parse_cpp_name(file_path: &Path) -> String {
    let ext = file_path.extension().unwrap();
    assert_eq!(ext, "cpp");

    let name = file_path.file_prefix().unwrap().to_str().unwrap();

    assert!(!name.contains(':'));
    assert!(!name.contains('-'));
    assert!(!name.is_empty());

    name.to_owned()
}

fn parse_cppm_name(base_name: &'static str, file_path: &Path) -> String {
    let ext = file_path.extension().unwrap();
    assert_eq!(ext, "cppm");

    let name = file_path.file_prefix().unwrap().to_str().unwrap();

    assert!(!name.contains(':'));
    assert!(!name.contains('-'));
    assert!(!name.is_empty());

    name.to_owned()
}

async fn parse_and_register_bin_file() {}

async fn parse_and_register_test_file() {}

async fn parse_and_register_mod_file(
    local_ex: LocalExRef,
    state: StateRef,
    id: u32,
    base_name: &'static str,
    mod_file_name: &'static str,
    path: PathBuf,
) {
    let mod_name = parse_mod_file_name(base_name, &path);

    let state = state.borrow_mut().unwrap();
    let self_id = state.push_mod(path);

    let deps = scan_mod_deps(state, &path, &mod_name);

    let state = state.borrow_mut().unwrap();
    let idx = usize::try_from(id).unwrap();

    for dep in dependencies {
        let dep_path: &'static str = if let Some(p) = state.intern_set.get(dep.as_str()) {
            p
        } else {
            let dl = dep.leak();
            state.intern_set.insert(dl);
            dl
        };
        let dep_id = state.push_or_get_path(dep_path);
        state.rev_deps[dep_id].push(self_id);
        deps.push(dep_id);
    }

    assert_eq!(self_id, state.push_target(target, path, deps));
}

async fn parse_and_register_module(local_ex: LocalExRef, state: StateRef, base_path: PathBuf) {
    let mod_path = mod_path.join("mod");
    let lib_path = mod_path.join("lib");
    let test_path = mod_path.join("test");
    let bin_path = mod_path.join("bin");
    let zx_json_path = mod_path.join("zx.json");

    let zx_json = read_and_parse_zx_json(state, &zx_json_path).await;

    let base_name = base_path.file_name().unwrap().to_str().unwrap();

    let mut cpp = Vec::new();
    let mut cppm = Vec::new();
    let mut test = Vec::new();
    let mut bin = Vec::new();

    let mut futs = futures::future::FuturesUnordered::new();
    let concurrency = 4;

    let (id, base_name): (u32, &'static str) = {
        let state = state.borrow_mut().unwrap();
        let id = state.get_or_push_mod(base_name);
        (id, state.mods[id].name)
    };

    let mut mod_dir = smol::fs::read_dir(mod_path).await.unwrap();
    while let Some(entry) = mod_dir.next().await {
        if futs.len() >= concurrency {
            futs.next().await.unwrap();
        }
        let path = entry.path().to_owned();
        let cppm_name = parse_cppm_name();
        futs.push(local_ex.spawn(parse_and_register_mod_file(state, id, base_name, path)));
    }

    while let Some(entry) = dir.next().await {
        if futs.len() >= concurrency {
            futs.next().await.unwrap();
        }
        let path = file_path.to_str().unwrap().to_owned().leak();
        futs.push(local_ex.spawn(parse_and_register_module_file(state, base_name, path)));
    }

    let mut test_dir = smol::fs::read_dir(test_path).await.unwrap();

    while let Some(entry) = dir.next().await {
        if futs.len() >= concurrency {
            futs.next().await.unwrap();
        }
        futs.push(local_ex.spawn(parse_module_test));
    }

    while futs.len() > 0 {
        futs.next().await.unwrap();
    }
}

async fn parse_and_register_modules(local_ex: LocalExRef, state: StateRef) {
    let mut src_dir = smol::fs::read_dir(state.cfg.src_path).await.unwrap();

    let concurrency = num_cpus::get();
    let mut futs = futures::future::FuturesUnordered::new();

    while let Some(mod_entry) = src_dir.next().await {
        let mod_entry = mod_entry.unwrap();
        let mod_path = mod_entry.path().to_owned();

        if futs.len() >= concurrency {
            futs.next().await.unwrap();
        }

        futs.push(local_ex.spawn(parse_module(local_ex, state, mod_path)));
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
        interner: Interner::new(),
        path_map: HashMap::new(),
        targets: Vec::new(),
        deps: Vec::new(),
        rev_deps: Vec::new(),
        is_ready: Vec::new(),
        path: Vec::new(),
    })));

    let local_ex = Box::leak(Box::new(smol::LocalExecutor::new()));

    local_ex.run(run(local_ex, state));
}
