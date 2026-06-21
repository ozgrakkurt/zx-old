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
	"sync"
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

	cflags = append(cflags, "-O"+cfg.optLevel)

	cflags = append(cflags, "-I"+cfg.includeDirPath)

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
	moduleDirPath string
	libDirPath    string
	binDirPath    string
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
	var moduleDir = flag.String("module-dir", "module", "directory for modules")
	var libDir = flag.String("lib-dir", "lib", "directory for libraries")
	var binDir = flag.String("bin-dir", "bin", "directory for binary targets")
	var includeDir = flag.String("include-dir", "include", "directory for header include path")

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
	moduleDirPath := makePathAbsolute(currentPath, *moduleDir)
	libDirPath := makePathAbsolute(currentPath, *libDir)
	binDirPath := makePathAbsolute(currentPath, *binDir)
	includeDirPath := makePathAbsolute(currentPath, *includeDir)

	cfg := Config{
		cxx:            *cxx,
		clangScanDeps:  *clangScanDeps,
		cflags:         cflags,
		ldflags:        ldflags,
		buildDirPath:   buildDirPath,
		moduleDirPath:  moduleDirPath,
		libDirPath:     libDirPath,
		binDirPath:     binDirPath,
		includeDirPath: includeDirPath,
	}

	return cfg
}

func parseCCName(fileName string) string {
	extParts := strings.Split(fileName, ".")

	if len(extParts) != 2 {
		log.Fatalf("Failed to parse filename: '%v'. It has to have a single dot ('.') character.", fileName)
	}

	name := extParts[0]
	ext := extParts[1]

	if ext != "cc" {
		log.Fatalf("File on c++ folder has an unexpected extension: %v. Expected it to have '.cc'", fileName)
	}

	if name == "" {
		log.Fatalf("Empty name isn't allowed for cc files.")
	}

	return name
}

func parseModuleName(baseName string, fileName string) []string {
	extParts := strings.Split(fileName, ".")

	if len(extParts) != 2 {
		log.Fatalf("Failed to parse filename: '%v'. It has to have a single dot ('.') character.", fileName)
	}

	name := extParts[0]
	ext := extParts[1]

	if strings.Contains(name, "-") {
		log.Fatalf("Module file name can't contain '-'. Found '-' character in this name: %s", fileName)
	}

	if strings.Contains(name, ":") {
		log.Fatalf("Module file name can't contain ':'. Found ':' character in this name: %s", fileName)
	}

	if ext != "cppm" {
		log.Fatalf("File on module folder has an unexpected extension: %v. Expected it to have '.cppm'", fileName)
	}

	if name == "" {
		log.Fatalf("Empty name isn't allowed for modules.")
	}

	if baseName == name {
		return []string{name}
	} else {
		return []string{baseName, name}
	}
}

type DepsFile struct {
	Dependencies     []string `json:"dependencies"`
	TestDependencies []string `json:"test_dependencies"`
}

func parseDepsFile(filePath string) DepsFile {
	content, err := os.ReadFile(filePath)
	if err != nil {
		log.Fatalf("Failed to read ")
	}
	out := DepsFile{}
	err = json.Unmarshal(content, &out)
	if err != nil {
		log.Fatalf("Failed to parse dependency file: %s: %v", filePath, err)
	}
	return out
}

type ModDepsReq struct {
	LogicalName string `json:"logical-name"`
}

type ModDepsProvides struct {
	LogicalName string `json:"logical-name"`
}

type ModDepsRule struct {
	Requires []ModDepsReq      `json:"requires"`
	Provides []ModDepsProvides `json:"provides"`
}

type ModDeps struct {
	Rules []ModDepsRule `json:"rules"`
}

func parseCCDependencies(cfg Config, filePath string) []string {
	args := []string{"-format=p1689", "--", cfg.cxx}
	args = append(args, cfg.cflags...)
	args = append(args, filePath)

	cmd := exec.Command(cfg.clangScanDeps, args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to parse dependencies of %v: %v: %s", filePath, err, output)
	}

	out := ModDeps{}
	err = json.Unmarshal(output, &out)
	if err != nil {
		log.Fatalf("Failed to deserialize scan-deps output for %v: %v", filePath, err)
	}

	deps := []string{}

	for _, rule := range out.Rules {
		for _, req := range rule.Requires {
			deps = append(deps, req.LogicalName)
		}
	}

	return deps
}

func parseModuleDependencies(cfg Config, filePath string, moduleName []string) []string {
	args := []string{"-format=p1689", "--", cfg.cxx}
	args = append(args, cfg.cflags...)
	args = append(args, filePath)

	cmd := exec.Command(cfg.clangScanDeps, args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to parse dependencies of %v: %v: %s", filePath, err, output)
	}

	out := ModDeps{}
	err = json.Unmarshal(output, &out)
	if err != nil {
		log.Fatalf("Failed to deserialize scan-deps output for %v: %v", filePath, err)
	}

	deps := []string{}

	name := strings.Join(moduleName, ":")

	for _, rule := range out.Rules {
		for _, req := range rule.Requires {
			deps = append(deps, req.LogicalName)
		}

		provides := false
		for _, pro := range rule.Provides {
			if pro.LogicalName == name {
				provides = true
				break
			}
		}

		if !provides {
			log.Fatalf("Expected %s to provide %s but it doesn't.", filePath, moduleName)
		}
	}

	return deps
}

type ModuleFile struct {
	filePath     string
	name         []string
	dependencies []string
}

type ParseModuleFile struct {
	cfg         Config
	baseName    string
	fileName    string
	moduleName  []string
	semaphore   chan struct{}
	modFileSend chan ModuleFile
	wg          sync.WaitGroup
}

func parseModuleFile(params ParseModuleFile) {
	filePath := path.Join(params.cfg.moduleDirPath, params.baseName, params.fileName)
	dependencies := parseModuleDependencies(filePath, params.moduleName)
	params.modFileSend <- ModuleFile{
		filePath:     filePath,
		name:         params.moduleName,
		dependencies: dependencies,
	}
	_ <- params.semaphore
	params.wg.Done()
}

const (
	CCKindLib CCKind = iota
	CCKindBin
	CCKindModTest
	CCKindLibTest
)

type CCFile struct {
	filePath     string
	kind         CCKind
	baseName     string
	ccName       string
	dependencies []string
}

type ParseCCFile struct {
	cfg        Config
	kind       CCKind
	ccName     string
	baseName   string
	fileName   string
	semaphore  chan struct{}
	ccFileSend chan CCFile
	wg         sync.WaitGroup
}

func parseCCFile(ParseCCFile params) {
	filePath := ""
	if params.kind == CCKindLib {
		filePath = path.Join(params.cfg.libDirPath, params.baseName, "src", params.fileName)
	} else if params.kind == CCKindLibTest {
		filePath = path.Join(params.cfg.libDirPath, params.baseName, "test", params.fileName)
	} else if params.kind == CCKindBin {
		filePath = path.Join(params.cfg.binDirPath, params.baseName, "src", params.fileName)
	} else if params.kind == CCKindModTest {
		filePath = path.Join(params.cfg.moduleDirPath, params.baseName, "test", params.fileName)
	} else {
		log.Fatalf("Unimplemented CCKind while parsing CC File: %s %s", params.baseName, params.fileName)
	}
	dependencies := parseCCDependencies(params.cfg, params.fileName)
	params.ccFileSend <- CCFile{
		filePath:     filePath,
		kind:         params.kind,
		baseName:     params.baseName,
		ccName:       params.ccName,
		dependencies: dependencies,
	}
	_ <- params.semaphore
	params.wg.Done()
}

type Module struct {
	parts            [][]string
	tests            []string
	dependencies     []string
	testDependencies []string
}

type Library struct {
	parts            []string
	tests            []string
	dependencies     []string
	testDependencies []string
}

type Binary struct {
	parts        []string
	dependencies []string
}

type ParseModFolder struct {
	cfg             Config
	baseName        string
	folderSemaphore chan struct{}
	fileSemaphore   chan struct{}
	modFileSend     chan ModuleFile
	ccFileSend      chan CCFile
	modSend         chan Module
	wg              sync.WaitGroup
}

func parseModFolder(params ParseModFolder) {
	srcFolderPath := path.Join(params.cfg.moduleDirPath, params.baseName, "src")
	testFolderPath := path.Join(params.cfg.moduleDirPath, params.baseName, "test")

	var subWg sync.WaitGroup

	srcEntries, err := os.ReadDir(srcFolderPath)
	if err != nil {
		log.Fatalf("Failed to list entries in mod directory: %s: %v", srcFolderPath, err)
	}

	parts := [][]string{}

	subWg.Add(len(srcEntries))
	for _, srcEntry := range srcEntries {
		moduleName := parseModuleName(srcEntry.Name())
		parts = append(parts, moduleName)
		fileSemaphore <- struct{}{}
		go parseModuleFile(ParseModuleFile{
			cfg:         params.cfg,
			moduleName:  moduleName,
			baseName:    params.baseName,
			fileName:    srcEntry.Name(),
			semaphore:   params.fileSemaphore,
			modFileSend: params.modFileSend,
			wg:          subWg,
		})
	}

	tests := []string{}

	testEntries, err := os.ReadDir(testFolderPath)
	if err == nil {
		subWg.add(len(testEntries))
		for _, testEntry := range testEntries {
			testName := parseCCName(testEntry.Name())
			tests = append(tests, testName)
			fileSemaphore <- struct{}{}
			go parseCCFile(ParseCCFile{
				cfg:        params.cfg,
				kind:       CCKindModTest,
				ccName:     testName,
				baseName:   params.baseName,
				fileName:   testEntry.Name(),
				semaphore:  params.fileSemaphore,
				ccFileSend: params.ccFileSend,
				wg:         subWg,
			})
		}
	}

	depFile := parseDepsFile(path.Join(params.cfg.moduleDirPath, params.baseName, "deps.json"))

	params.modSend <- Module{
		parts:            parts,
		tests:            tests,
		dependencies:     depFile.Dependencies,
		testDependencies: depFile.TestDependencies,
	}

	subWg.Wait()
	_ <- params.folderSemaphore
	params.wg.Done()
}

type ParseModFolders struct {
	cfg             Config
	fileSemaphore   chan struct{}
	folderSemaphore chan struct{}
	modFileSend     chan ModuleFile
	ccFileSend      chan CCFile
	modSend         chan Module
	wg              sync.WaitGroup
}

func parseModFolders(ParseModFolders params) {
	entries, err := os.ReadDir(params.cfg.moduleDirPath)
	if err != nil {
		log.Fatalf("Failed to list entries in modules directory.")
	}

	params.wg.Add(len(entries))
	for _, entry := range entries {
		params.folderSemaphore <- struct{}{}
		go parseModFolder(ParseModFolder{
			cfg:             params.cfg,
			fileSemaphore:   params.fileSemaphore,
			folderSemaphore: params.folderSemaphore,
			modFileSend:     params.modFileSend,
			ccFileSend:      params.ccFileSend,
			wg:              params.wg,
			modSend:         params.modSend,
		})
	}

	params.wg.Done()
}

type ParseLibFolder struct {
	cfg             Config
	baseName        string
	folderSemaphore chan struct{}
	fileSemaphore   chan struct{}
	ccFileSend      chan CCFile
	libSend         chan Library
	wg              sync.WaitGroup
}

func parseLibFolder(ParseLibFolder params) {
	srcFolderPath := path.Join(params.cfg.libDirPath, params.baseName, "src")
	testFolderPath := path.Join(params.cfg.libDirPath, params.baseName, "test")

	var subWg sync.WaitGroup

	srcEntries, err := os.ReadDir(srcFolderPath)
	if err != nil {
		log.Fatalf("Failed to list entries in lib directory: %s: %v", srcFolderPath, err)
	}

	parts := []string{}

	subWg.add(len(srcEntries))
	for _, srcEntry := range srcEntries {
		partName := parseCCName(srcEntry.Name())
		parts = append(parts, partName)
		fileSemaphore <- struct{}{}
		go parseCCFile(ParseCCFile{
			cfg:        params.cfg,
			kind:       CCKindLib,
			ccName:     partName,
			baseName:   params.baseName,
			fileName:   srcEntry.Name(),
			semaphore:  params.fileSemaphore,
			ccFileSend: params.ccFileSend,
			wg:         subWg,
		})
	}

	tests := []string{}

	testEntries, err := os.ReadDir(testFolderPath)
	if err == nil {
		subWg.add(len(testEntries))
		for _, testEntry := range testEntries {
			testName := parseCCName(testEntry.Name())
			tests = append(tests, testName)
			fileSemaphore <- struct{}{}
			go parseCCFile(ParseCCFile{
				cfg:        params.cfg,
				kind:       CCKindLibTest,
				ccName:     testName,
				baseName:   params.baseName,
				fileName:   testEntry.Name(),
				semaphore:  params.fileSemaphore,
				ccFileSend: params.ccFileSend,
				wg:         subWg,
			})
		}
	}

	depsFile := parseDepsFile(path.Join(params.cfg.libDirPath, baseName, "deps.json"))

	params.libSend <- Library{
		parts:            parts,
		tests:            tests,
		dependencies:     depsFile.Dependencies,
		testDependencies: depsFile.TestDependencies,
	}

	subWg.Wait()
	_ <- params.folderSemaphore
	params.wg.Done()
}

type ParseLibFolders struct {
	cfg             Config
	folderSemaphore chan struct{}
	fileSemaphore   chan struct{}
	ccFileSend      chan CCFile
	wg              sync.WaitGroup
	libSend         chan Library
}

func parseLibFolders(params ParseLibFolders) {
	entries, err := os.ReadDir(params.cfg.libDirPath)
	if err != nil {
		log.Fatalf("Failed to list libraries directory: %s, %v", params.cfg.libDirPath, err)
	}

	params.wg.Add(len(entries))
	for _, entry := range entries {
		params.folderSemaphore <- struct{}{}
		go parseLibFolder(ParseLibFolder{
			cfg:             params.cfg,
			baseName:        entry.Name(),
			folderSemaphore: params.folderSemaphore,
			fileSemaphore:   params.fileSemaphore,
			ccFileSend:      params.ccFileSend,
			wg:              params.wg,
			libSend:         params.libSend,
		})
	}

	params.wg.Done()
}

type ParseBinFolder struct {
	cfg             Config
	baseName        string
	folderSemaphore chan struct{}
	fileSemaphore   chan struct{}
	ccFileSend      chan CCFile
	wg              sync.WaitGroup
	binSend         chan Binary
}

func parseBinFolder(params ParseBinFolder) {
	srcFolderPath := path.Join(params.cfg.binDirPath, params.baseName, "src")

	var subWg sync.WaitGroup

	srcEntries, err := os.ReadDir(srcFolderPath)
	if err != nil {
		log.Fatalf("Failed to list entries in bin directory: %s: %v", srcFolderPath, err)
	}

	parts := []string{}

	subWg.add(len(srcEntries))
	for _, srcEntry := range srcEntries {
		ccName := parseCCName(srcEntry.Name())
		parts = append(parts, ccName)
		fileSemaphore <- struct{}{}
		go parseCCFile(ParseCCFile{
			cfg:        params.cfg,
			kind:       CCKindBin,
			baseName:   params.baseName,
			fileName:   srcEntry.Name(),
			semaphore:  params.fileSemaphore,
			ccFileSend: params.ccFileSend,
			wg:         subWg,
		})
	}

	depsFile := parseDepsFile(path.Join(params.cfg.binDirPath, params.baseName, "deps.json"))
	if len(depsFile.testDependencies) > 0 {
		log.Fatalf("The binary %s can't have test dependencies in it's deps.json file", params.baseName)
	}

	params.binSend <- Binary{
		parts:        parts,
		dependencies: depsFile.Dependencies,
	}

	subWg.Wait()
	_ <- params.folderSemaphore
	params.wg.Done()
}

type ParseBinFolders struct {
	cfg             Config
	folderSemaphore chan struct{}
	fileSemaphore   chan struct{}
	ccFileSend      chan CCFile
	wg              sync.WaitGroup
	binSend         chan Binary
}

func parseBinFolders(params ParseBinFolders) {
	entries, err := os.ReadDir(params.cfg.binDirPath)
	if err != nil {
		log.Fatalf("Failed to list binaries directory: %s, %v", params.cfg.binDirPath, err)
	}

	params.wg.Add(len(entries))
	for _, entry := range entries {
		params.folderSemaphore <- struct{}{}
		go parseBinFolder(ParseBinFolder{
			cfg:             params.cfg,
			baseName:        entry.Name(),
			folderSemaphore: params.folderSemaphore,
			fileSemaphore:   params.fileSemaphore,
			ccFileSend:      params.ccFileSend,
			wg:              params.wg,
			binSend:         params.binSend,
		})
	}

	params.wg.Done()
}

type ParseFolders struct {
	cfg         Config
	modFileSend chan ModuleFile
	ccFileSend  chan CCFile
	finished    chan struct{}
	modSend     chan Module
	libSend     chan Library
	binSend     chan Binary
}

func parseFolders(params ParseFolders) {
	concurrency := runtime.NumCPU()
	folderSemaphore := make(chan struct{}, concurrency*2)
	fileSemaphore := make(chan struct{}, concurrency*2)

	var wg sync.WaitGroup

	wg.Add(3)
	go parseModFolders(ParseModFolders{
		cfg:             params.cfg,
		fileSemaphore:   fileSemaphore,
		folderSemaphore: folderSemaphore,
		modFileSend:     params.modFileSend,
		ccFileSend:      params.ccFileSend,
		modSend:         params.modSend,
		wg:              wg,
	})
	go parseLibFolders(ParseLibFolders{
		cfg:             params.cfg,
		fileSemaphore:   fileSemaphore,
		folderSemaphore: folderSemaphore,
		ccFileSend:      params.ccFileSend,
		libSend:         params.libSend,
		wg:              wg,
	})
	go parseBinFolders(ParseBinFolders{
		cfg:             params.cfg,
		fileSemaphore:   fileSemaphore,
		folderSemaphore: folderSemaphore,
		ccFileSend:      params.ccFileSend,
		binSend:         params.bindSend,
		wg:              wg,
	})

	wg.Wait()
	finished <- struct{}{}
}

func goCompileModule(cfg Config, modFile ModuleFile, compiledSend chan string) {
	args := []string{}

	args = append(args, cfg.cflags...)

	outFileName := strings.Join(strings.Split(modFile.name, ":"), "-") + ".pcm"
	outPath := path.Join(cfg.buildDirPath, outFileName)

	args = append(args, modFile.filePath)
	args = append(args, "--precompile")
	args = append(args, "-o")
	args = append(args, outPath)

	args = append(args, "-fprebuilt-module-path="+cfg.buildDirPath)

	cmd := exec.Command(cfg.cxx, args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to compile %v: %v: %s", modFile.filePath, err, output)
	}

	compiledSend <- modFile.name
}

func goCompileBin(cfg Config, name string, finishSignal chan struct{}) {}

func goCompileLibObj(cfg Config, name string, finishSignal chan struct{}) {}

func goCompile(cfg Config, src SourceFile, compiledChannel chan string) {
	args := []string{}

	args = append(args, cfg.cflags...)

	srcAbsPath := ""

	if src.kind == Module {
		outFileName := strings.Join(strings.Split(src.name, ":"), "-") + ".pcm"
		outAbsPath := path.Join(cfg.buildDirPath, outFileName)

		inFileName := strings.Join(strings.Split(src.name, ":"), "_") + ".cppm"
		srcAbsPath = path.Join(cfg.srcDirPath, inFileName)

		args = append(args, srcAbsPath)
		args = append(args, "--precompile")
		args = append(args, "-o")
		args = append(args, outAbsPath)
	} else {
		outFileName := src.name
		outAbsPath := path.Join(cfg.buildDirPath, outFileName)

		inFileName := src.name + ".cc"
		srcAbsPath = path.Join(cfg.srcDirPath, inFileName)

		args = append(args, path.Join(cfg.buildDirPath, "libc.o"))
		args = append(args, srcAbsPath)
		args = append(args, "-o")
		args = append(args, outAbsPath)
		args = append(args, cfg.ldflags...)
	}

	args = append(args, "-fprebuilt-module-path="+cfg.buildDirPath)

	cmd := exec.Command(cfg.cxx, args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to compile %v: %v: %s", srcAbsPath, err, output)
	}

	compiledChannel <- src.name
}

func goTest(cfg Config, name string, testedChannel chan string) {
	filePath := path.Join(cfg.buildDirPath, name)

	cmd := exec.Command(filePath)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to run test %v: %v: %s", filePath, err, output)
	}

	testedChannel <- name
}

type CompileStatus struct {
	src  SourceFile
	deps map[string]struct{}
}

func compileTestAll(cfg Config, srcChannel chan SourceFile) {
	compiledChannel := make(chan string, 100)
	testedChannel := make(chan string, 100)

	// status per artifact
	status := make(map[string]CompileStatus, 0)

	// maps the name of a module to the names of things that depend on it
	reverseDependencyMap := make(map[string][]string, 0)

	// ready to compile but waiting
	toCompile := make([]string, 0)

	// ready to test but waiting
	toTest := make([]string, 0)

	// concurrency limit for compilations and tests.
	concurrency := runtime.NumCPU()

	nCompiling := 0
	nTesting := 0

	nToCompile := 0
	nToTest := 0

	srcChannelDone := false

	for {
		select {
		case src := <-srcChannel:
			if src.name == "" {
				srcChannelDone = true
				// check that all dependencies are actually found
				for _, entry := range status {
					for _, dep := range entry.src.deps {
						_, has := status[dep]
						if !has {
							log.Fatalf("The dependency '%s' for '%s' wasn't found.", dep, entry.src.name)
						}
					}
				}
				continue
			}

			_, alreadyExists := status[src.name]
			if alreadyExists {
				log.Fatalf("Duplicate name: %v", src.name)
			}

			nToCompile += 1

			if src.kind == Test {
				nToTest += 1
			}

			deps := make(map[string]struct{})
			for _, dep := range src.deps {
				deps[dep] = struct{}{}
			}

			status[src.name] = CompileStatus{
				src:  src,
				deps: deps,
			}

			if len(src.deps) == 0 {
				toCompile = append(toCompile, src.name)
			} else {
				// insert dependencies to reversed index
				for _, dependency := range src.deps {
					entry, exists := reverseDependencyMap[dependency]
					if exists {
						reverseDependencyMap[dependency] = append(entry, src.name)
					} else {
						reverseDependencyMap[dependency] = []string{src.name}
					}
				}
			}
		case name := <-compiledChannel:
			nToCompile -= 1
			// remove reverse dependencies since this module is now ready
			entry, exists := reverseDependencyMap[name]
			if exists {
				for _, dependant := range entry {
					delete(status[dependant].deps, name)

					// add to compile queue if ready to compile
					if len(status[dependant].deps) == 0 {
						toCompile = append(toCompile, dependant)
					}
				}
				delete(reverseDependencyMap, name)
			}

			// add to test queue if the compiled item is a test
			if status[name].src.kind == Test {
				toTest = append(toTest, name)
			}

			nCompiling -= 1
		case name := <-testedChannel:
			nToTest -= 1
			_ = name
			nTesting -= 1
		}

		// start new compilations if there is room
		for nCompiling < concurrency && len(toCompile) > 0 {
			go goCompile(cfg, status[toCompile[len(toCompile)-1]].src, compiledChannel)
			toCompile = toCompile[:len(toCompile)-1]
			nCompiling += 1
		}

		// start new tests if there is room
		for nTesting < concurrency && len(toTest) > 0 {
			go goTest(cfg, toTest[len(toTest)-1], testedChannel)
			toTest = toTest[:len(toTest)-1]
			nTesting += 1
		}

		if srcChannelDone && nToCompile == 0 && nToTest == 0 {
			break
		}
	}
}

func main() {
	// precompileModules (modules) - compile object files (tests, bin, lib)
	// link executables (tests, bin)

	cfg := readConfig()

	err := os.RemoveAll(cfg.buildDirPath)
	if err != nil {
		log.Fatalf("Failed to clean the build directory: %v", err)
	}

	err = os.MkdirAll(cfg.buildDirPath, os.ModePerm)
	if err != nil {
		log.Fatalf("Failed to create build directory: %v", err)
	}

	compileLibC(cfg)

	srcChannel := make(chan SourceFile, 100)
	go goProcessSrcDir(cfg, srcChannel)

	compileTestAll(cfg, srcChannel)
}
