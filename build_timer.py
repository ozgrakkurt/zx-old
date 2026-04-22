# clean:
# 	rm -rf build
# configure:
# 	./configure.py
# build_debug:
# 	ninja -C build/debug
# build_release:
# 	ninja -C build/release
# test_debug:
# 	ninja -C build/debug test
# test_release:
# 	ninja -C build/release test


import subprocess
import time

t0 = time.perf_counter()

subprocess.run(["rm", "-rf", "build"])
subprocess.run(["./configure.py"], check=True)
subprocess.run(["ninja", "-C", "build/debug"], check=True)

t1 = time.perf_counter()

subprocess.run(["rm", "-rf", "build"])
subprocess.run(["./configure.py"], check=True)
subprocess.run(["ninja", "-C", "build/release"], check=True)

t2 = time.perf_counter()

subprocess.run(["rm", "-rf", "build"])
subprocess.run(["./configure.py"], check=True)
subprocess.run(["ninja", "-C", "build/debug", "test"], check=True)

t3 = time.perf_counter()

subprocess.run(["rm", "-rf", "build"])
subprocess.run(["./configure.py"], check=True)
subprocess.run(["ninja", "-C", "build/release", "test"], check=True)

t4 = time.perf_counter()

print(f"Debug Build: {t1 - t0}")
print(f"Release Build: {t2 - t1}")
print(f"Debug Test: {t3 - t2}")
print(f"Release Test: {t4 - t3}")

print(f"Total Time: {t4 - t0}")
