// const safety = @import("./safety.zig").safety;
// const arith = @import("./arith.zig");

// pub const Buf = struct {
//     buf: []u8,
//     offset: u64,

//     pub fn init(buf: []u8) Buf {
//         return Buf{
//             .buf = buf,
//             .offset = 0,
//         };
//     }

//     pub fn n_written(self: *const Buf) u64 {
//         return self.offset;
//     }

//     pub fn written(self: *const Buf) []const u8 {
//         return self.buf[0..self.offset];
//     }

//     pub fn reset(self: *Buf) void {
//         if (safety) {
//             @memset(self.buf, 0xaa);
//         }
//         self.offset = 0;
//     }

//     pub const PushErr = error{
//         NoCapacityLeft,
//     };

//     pub fn push_u64(val: u64) PushErr!void {
//         const MAX_NUM_DIGITS = 20;

//         var digits: [MAX_NUM_DIGITS]u8 = if (safety) @splat(0xaa) else undefined;

//         var pos: u64 = MAX_NUM_DIGITS;
//         var num = val;
//         while (num > 0) {
//             pos -= 1;
//         }
//     }
// };
