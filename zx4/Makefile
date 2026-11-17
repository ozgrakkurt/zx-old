cfmt:
	clang-format -i -style=file src/*/*.cc src/*/*.h include/*/*.h
pfmt:
	uvx --offline ruff format *.py
pchk: pfmt
	uvx --offline ty check *.py
git_repack:
	git repack -a -d -f --depth=250 --window=250
cfg: pfmt pchk
	python configure.py
fuzz: cfmt
	ninja -C build/fuzz0 fuzz
clean:
	rm -rf build && python configure.py

