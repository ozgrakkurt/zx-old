const safety = @import("./safety.zig").safety;

pub fn assert_dev(cond: bool) void {
    if (safety) {
        if (!cond) {
            @branchHint(.cold);
            @panic("assertion failed.");
        }
    }
}

pub fn assert(cond: bool) void {
    if (!cond) {
        @branchHint(.cold);
        @panic("assertion failed.");
    }
}
