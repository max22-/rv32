fn print_int(a: i32) void {
    asm volatile ("li a7, 2" ::: "a7");
    asm volatile ("ecall"
        :
        : [a] "{a0}" (a),
    );
}

export fn _start() noreturn {
    asm volatile ("la sp, __stack_top");
    print_int(42);
    while (true) {}
}
