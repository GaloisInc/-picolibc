# Copyright © 2021 Galois, Inc.  All rights reserved.

srcs_machine = [
  'misc.c',
  'nanocxx.cpp',

  'stack_protector.c',

  # Replace all the malloc implementation functions.
  'malloc-calloc.c',
  'malloc-cfree.c',
  'malloc-free.c',
  'malloc-mallinfo.c',
  'malloc-malloc.c',
  'malloc-malloc_stats.c',
  'malloc-malloc_usable_size.c',
  'malloc-mallopt.c',
  'malloc-memalign.c',
  'malloc-pvalloc.c',
  'malloc-realloc.c',
  'malloc-valloc.c',
]

inc_headers_machine = [
  'fromager.h',
]

srcs_machine_builtins = [
	'llvm_intrin.c',
	'llvm_intrin2.ll',
]

srcs_machine_syscalls = [
  'microram/syscalls.c',
]

srcs_machine_syscalls_native = [
  #'native/syscalls.c',
]

srcs_machine_syscalls_common = [
  'syscalls_common.c',
]

srcs_machine_native = [
  #'cc_native.c',
]

foreach target : targets
	value = get_variable('target_' + target)

	static_library('machine_builtins' + target,
		srcs_machine_builtins,
		pic: false,
        dependencies: [global_dep],
		c_args: value[1] + ['-fno-builtin'],
        install: true,
        install_dir: instdir)

	set_variable('lib_machine' + target,
		static_library('machine' + target,
			srcs_machine,
			pic: false,
            dependencies: [global_dep],
			c_args: value[1]))

    static_library('machine_syscalls' + target,
        srcs_machine_syscalls + srcs_machine_syscalls_common,
        pic: false,
        dependencies: [global_dep],
        c_args: value[1],
        install: true,
        install_dir: instdir)

    static_library('machine_syscalls_native' + target,
        srcs_machine_syscalls_native + srcs_machine_syscalls_common,
        pic: false,
        dependencies: [global_dep],
        c_args: value[1],
        install: true,
        install_dir: instdir)

    static_library('machine_native' + target,
        srcs_machine_native,
        pic: false,
        c_args: value[1],
        install: true,
        install_dir: instdir)
endforeach

install_headers(inc_headers_machine)
