# AGENTS.md for zx2

Read `./README.md` and keep it in mind always, it is VERY IMPORTANT.

When doing reviews, be careful about out-of-bounds buffer access, integer overflow, null pointer dereference.
Also be careful about potential performance issues while doing reviews.

Do NOT modify or create files except `*_test.cc` files and `proj.py` (for registering new tests).

Do NOT run any build commands except `ninja -C build/debug test`.

Do NOT just use `(void)myvar;` pattern for ignoring unused parameters or variables.
Actually remove the parameter/variable so code stays clean.
