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
	var buildDir = flag.String("build-dir", "build", "directory for outputting build artifacts")
	var moduleDir = flag.String("module-dir", "module", "directory for modules")
	var libDir = flag.String("lib-dir", "lib", "directory for libraries")
	var binDir = flag.String("bin-dir", "bin", "directory for binary targets")
	var includeDir = flag.String("include-dir", "include", "directory for header include path")

	flag.Parse()

	currentPath, err := os.Getwd()
	if err != nil {
		log.Fatalf("Failed to get current working directory: %v", err)
	}

	buildDirPath := makePathAbsolute(currentPath, *buildDir)
	moduleDirPath := makePathAbsolute(currentPath, *moduleDir)
	libDirPath := makePathAbsolute(currentPath, *libDir)
	binDirPath := makePathAbsolute(currentPath, *binDir)
	includeDirPath := makePathAbsolute(currentPath, *includeDir)

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

	if ext != "cppm" {
		log.Fatalf("File on module folder has an unexpected extension: %v. Expected it to have '.cppm'", fileName)
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

func parseDepsFile(filePath string) DepsFile {
	content, err := os.ReadFile(filePath)
	if err != nil {
		log.Fatalf("Failed to read deps file: %s, %v", filePath, err)
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

type ModuleFile struct {
	filePath     string
	moduleName   string
	dependencies []string
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
		filePath:     filePath,
		moduleName:   params.moduleName,
		dependencies: dependencies,
	}
	params.wg.Done()
}

type CCKind int

const (
	CCKindLib CCKind = iota
	CCKindBin
	CCKindModTest
	CCKindLibTest
)

type CCFile struct {
	filePath     string
	outPath      string
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
	outFileName := params.ccName + ".o"

	outPath := ""
	filePath := ""
	if params.kind == CCKindLib {
		filePath = filepath.Join(params.cfg.libDirPath, params.baseName, "src", params.fileName)
		outPath = filepath.Join(params.cfg.libDirPath, params.baseName, "obj", outFileName)
	} else if params.kind == CCKindLibTest {
		filePath = filepath.Join(params.cfg.libDirPath, params.baseName, "test", params.fileName)
		outPath = filepath.Join(params.cfg.libDirPath, params.baseName, "test", outFileName)
	} else if params.kind == CCKindBin {
		filePath = filepath.Join(params.cfg.binDirPath, params.baseName, "src", params.fileName)
		outPath = filepath.Join(params.cfg.binDirPath, params.baseName, "obj", outFileName)
	} else if params.kind == CCKindModTest {
		filePath = filepath.Join(params.cfg.moduleDirPath, params.baseName, "test", params.fileName)
		outPath = filepath.Join(params.cfg.moduleDirPath, params.baseName, "test", outFileName)
	} else {
		log.Fatalf("Unimplemented CCKind while parsing CC File: %s %s", params.baseName, params.fileName)
	}
	dependencies := parseCCDependencies(params.cfg, filePath)
	_ <- params.semaphore
	params.ccFileSend <- CCFile{
		filePath:     filePath,
		outPath:      outPath,
		kind:         params.kind,
		baseName:     params.baseName,
		ccName:       params.ccName,
		dependencies: dependencies,
	}
	params.wg.Done()
}

type Module struct {
	name             string
	parts            [][]string
	tests            []string
	dependencies     []string
	testDependencies []string
}

type Library struct {
	name             string
	parts            []string
	tests            []string
	dependencies     []string
	testDependencies []string
}

type Binary struct {
	name         string
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
	srcFolderPath := filepath.Join(params.cfg.moduleDirPath, params.baseName, "src")
	testFolderPath := filepath.Join(params.cfg.moduleDirPath, params.baseName, "test")

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
		subWg.Add(len(testEntries))
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

	depFile := parseDepsFile(filepath.Join(params.cfg.moduleDirPath, params.baseName, "deps.json"))

	params.modSend <- Module{
		name:             params.baseName,
		parts:            parts,
		tests:            tests,
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
	srcFolderPath := filepath.Join(params.cfg.libDirPath, params.baseName, "src")
	testFolderPath := filepath.Join(params.cfg.libDirPath, params.baseName, "test")

	var subWg sync.WaitGroup

	srcEntries, err := os.ReadDir(srcFolderPath)
	if err != nil {
		log.Fatalf("Failed to list entries in lib directory: %s: %v", srcFolderPath, err)
	}

	parts := []string{}

	subWg.Add(len(srcEntries))
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
		subWg.Add(len(testEntries))
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

	depsFile := parseDepsFile(filepath.Join(params.cfg.libDirPath, params.baseName, "deps.json"))

	params.libSend <- Library{
		name:             params.baseName,
		parts:            parts,
		tests:            tests,
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
	srcFolderPath := filepath.Join(params.cfg.binDirPath, params.baseName, "src")

	var subWg sync.WaitGroup

	srcEntries, err := os.ReadDir(srcFolderPath)
	if err != nil {
		log.Fatalf("Failed to list entries in bin directory: %s: %v", srcFolderPath, err)
	}

	parts := []string{}

	subWg.Add(len(srcEntries))
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

	depsFile := parseDepsFile(filepath.Join(params.cfg.binDirPath, params.baseName, "deps.json"))
	if len(depsFile.testDependencies) > 0 {
		log.Fatalf("The binary %s can't have test dependencies in it's deps.json file", params.baseName)
	}

	params.binSend <- Binary{
		name:         params.baseName,
		parts:        parts,
		dependencies: depsFile.Dependencies,
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
		binSend:         params.binSend,
		wg:              wg,
	})

	wg.Wait()
	finished <- struct{}{}
}

func archiveLib(cfg Config, lib Library, libArchived chan string, semaphore chan struct{}) {
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

	<-semaphore
	libArchived <- lib.name
}

func linkBinary(cfg Config, bin Binary, semaphore chan struct{}, wg sync.WaitGroup) {
	binPath := filepath.Join(cfg.buildDirPath, "bin", bin.name, bin.name)

	args := []string{}

	args = append(args, cfg.ldflags...)

	args = append(args, "-o")
	args = append(args, binPath)

	for _, obj_name := range bin.parts {
		args = append(args, filepath.Join(cfg.buildDirPath, "bin", bin.name, "obj", obj_name+".o"))
	}

	for _, lib_name := range bin.dependencies {
		args = append(args, filepath.Join(cfg.buildDirPath, "lib", lib_name, lib_name+".a"))
	}

	cmd := exec.Command(cfg.cxx, args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to link binary %v: %v\n %s", binPath, err, output)
	}

	<-semaphore
	wg.Done()
}

type LinkAndRunTest struct {
	cfg          Config
	dependencies []string
	objPath      string
	outPath      string
	semaphore    chan struct{}
	wg           sync.WaitGroup
}

func linkAndRunTest(params LinkAndRunTest) {
	args := []string{}

	args = append(args, params.cfg.ldflags...)

	args = append(args, "-o")
	args = append(args, params.outPath)

	args = append(args, params.objPath)

	for _, lib_name := range params.dependencies {
		args = append(args, filepath.Join(params.cfg.buildDirPath, "lib", lib_name, lib_name+".a"))
	}

	cmd := exec.Command(params.cfg.cxx, args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to link test %v: %v\n %s", params.outPath, err, output)
	}

	// run the test
	cmd := exec.Command(params.outPath)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to run test %v: %v\n %s", params.outPath, err, output)
	}

	<-params.semaphore
	params.wg.Done()
}

func compileModuleFile(cfg Config, modFile ModuleFile, modCompiled chan string, semaphore chan struct{}) {
	args := []string{}

	args = append(args, cfg.cflags...)

	outFileName := strings.Join(strings.Split(modFile.moduleName, ":"), "-") + ".pcm"
	outPath := filepath.Join(cfg.buildDirPath, "module", outFileName)

	args = append(args, modFile.filePath)
	args = append(args, "--precompile")
	args = append(args, "-o")
	args = append(args, outPath)

	args = append(args, "-fprebuilt-module-path="+filepath.Join(cfg.buildDirPath, "module"))

	cmd := exec.Command(cfg.cxx, args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to compile %v: %v\n %s", modFile.filePath, err, output)
	}

	<-semaphore
	modCompiled <- modFile.moduleName
}

func compileCCFile(cfg Config, ccFile CCFile, ccCompiled chan string, semaphore chan struct{}) {
	args := []string{}

	args = append(args, cfg.cflags...)

	args = append(args, "-c")
	args = append(args, ccFile.filePath)
	args = append(args, "-o")
	args = append(args, ccFile.outPath)

	args = append(args, "-fprebuilt-module-path="+filepath.Join(cfg.buildDirPath, "module"))

	cmd := exec.Command(cfg.cxx, args...)
	output, err := cmd.CombinedOutput()
	if err != nil {
		log.Fatalf("Failed to compile %v: %v\n %s", ccFile.filePath, err, output)
	}

	<-semaphore
	ccCompiled <- ccFile.filePath
}

func compileTestAll() {

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
	cfg := readConfig()

	err := os.RemoveAll(cfg.buildDirPath)
	if err != nil {
		log.Fatalf("Failed to clean the build directory: %v", err)
	}

	err = os.MkdirAll(cfg.buildDirPath, os.ModePerm)
	if err != nil {
		log.Fatalf("Failed to create build directory: %v", err)
	}

	modFileSend := make(chan ModuleFile)
	ccFileSend := make(chan CCFile)
	parseFinished := make(chan struct{}, 1)
	modSend := make(chan Module)
	libSend := make(chan Library)
	binSend := make(chan Binary)

	go parseFolders(ParseFolders{
		cfg:         cfg,
		modFileSend: modFileSend,
		ccFileSend:  ccFileSend,
		finished:    parseFinished,
		modSend:     modSend,
		libSend:     libSend,
		binSend:     binSend,
	})
}
