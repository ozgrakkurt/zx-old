package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"log"
	"os"
	"os/exec"
	"path"
	"runtime"
	"strings"
)

type FlagConfig struct {
	optLevel  string
	devMode   bool
	debugInfo bool
	fuzz      bool
}

func makeCflags(cfg FlagConfig) []string {
	cflags := []string{
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
	}

	cflags = append(cflags, fmt.Sprintf("-O%s", cfg.optLevel))

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
		cflags = append(cflags, "-fsanitize-coverage=trace-pc-guard")
	}

	return cflags
}

func makeLdflags(cfg FlagConfig) []string {
	ldflags := []string{
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

	ldflags = append(ldflags, fmt.Sprintf("-O%s", cfg.optLevel))

	if cfg.debugInfo {
		ldflags = append(ldflags, "-g1")
		ldflags = append(ldflags, "-gdwarf64")
		ldflags = append(ldflags, "-gdwarf-5")
		ldflags = append(ldflags, "-funwind-tables")
		ldflags = append(ldflags, "-fasynchronous-unwind-tables")
		ldflags = append(ldflags, "-DZX_DEBUG_INFO")
	} else {
		ldflags = append(ldflags, "-fno-unwind-tables")
		ldflags = append(ldflags, "-fno-asynchronous-unwind-tables")

		ldflags = append(ldflags, "-Wl,-s")
	}

	return ldflags
}

type Config struct {
	cxx           string
	clangScanDeps string
	cflags        []string
	ldflags       []string
	buildDirPath  string
	srcDirPath    string
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

func readConfig() Config {
	var cxx = flag.String("cxx", "clang++", "c++ compiler")
	var clangScanDeps = flag.String("clang-scan-deps", "clang-scan-deps", "clang-scan-deps executable")
	var optLevel = flag.String("opt-level", "0", "optimization ('-O') flag value for compiling c++ code")
	var devMode = flag.Bool("dev-mode", true, "enable development mode or not")
	var debugInfo = flag.Bool("debug-info", true, "include debug info in the output executables")
	var fuzz = flag.Bool("fuzz", false, "enable fuzz testing instrumentation")
	var buildDir = flag.String("build-dir", "build", "directory for outputting build artifacts")
	var srcDir = flag.String("src-dir", "src", "source code directory")

	flag.Parse()

	flagCfg := FlagConfig{
		optLevel:  *optLevel,
		devMode:   *devMode,
		debugInfo: *debugInfo,
		fuzz:      *fuzz,
	}

	cflags := makeCflags(flagCfg)
	ldflags := makeLdflags(flagCfg)

	currentPath, err := os.Getwd()
	if err != nil {
		log.Fatalf("Failed to get current working directory: %v", err)
	}

	buildDirPath := makePathAbsolute(currentPath, *buildDir)
	srcDirPath := makePathAbsolute(currentPath, *srcDir)

	cfg := Config{
		cxx:           *cxx,
		clangScanDeps: *clangScanDeps,
		cflags:        cflags,
		ldflags:       ldflags,
		buildDirPath:  buildDirPath,
		srcDirPath:    srcDirPath,
	}

	return cfg
}

type SrcKind int

const (
	Module SrcKind = iota
	Test
	Bin
)

type SourceFile struct {
	name string
	deps []string
	kind SrcKind
}

func parseSrcFileName(name string) SourceFile {
	extParts := strings.Split(name, ".")

	if len(extParts) != 2 {
		log.Fatalf("Failed to parse filename: '%v'. It has to have a single dot ('.') character.", name)
	}

	fileName := extParts[0]
	ext := extParts[1]

	nameParts := strings.Split(fileName, "_")

	if len(nameParts) == 0 {
		log.Fatalf("Empty name in: %v", name)
	}

	isTest := "test" == nameParts[len(nameParts)-1]

	if ext == "cppm" {
		if isTest {
			log.Fatalf("Module file ('.cppm') name can't end with 'test' suffix.")
		}

		if len(nameParts) == 1 {
			return SourceFile{
				name: nameParts[0],
				deps: make([]string, 0),
				kind: Module,
			}
		} else if len(nameParts) == 2 {
			return SourceFile{
				name: nameParts[0] + ":" + nameParts[1],
				deps: make([]string, 0),
				kind: Module,
			}
		} else {
			log.Fatalf("Module file ('.cppm') is expected to have only one or zero '_' character in it.")
		}
	} else if ext == "cc" {
		if isTest {
			return SourceFile{
				name: fileName,
				deps: make([]string, 0),
				kind: Test,
			}
		} else {
			return SourceFile{
				name: fileName,
				deps: make([]string, 0),
				kind: Bin,
			}
		}
	} else {
		log.Fatalf("Unknown extension '%v' on file '%v'", ext, name)
	}

	panic("not possible")
}

type DepsOutReq struct {
	LogicalName string `json:"logical-name"`
}

type DepsOutRule struct {
	Requires []DepsOutReq `json:"requires"`
}

type DepsOut struct {
	Rules []DepsOutRule `json:"rules"`
}

func parseSrcDeps(cfg Config, srcAbsPath string) []string {
	args := []string{"-format=p1689", "--", cfg.cxx}
	args = append(args, cfg.cflags...) // Assuming cfg.cflags is a []string
	args = append(args, srcAbsPath)

	cmd := exec.Command(cfg.clangScanDeps, args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to parse dependencies of %v: %v: %s", srcAbsPath, err, output)
	}

	out := DepsOut{}
	err = json.Unmarshal(output, &out)
	if err != nil {
		log.Fatalf("Failed to deserialize scan-deps output for %v: %v", srcAbsPath, err)
	}

	deps := make([]string, 0)

	for _, rule := range out.Rules {
		for _, req := range rule.Requires {
			deps = append(deps, req.LogicalName)
		}
	}

	return deps
}

func goParseSrcDeps(cfg Config, srcFileName string, finish chan bool, srcChannel chan SourceFile) {
	srcAbsPath := path.Join(cfg.srcDirPath, srcFileName)
	src := parseSrcFileName(srcFileName)
	src.deps = parseSrcDeps(cfg, srcAbsPath)
	finish <- true
	srcChannel <- src
}

func goProcessSrcDir(cfg Config, srcChannel chan SourceFile) {
	defer close(srcChannel)

	srcDir, err := os.Open(cfg.srcDirPath)
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
	finish := make(chan bool, 100)
	nParseRunning := 0
	parseConcurrency := runtime.NumCPU()
	for {
		entries, err := srcDir.ReadDir(chunkSize)

		if err != nil && err != io.EOF {
			log.Fatalf("Failed to iterate source directory: %v", err)
		}

		for _, entry := range entries {
			if nParseRunning >= parseConcurrency {
				<-finish
				nParseRunning -= 1
			}

			go goParseSrcDeps(cfg, entry.Name(), finish, srcChannel)
			nParseRunning += 1
		}

		if err == io.EOF {
			break
		}
	}

	for nParseRunning > 0 {
		<-finish
		nParseRunning -= 1
	}
}

func main() {
	cfg := readConfig()

	err := os.RemoveAll(cfg.buildDirPath)
	if err != nil {
		log.Fatalf("Failed to clean the build directory: %v", err)
	}

	srcChannel := make(chan SourceFile, 100)
	go goProcessSrcDir(cfg, srcChannel)

	// main loop
	for {
		srcFile, more := <-srcChannel
		if !more {
			log.Fatalf("Finished")
		} else {
			fmt.Printf("%#v\n", srcFile)
		}
	}

	// map out module dependencies per module
	// compile everything in parallel and run what user requested in parallel too
}
