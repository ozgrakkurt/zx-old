# zx build system

- modules
  - source
  - tests
- libraries
  - source
  - tests
- binaries
  - source

# Module compilation

- Read deps.json file to determine dependencies.
- Find dependencies for each submodule. Submodules can only depend on explicitly imported dependency root modules
  or submodules of the current root module.
- Each file generates one .pcm and one .o file.
- .o files of a single root module are archived into a single .a file. And this .a file is linked into any binary
  target that depends on this module.
- Dependencies on root modules are handled by specifying the .pcm file as `-fmodule-file=<module-name>=<path/to/BMI>` while
  compiling.
- Tests inherit both dependencies and test_dependencies from deps.json. They implicitly depend on the root module.
  They are basically binary targets but are run automatically because they are tests and they have implicit dependency
  to the current root module.
- Should use :partition way to avoid naming conflicts between submodules or different root modules.
-  Might need to specify internal partition pcm paths to the dependants of a module? Or maybe
  just need to specify the path of the root module pcm?

# Library

- Read deps.json file
- Find dependencies for each object.
- Each object is compiled in parallel.
- Same idea on tests/archiving as module. All files inside lib source are compiled to object files and archived into .a file.
- Tests depend on current library implicitly.
- Find specify .pcm files as `-fmodule-file=` using the deps.json information.
- Only need clang-scan-deps on object source files to do integrity checking.

# Binary

- Same as library but doesn't have tests and is linked into an executable at the end.
