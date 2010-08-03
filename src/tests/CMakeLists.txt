SET(tests
	test_syscall
	test_linker
	test_xlinker
	test_ylinker
	test_normal_linker
	test_flush_logger
	test_segfault
	test_getpid
	test_sigign
	test_sleep
	test_sigaction
	test_sighandler
	test_sighandler_2
	test_sigprocmask
	test_rt_sigaction
	test_syscall_2
	test_fork
	test_signals
	test_threads
	)

build_interp_tests("${tests}" "")

ADD_EXECUTABLE(interp_as_normal_so interp_as_normal_so.c)
TARGET_LINK_LIBRARIES(interp_as_normal_so interp)

IF (BUILD_TESTS)
SET_PROPERTY(TARGET test_linker
	APPEND
	PROPERTY LINK_FLAGS "-Wl,-dynamic-linker,${CMAKE_BINARY_DIR}/src/interp/libinterp.so"
	)
SET_PROPERTY(TARGET test_segfault
	APPEND
	PROPERTY LINK_FLAGS "-Wl,-dynamic-linker,${CMAKE_BINARY_DIR}/src/interp/libinterp.so"
	)
SET_PROPERTY(TARGET test_xlinker
	APPEND
	PROPERTY LINK_FLAGS "-Wl,-dynamic-linker,${CMAKE_BINARY_DIR}/src/interp/libxinterp.so"
	)
SET_PROPERTY(TARGET test_ylinker
	APPEND
	PROPERTY LINK_FLAGS "-Wl,-dynamic-linker,${CMAKE_BINARY_DIR}/src/interp/libinterp.so"
	)
SET_PROPERTY(TARGET test_flush_logger
	APPEND
	PROPERTY LINK_FLAGS "-Wl,-dynamic-linker,${CMAKE_BINARY_DIR}/src/interp/libinterp.so"
	)

SET_PROPERTY(TARGET test_getpid
	APPEND
	PROPERTY LINK_FLAGS "-Wl,-dynamic-linker,${CMAKE_BINARY_DIR}/src/interp/libinterp.so"
	)

SET_PROPERTY(TARGET test_sigign
	APPEND
	PROPERTY LINK_FLAGS "-Wl,-dynamic-linker,${CMAKE_BINARY_DIR}/src/interp/libinterp.so"
	)

SET_PROPERTY(TARGET test_sleep
	APPEND
	PROPERTY LINK_FLAGS "-Wl,-dynamic-linker,${CMAKE_BINARY_DIR}/src/interp/libinterp.so"
	)

SET_PROPERTY(TARGET test_sigaction
	APPEND
	PROPERTY LINK_FLAGS "-Wl,-dynamic-linker,${CMAKE_BINARY_DIR}/src/interp/libinterp.so"
	)

SET_PROPERTY(TARGET test_threads
	APPEND
	PROPERTY LINK_FLAGS "-Wl,-dynamic-linker,${CMAKE_BINARY_DIR}/src/interp/libinterp.so")
TARGET_LINK_LIBRARIES(test_threads "-lpthread")

## SET(CAN_USE_ASSEMBLER TRUE)
## ENABLE_LANGUAGE(ASM-S)
## ADD_EXECUTABLE(test_syscall_asm test_syscall_asm.S)
## 
## SET_PROPERTY(TARGET test_syscall_asm
## 	APPEND
## 	PROPERTY LINK_FLAGS "--dynamic-linker=${CMAKE_BINARY_DIR}/src/interp/libinterp.so"
## 	)

ENDIF(BUILD_TESTS)

