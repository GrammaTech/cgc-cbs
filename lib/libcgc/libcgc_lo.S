.globl _start
_start:
#if defined(__arm__)
    bl cgc_initialize_secret_page
    bl main
    b _exit
#elif defined(__i386__)
    andl $-16, %esp
    call cgc_initialize_secret_page
    call main
    mov %eax, (%esp)
    call _exit
#elif defined(__x86_64__)
    andq $-16, %rsp
    call cgc_initialize_secret_page
    call main
    mov %eax, %edi
    call _exit
#else
#error unknown isa
#endif
