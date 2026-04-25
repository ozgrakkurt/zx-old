.PHONY: all py_format py_check c_format
all: py_format py_check c_format

py_format:
	uvx ruff format *.py
py_check:
	uvx ty check *.py
c_format:
	clang-format -i -style=file src/*/*.cc src/*/*.h include/*/*.h
git_repack:
	git repack -a -d -f --depth=250 --window=250
test:
	ninja -C build/test0 test && \
	ninja -C build/test1 test && \
	ninja -C build/test2 test && \
	ninja -C build/test3 test && \
	ninja -C build/test4 test && \
	ninja -C build/test5 test

