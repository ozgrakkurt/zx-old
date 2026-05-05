cfmt:
	clang-format -i -style=file src/*/*.cc src/*/*.h include/*/*.h
pfmt:
	uvx ruff format *.py
pchk: pfmt
	uvx ty check *.py
git_repack:
	git repack -a -d -f --depth=250 --window=250
cfg: pfmt pchk
	python configure.py
test: cfmt
	ninja -C build/test0 test && \
	ninja -C build/test1 test && \
	ninja -C build/test2 test && \
	ninja -C build/test3 test && \
	ninja -C build/test4 test && \
	ninja -C build/test5 test
fuzz: cfmt
	ninja -C build/fuzz0 fuzz
clean:
	rm -rf build && python configure.py

