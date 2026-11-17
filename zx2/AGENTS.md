# AGENTS.md for zx2

Read `./README.md` and keep it in mind always, it is VERY IMPORTANT.

When doing reviews, be careful about these things:
- Out-of-bounds buffer access
- Integer overflow
- Null pointer dereference.
- Wrong size calculation when doing allocations.
- Issues that don't effect correctness but cause over-allocation or performance issues.
- In structs with methods:
    - Member variables not prefixed with `m_`.
    - Usage of member variables without `this->`.
    - Directly accessing struct member by name in code outside of the struct definition.

Do NOT modify or create files except `*_test.cc` files and `proj.py` (for registering new tests).

Do NOT run any build commands except `ninja -C build/debug test`.

Do NOT just use `(void)myvar;` pattern for ignoring unused parameters or variables.
Actually remove the parameter/variable so code stays clean.
