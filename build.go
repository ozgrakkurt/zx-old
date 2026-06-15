package main

import (
	"flag"
	"fmt"
	"path"
	"os"
	"log"
	"io"
	"strings"
	"os/exec"
	"encoding/json"
)

type Config struct {
	cxx string
	clangScanDeps string
	cflags string
	ldflags string
	buildDir string
	srcDir string
}

func makeCflags(cfg Config) string {
	cflags := []string {
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
        "--std=c++23",
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
	}

    cflags = append(cflags, fmt.Sprintf("-O%d", config.optLevel))

	if cfg.debugInfo {
		cflags = append(cflags, "-g1")
		cflags = append(cflags, "-gdwarf64")
		cflags = append(cflags, "-gdwarf-5")
		cflags = append(cflags, "-funwind-tables")
		cflags = append(cflags, "-fasynchronous-unwind-tables")
		cflags = append(cflags, "-DZX_DEBUG_INFO")
	} else {
		cflags = append(cflags, "-fno-unwind-tables")
		cflags = append(cflags, "-fno-asynchronous-unwind-tables")
	}

	if cfg.devMode {
		cflags = append(cflags, "-DZX_DEV")
	}

	if cfg.fuzz {
		cflags = append(cflags, "-DZX_FUZZ")
		cflag = append(cflags, "-fsanitize-coverage-trace=pc-guard")
	}

	if cfg.thin_lto {
		cflags.append("-flto=thin")
	}

	return strings.Join(cflags, " ")
}

func makeLdflags(cfg Config) string {
	ldflags := []string {
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
	}

    ldflags = append(ldflags, fmt.Sprintf("-O%d", config.optLevel))
}

func readConfig() Config {
	var cxx = flag.String("cxx", "clang++", "c++ compiler")
	var clangScanDeps = flag.String("clang-scan-deps", "clang-scan-deps", "clang-scan-deps executable")
	var thinLto = flag.Bool("thin-lto", false, "enable clang thin-lto or not")
	var optLevel  = flag.Int("opt-level", 0, "optimzation level for compiling c++ code")
	var devMode = flag.Bool("dev-mode", true, "enable development mode or not")
	var debugInfo = flag.Bool("debug-info", true, "include debug info in the output executables")
	var fuzz = flag.Bool("fuzz", false, "enable fuzz testing instrumentation")
	var buildDir  = flag.String("build-dir", "build", "directory for outputting build artifacts")
	var srcDir = flag.String("src-dir", "src", "source code directory")

    flag.Parse()

    cfg := Config {
    	cxx: *cxx,
    	clangScanDeps: *clangScanDeps,
    	thinLto: *thinLto,
    	optLevel: *optLevel,
    	devMode: *devMode,
    	debugInfo: *debugInfo,
    	fuzz: *fuzz,
    	buildDir: *buildDir,
    	srcDir: *srcDir,
    }

    return cfg
}

// Convert a path (`pathval`) to an absolute path using `defaultBase`.
// The path in `pathval` will be returned without modification if it is already absolute.
func makePathAbsolute(defaultBase string, pathval string) string {
	if path.IsAbs(pathval) {
		return pathval
	} else {
		return path.Join(defaultBase, pathval)
	}
}

type SrcKind int

const (
	Module SrcKind = iota
	Test
	Cpp	
)

type SourceFile struct {
	name string
	deps []string
	kind SrcKind
}

func parseSrcFileName(name string) SourceFile {
	extParts := strings.Split(entry.Name(), ".")

	if len(extParts) != 2 {
		log.Fatalf("Failed to parse filename: '%v'. It has to have a single dot ('.') character.", entry.Name())
	}

	fileName := extParts[0]
	ext := extParts[1]

	nameParts := strings.Split(fileName, "_")

	if ext == "cppm" {
		if strings.HasSuffix(fileName, "_test") {
			log.Fatalf("Module file ('.cppm') can't have '_test' suffix.")
		}
	} else if ext == "cc" {
		if strings.HasSuffix(fileName, "_test") {
		} else {
			
		}
	} else {
		log.Fatalf("Unknown extension '%v' on file '%v'", ext, entry.Name())
	}
}

type DepsOutReq struct {
	logical_name string `json:"logical-name"`
}

type DepsOutRule struct {
	requires []DepsOutReq
}

type DepsOut struct {
	rules []DepsOutRule
}

func parseSrcDeps(cfg Config, srcAbsPath string) []string {
	output, err := exec.CommandContext(cfg.clangScanDeps, "-format=p1689", "--", cfg.cxx, cfg.cflags).CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to parse dependencies of %v: %v", srcAbsPath, err)
	}

	// TODO: parse output
	out := DepsOut{}
	err = json.Unmarshal(output, &out)
	if err != nil {
		log.Fatalf("Failed to deserialize scan-deps output for %v: %v", srcAbsPath, err)
	}

	return make([]string, 0)
}

func goParseSrcDeps(srcDirAbsPath string, srcFileName string, finish chan bool, srcChannel chan SourceFile) {
	srcAbsPath := path.Join(srcDirAbsPath, srcFileName)
	src := parseSrcFileName(srcFileName)
	src.deps = parseSrcDeps(srcAbsPath)
	finish <- true
	srcChannel <- src
}

func goProcessSrcDir(srcDirPath string, srcChannel chan SourceFile) {
	defer close(srcChannel)
	
	srcDir, err := os.Open(srcDirPath)
	if err != nil {
		log.Fatalf("Failed to open source directory: %v", err)
	}
	defer srcDir.Close()

	fileInfo, err := srcDir.Stat()
	if err != nil {
		log.Fatalf("Failed to stat the source directory: %v", err)
	}

	if !fileInfo.IsDir() {
		log.Fatalf("Source directory isn't a directory: %v", err)
	}

	chunkSize := 100
	for {
		entries, err := srcDir.ReadDir(chunkSize)

		if err != nil && err != io.EOF {
			log.Fatalf("Failed to iterate source directory: %v", err)
		}

		for _, entry := range entries {
			srcFile := parseSrcFileName(entry.Name())

			absPath := path.Join(srcDirPath, entry.Name())
			srcFile.deps = parseSrcDeps(absPath)
			
			srcChannel <- srcFile
		}

		if err == io.EOF {
			break
		}
	}
}

func main() {
	cfg := readConfig()

	currentPath, err := os.Getwd()
	if err != nil {
		log.Fatalf("Failed to get current working directory: %v", err)
	}

	buildDirPath := makePathAbsolute(currentPath, cfg.buildDir)
	srcDirPath :=   makePathAbsolute(currentPath, cfg.srcDir)

	err = os.RemoveAll(buildDirPath)
	if err != nil {
		log.Fatalf("Failed to clean the build directory: %v", err)
	}

	srcChannel := make(chan SourceDeps, 100)
	go iterateSource(srcDirPath, srcChannel)

	// main loop
	for {
		deps, more := <-depsChannel
		if !more {
			log.Fatalf("Finished")
		} else {
			fmt.Println(deps)
		}
	}

	// map out module dependencies per module
	// compile everything in parallel and run what user requested in parallel too
}
