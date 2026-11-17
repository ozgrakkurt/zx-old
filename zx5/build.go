package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"sync"
)

type FlagConfig struct {
	optLevel       string
	devMode        bool
	debugInfo      bool
	fuzz           bool
	includeDirPath string
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
	ar            string
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
	if filepath.IsAbs(pathval) {
		return pathval
	} else {
		return filepath.Join(defaultBase, pathval)
	}
}

func readConfig() Config {
	var cxx = flag.String("cxx", "clang++", "c++ compiler")
	var ar = flag.String("ar", "llvm-ar", "ar binary for archiving static libraries")
	var clangScanDeps = flag.String("clang-scan-deps", "clang-scan-deps", "clang-scan-deps executable")
	var optLevel = flag.String("opt-level", "0", "optimization ('-O') flag value for compiling c++ code")
	var devMode = flag.Bool("dev-mode", true, "enable development mode or not")
	var debugInfo = flag.Bool("debug-info", true, "include debug info in the output executables")
	var fuzz = flag.Bool("fuzz", false, "enable fuzz testing instrumentation")
	var projDir = flag.String("proj-dir", ".", "project directory")
	var buildDir = flag.String("build-dir", "build", "directory for outputting build artifacts")

	flag.Parse()

	currentPath, err := os.Getwd()
	if err != nil {
		log.Fatalf("Failed to get current working directory: %v", err)
	}

	projDirPath := makePathAbsolute(currentPath, *projDir)
	buildDirPath := makePathAbsolute(currentPath, *buildDir)

	moduleDirPath := filepath.Join(projDirPath, "module")
	libDirPath := filepath.Join(projDirPath, "lib")
	binDirPath := filepath.Join(projDirPath, "bin")
	includeDirPath := filepath.Join(projDirPath, "include")

	flagCfg := FlagConfig{
		optLevel:       *optLevel,
		devMode:        *devMode,
		debugInfo:      *debugInfo,
		fuzz:           *fuzz,
		includeDirPath: includeDirPath,
	}

	cflags := makeCflags(flagCfg)
	ldflags := makeLdflags(flagCfg)

	cfg := Config{
		cxx:           *cxx,
		ar:            *ar,
		clangScanDeps: *clangScanDeps,
		cflags:        cflags,
		ldflags:       ldflags,
		buildDirPath:  buildDirPath,
		moduleDirPath: moduleDirPath,
		libDirPath:    libDirPath,
		binDirPath:    binDirPath,
	}

	return cfg
}

type State struct {
	cfg Config
	// absolute path -> entity index mapping
	pathMap map[string]int
	// entity state
	entities []Entity
	deps [][]int
	revDeps [][]int
	isReady []bool
	// aboslute path of entity at index
	path []string
	// receives entity index when an entity has finished building
	notify chan int
	// receives an entity when the entity is discovered
	entityRecv chan Entity
}

type Entity interface {
	// Registers self to state including it's dependencies
	// After registering itself, should check for cyclic dependency
	// chains by doing a dependency traversal starting from itself.
	registerSelf(state *State)
	// Triggers build of self, should notify the state when the build is one through the
	// state.notify channel.
	triggerBuild(state *State)
}

type ModuleFile struct {
	path string
	moduleName   string
	dependencies []string
}

func (mf ModuleFile) registerSelf(state *State) {
	entityIdx, found := state.pathMap[mf.path]

	if !foun
}

type CCFile struct {
	inputPath    string
	outputPath   string
	dependencies []string
}

type Module struct {
	moduleName       string
	parts            []string
	testInputPaths   []string
	testOutputPaths  []string
	dependencies     []string
	testDependencies []string
}

type Library struct {
	libName          string
	srcInputPaths    []string
	srcOutputPaths   []string
	testInputPaths   []string
	testOutputPaths  []string
	dependencies     []string
	testDependencies []string
}

type Binary struct {
	binName        string
	srcInputPaths  []string
	srcOutputPaths []string
	dependencies   []string
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

func parseModuleName(baseName string, fileName string) string {
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

	if ext != "ccm" {
		log.Fatalf("File on module folder has an unexpected extension: %v. Expected it to have '.ccm'", fileName)
	}

	if name == "" {
		log.Fatalf("Empty name isn't allowed for modules.")
	}

	if baseName == name {
		return name
	} else {
		return baseName + ":" + name
	}
}

type DepsFile struct {
	Dependencies     []string `json:"dependencies"`
	TestDependencies []string `json:"test_dependencies"`
}

func parseDepsFile(filePath string, send chan DepsFile) {
	content, err := os.ReadFile(filePath)
	if err != nil {
		log.Fatalf("Failed to read deps file: %s, %v", filePath, err)
	}
	out := DepsFile{}
	err = json.Unmarshal(content, &out)
	if err != nil {
		log.Fatalf("Failed to parse dependency file: %s: %v", filePath, err)
	}
	send <- out
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

func parseModuleDependencies(cfg Config, filePath string, moduleName string) []string {
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

		provides := false
		for _, pro := range rule.Provides {
			if pro.LogicalName == moduleName {
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

type ParseModuleFile struct {
	cfg         Config
	baseName    string
	fileName    string
	moduleName  string
	semaphore   chan struct{}
	modFileSend chan ModuleFile
	wg          sync.WaitGroup
}

func parseModuleFile(params ParseModuleFile) {
	filePath := filepath.Join(params.cfg.moduleDirPath, params.baseName, params.fileName)
	dependencies := parseModuleDependencies(params.cfg, filePath, params.moduleName)
	_ <- params.semaphore
	params.modFileSend <- ModuleFile{
		moduleName:   params.moduleName,
		dependencies: dependencies,
	}
	params.wg.Done()
}

type ParseCCFile struct {
	cfg        Config
	inputPath  string
	outputPath string
	semaphore  chan struct{}
	ccFileSend chan CCFile
	wg         sync.WaitGroup
}

func parseCCFile(ParseCCFile params) {
	dependencies := parseCCDependencies(params.cfg, params.inputPath)
	_ <- params.semaphore
	params.ccFileSend <- CCFile{
		inputPath:    params.inputPath,
		outputPath:   params.outputPath,
		dependencies: dependencies,
	}
	params.wg.Done()
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
	basePath := filepathJoin(params.cfg.moduleDirPath, params.baseName)

	srcFolderPath := filepath.Join(basePath, "src")
	testFolderPath := filepath.Join(basePath, "test")

	depsChan = make(chan DepsFile, 1)
	go parseDepsFile(filepath.Join(basePath, "deps.json"), depsChan)

	var subWg sync.WaitGroup

	srcEntries, err := os.ReadDir(srcFolderPath)
	if err != nil {
		log.Fatalf("Failed to list entries in mod directory: %s: %v", srcFolderPath, err)
	}

	parts := []string{}

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

	testInputPaths := []string{}
	testOutputPaths := []string{}

	testEntries, err := os.ReadDir(testFolderPath)
	if err == nil {
		subWg.Add(len(testEntries))
		for _, testEntry := range testEntries {
			inputPath := filepath.Join(testFolderPath, testEntry.Name())
			testName := parseCCName(testEntry.Name())
			outputPath := filepath.Join(params.cfg.buildDirPath, "mod", params.baseName, "test", testName+".o")
			testInputPaths = append(testInputPaths, inputPath)
			testOutputPaths = append(testOutputPath, outputPath)
			fileSemaphore <- struct{}{}
			go parseCCFile(ParseCCFile{
				cfg:        params.cfg,
				inputPath:  inputPath,
				outputPath: outputPath,
				semaphore:  params.fileSemaphore,
				ccFileSend: params.ccFileSend,
				wg:         subWg,
			})
		}
	}

	depFile := <-depsChan
	params.modSend <- Module{
		moduleName:       params.baseName,
		parts:            parts,
		testInputPaths:   testInputPaths,
		testOutputPaths:  testOutputPaths,
		dependencies:     depFile.Dependencies,
		testDependencies: depFile.TestDependencies,
	}

	_ <- params.folderSemaphore
	subWg.Wait()
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

	var subWg sync.WaitGroup

	subWg.Add(len(entries))
	for _, entry := range entries {
		params.folderSemaphore <- struct{}{}
		go parseModFolder(ParseModFolder{
			cfg:             params.cfg,
			fileSemaphore:   params.fileSemaphore,
			folderSemaphore: params.folderSemaphore,
			modFileSend:     params.modFileSend,
			ccFileSend:      params.ccFileSend,
			wg:              subWg,
			modSend:         params.modSend,
		})
	}

	subWg.Wait()
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
	basePath := filepath.Join(params.cfg.libDirPath, params.baseName)
	srcFolderPath := filepath.Join(basePath, "src")
	testFolderPath := filepath.Join(basePath, "test")

	depsChan = make(chan DepsFile, 1)
	go parseDepsFile(filepath.Join(basePath, "deps.json"), depsChan)

	var subWg sync.WaitGroup

	srcEntries, err := os.ReadDir(srcFolderPath)
	if err != nil {
		log.Fatalf("Failed to list entries in lib directory: %s: %v", srcFolderPath, err)
	}

	srcInputPaths := []string{}
	srcOutputPaths := []string{}

	subWg.Add(len(srcEntries))
	for _, srcEntry := range srcEntries {
		inputPath := filepath.Join(srcFolderPath, srcEntry.Name())
		partName := parseCCName(srcEntry.Name())
		outputPath := filepath.Join(params.cfg.buildDirPath, "lib", params.baseName, "src", partName+".o")

		srcInputPaths := append(srcInputPaths, inputPath)
		srcOutputPaths := append(srcOutputPaths, outputPath)

		fileSemaphore <- struct{}{}
		go parseCCFile(ParseCCFile{
			cfg:        params.cfg,
			inputPath:  inputPath,
			outputPath: outputPath,
			semaphore:  params.fileSemaphore,
			ccFileSend: params.ccFileSend,
			wg:         subWg,
		})
	}

	testInputPaths := []string{}
	testOutputPaths := []string{}

	testEntries, err := os.ReadDir(testFolderPath)
	if err == nil {
		subWg.Add(len(testEntries))
		for _, testEntry := range testEntries {
			inputPath := filepath.Join(testFolderPath, testEntry.Name())
			testName := parseCCName(testEntry.Name())
			outputPath := filepath.Join(params.cfg.buildDirPath, "lib", params.baseName, "test", testName+".o")

			testInputPaths := append(testInputPaths, inputPath)
			testOutputPaths := append(testOutputPaths, outputPath)

			fileSemaphore <- struct{}{}
			go parseCCFile(ParseCCFile{
				cfg:        params.cfg,
				inputPath:  inputPath,
				outputPath: outputPath,
				semaphore:  params.fileSemaphore,
				ccFileSend: params.ccFileSend,
				wg:         subWg,
			})
		}
	}

	depsFile := <-depsChan
	params.libSend <- Library{
		libName:          params.baseName,
		srcInputPaths:    srcInputPaths,
		srcOutputPaths:   srcOutputPaths,
		testInputPaths:   testInputPaths,
		testOutputPaths:  testOutputPaths,
		dependencies:     depsFile.Dependencies,
		testDependencies: depsFile.TestDependencies,
	}

	_ <- params.folderSemaphore
	subWg.Wait()
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

	var subWg sync.WaitGroup

	subWg.Add(len(entries))
	for _, entry := range entries {
		params.folderSemaphore <- struct{}{}
		go parseLibFolder(ParseLibFolder{
			cfg:             params.cfg,
			baseName:        entry.Name(),
			folderSemaphore: params.folderSemaphore,
			fileSemaphore:   params.fileSemaphore,
			ccFileSend:      params.ccFileSend,
			wg:              subWg,
			libSend:         params.libSend,
		})
	}

	subWg.Wait()
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
	basePath := filepath.Join(params.cfg.binDirPath, params.baseName)
	srcFolderPath := filepath.Join(basePath, "src")

	depsChan = make(chan DepsFile, 1)
	go parseDepsFile(filepath.Join(basePath, "deps.json"), depsChan)

	var subWg sync.WaitGroup

	srcEntries, err := os.ReadDir(srcFolderPath)
	if err != nil {
		log.Fatalf("Failed to list entries in bin directory: %s: %v", srcFolderPath, err)
	}

	srcInputPaths := []string{}
	srcOutputPaths := []string{}

	subWg.Add(len(srcEntries))
	for _, srcEntry := range srcEntries {
		inputPath := filepath.Join(srcFolderPath, srcEntry.Name())
		ccName := parseCCName(srcEntry.Name())
		outputPath := filepath.Join(params.buildDirPath, "bin", params.baseName, "src", ccName+".o")
		fileSemaphore <- struct{}{}
		go parseCCFile(ParseCCFile{
			cfg:        params.cfg,
			inputPath:  inputPath,
			outputPath: outputPath,
			semaphore:  params.fileSemaphore,
			ccFileSend: params.ccFileSend,
			wg:         subWg,
		})
	}

	depsFile := <-depsChan
	if len(depsFile.testDependencies) > 0 {
		log.Fatalf("The binary %s can't have test dependencies in it's deps.json file", params.baseName)
	}

	params.binSend <- Binary{
		binName:        params.baseName,
		srcInputPaths:  srcInputPaths,
		srcOutputPaths: srcOutputPaths,
		dependencies:   depsFile.Dependencies,
	}

	_ <- params.folderSemaphore
	subWg.Wait()
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

	var subWg sync.WaitGroup

	subWg.Add(len(entries))
	for _, entry := range entries {
		params.folderSemaphore <- struct{}{}
		go parseBinFolder(ParseBinFolder{
			cfg:             params.cfg,
			baseName:        entry.Name(),
			folderSemaphore: params.folderSemaphore,
			fileSemaphore:   params.fileSemaphore,
			ccFileSend:      params.ccFileSend,
			wg:              subWg,
			binSend:         params.binSend,
		})
	}

	subWg.Wait()
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
	folderSemaphore := make(chan struct{}, concurrency)
	fileSemaphore := make(chan struct{}, concurrency)

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
		binSend:         params.binSend,
		wg:              wg,
	})

	wg.Wait()
	finished <- struct{}{}
}

func archiveLib(cfg Config, lib Library) {
	archivePath := filepath.Join(cfg.buildDirPath, "lib", lib.name, lib.name+".a")

	err := os.Remove(archivePath)
	_ = err

	args := []string{
		"crs",
		archivePath,
	}

	for _, obj_name := range lib.parts {
		args = append(args, filepath.Join(cfg.buildDirPath, "lib", lib.name, obj_name+".o"))
	}

	cmd := exec.Command(cfg.ar, args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to archive library %v: %v\n %s", archivePath, err, output)
	}
}

func linkBinary(cfg Config, bin Binary, dependencies map[string]struct{}) {
	binPath := filepath.Join(cfg.buildDirPath, "bin", bin.binName, bin.binName)

	args := []string{}

	args = append(args, cfg.ldflags...)

	args = append(args, "-o")
	args = append(args, binPath)

	for _, objPath := range bin.srcOutputPaths {
		args = append(args, objPath)
	}

	for lib_name, _ := range dependencies {
		args = append(args, filepath.Join(cfg.buildDirPath, "lib", lib_name, lib_name+".a"))
	}

	cmd := exec.Command(cfg.cxx, args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to link binary %v: %v\n %s", binPath, err, output)
	}
}

func linkAndRunTest(cfg Config, objPath string, binPath string, dependencies map[string]struct{}) {
	args := []string{}

	args = append(args, cfg.ldflags...)

	args = append(args, "-o")
	args = append(args, outputPath)

	args = append(args, objPath)

	for lib_name, _ := range dependencies {
		args = append(args, filepath.Join(cfg.buildDirPath, "lib", lib_name, lib_name+".a"))
	}

	cmd := exec.Command(cfg.cxx, args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to link test %v: %v\n %s", outPath, err, output)
	}

	// run the test
	cmd := exec.Command(outputPath)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to run test %v: %v\n %s", outPath, err, output)
	}
}

func compileModuleFile(cfg Config, modFile ModuleFile) {
	args := []string{}

	args = append(args, cfg.cflags...)

	nameParts := strings.Split(modFile.moduleName, ":")

	outFileName := strings.Join(nameParts, "-") + ".pcm"
	outPath := filepath.Join(cfg.buildDirPath, "module", outFileName)

	filePath := filepath.Join(cfg.moduleDirPath, nameParts[0], "src", nameParts[len(nameParts)-1]+".ccm")

	args = append(args, filePath)
	args = append(args, "--precompile")
	args = append(args, "-o")
	args = append(args, outPath)

	args = append(args, "-fprebuilt-module-path="+filepath.Join(cfg.buildDirPath, "module"))

	cmd := exec.Command(cfg.cxx, args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to compile %v: %v\n %s", filePath, err, output)
	}
}

func compileCCFile(cfg Config, ccFile CCFile) {
	args := []string{}

	args = append(args, cfg.cflags...)

	args = append(args, "-c")
	args = append(args, ccFile.inputPath)
	args = append(args, "-o")
	args = append(args, ccFile.outputPath)

	args = append(args, "-fprebuilt-module-path="+filepath.Join(cfg.buildDirPath, "module"))

	cmd := exec.Command(cfg.cxx, args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to compile %v: %v\n %s", ccFile.inputPath, err, output)
	}
}

func build(cfg Config) {
	err := os.RemoveAll(cfg.buildDirPath)
	if err != nil {
		log.Fatalf("Failed to clean the build directory: %v", err)
	}

	err = os.MkdirAll(cfg.buildDirPath, os.ModePerm)
	if err != nil {
		log.Fatalf("Failed to create build directory: %v", err)
	}

	modFileChan := make(chan ModuleFile)
	ccFileChan := make(chan CCFile)
	parseFinished := make(chan struct{}, 1)
	modChan := make(chan Module)
	libChan := make(chan Library)
	binChan := make(chan Binary)

	go parseFolders(ParseFolders{
		cfg:         cfg,
		modFileSend: modFileChan,
		ccFileSend:  ccFileChan,
		finished:    parseFinished,
		modSend:     modChan,
		libSend:     libChan,
		binSend:     binChan,
	})

	notify := make(chan int)

	moduleFiles := []ModuleFile
	ccFiles := []CCFile

	modules := []Module
	libs := []Library
	binaries := []Binary

	entities := []Entity

	// map name of an entity to the index of it
	modFileEntity := make(map[string]int)
	ccFileEntity := make(map[string]int)
	moduleEntity := make(map[string]int)
	libEntity := make(map[string]int)
	binEntity := make(map[string]int)

	// dependencies of entity at index
	deps := [][]int
	// dependants of entity at index
	revDeps := [][]int
	// entity at index is already compiled/linked/ready
	isReady := []bool

	parsingIsFinished := false
	nPending := 0

	concurrency := runtime.NumCPU() * 2
	semaphore := make(chan struct{}, concurrency)

	registerModFile := func(modFile ModuleFile) int {
		
	}
	registerModuleName := func(moduleName string) int {
		
	}

	registerCCFile := func(ccFile CCFile) int {
		
	}
	registerCCFilePath := func(inputPath string) int {
		
	}

	registerModule := func(module Module) int {
		
	}

	registerLib := func(lib Library) int {
		
	}
	registerLibName := func(libName string) int {
		
	}

	registerBin := func(bin Binary) int {
		entityIdx, found := binEntity[bin.binName]
		if !found {
			binIdx := len(binaries)
			binaries = append(binaries, bin)

			entityIdx = len(entities)

			binEntity[bin.binName] = entityIdx

			entities = append(entities, Entity { kind: EntityBin, idx: binIdx })

			isReady = append(isReady, false)
			revDeps = append(revDeps, []int{})

			deps := []int{}

			for 

			deps = append(deps, []int{})
		}

		return entityIdx
	}

	for {
		select {
		case modFile := <-modFileChan:
			if parsingIsFinished {
				panic("impossible")
			}

			entityIdx, found := modFileEntity[modFile.moduleName]
			if !found {
				modFileIdx := len(moduleFiles)
				moduleFiles = append(moduleFiles, modFile)

				entityIdx = len(entities)
				entities = append(entities, Entity{kind: EntityModFile, idx: modFileIdx})
			} else {
				modFiles[entities[entityIdx].idx] = modFile
			}
		case ccFile := <-ccFileChan:
			if parsingIsFinished {
				panic("impossible")
			}

			depsSet := make(map[string]struct{})
			for _, dep := range ccFile.dependencies {
				_, isReady := modFileReady[dep]
				if !isReady {
					depsSet[dep] = struct{}{}
				}
			}
			ccFiles[ccFile.inputPath] = ccFile
			if len(depsSet) == 0 {
				semaphore <- struct{}{}
				go compileCCFile(cfg, ccFile, ccCompiled, semaphore)
				nPending += 1
			} else {
				ccDeps[ccFile.inputPath] = depsSet
				for dep, _ := range depsSet {
					revDeps, has := ccDepsRev[dep]
					if has {
						ccDepsRev[dep] = append(revDeps, ccFile.inputPath)
					} else {
						ccDepsRev[dep] = []string{ccFile.inputPath}
					}
				}
			}
		case mod := <-modChan:
			if parsingIsFinished {
				panic("impossible")
			}
			modules[mod.moduleName] = mod
		case lib := <-libChan:
			if parsingIsFinished {
				panic("impossible")
			}

		case bin := <-binChan:
			if parsingIsFinished {
				panic("impossible")
			}
			panic("todo")
		case _ := <-parseFinished:
			if parsingIsFinished {
				panic("impossible")
			}
			parsingIsFinished := true
		case libName := <-libArchived:
			panic("todo")
		case moduleName := <-modCompiled:
			panic("todo")
		case ccPath := <-ccCompiled:
			panic("todo")
		case _ := <-testFinished:
			panic("todo")
		case _ := <-binaryBuilt:
			panic("todo")
		}

		if parsingIsFinished && nPending == 0 {
			break
		}
	}
}

func main() {
	cfg := readConfig()
	build(cfg)
}
