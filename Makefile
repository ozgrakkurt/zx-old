py_format:
	uvx ruff format *.py
py_check:
	uvx ty check *.py
c_format:
	clang-format -i -style=file src/*/*.c include/*/*.h
