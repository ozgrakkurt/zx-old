pub const ReadErr = error{
    OutOfBounds,
};

pub fn read(comptime T: type, slice: []const T, index: u64) ReadErr!T {
    if (index >= slice.len) {
        return ReadErr.OutOfBounds;
    }
    return slice[index];
}

pub const WriteErr = error{
    OutOfBounds,
};

pub fn write(comptime T: type, slice: []T, index: u64) WriteErr!T {
    if (index >= slice.len) {
        return WriteErr.OutOfBounds;
    }
    return slice[index];
}
