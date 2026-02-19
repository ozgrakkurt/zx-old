.PHONY: all py_format py_check c_format

all: py_format py_check c_format

py_format:
	uvx ruff format *.py
py_check:
	uvx ty check *.py
c_format:
	clang-format -i -style=file src/*/*.c src/*/*.h include/*/*.h
git_repack:
	git repack -a -d -f --depth=250 --window=250
