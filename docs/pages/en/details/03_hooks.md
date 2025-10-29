\page hooks Hooks
\tableofcontents

## 1. 🌐 Introduction {#hooks-introduction}

This document describes in precise and detailed manner the operation of the `interceptor` architecture used in the rootkit to intercept, modify, and control system calls under Linux. The `interceptor` system relies on the *ftrace* infrastructure, where each *hook* can intervene before or after the native function execution, modify its arguments or result, and be dynamically enabled or disabled based on configuration files. The code organization is structured in two main modules: the core (`core`), responsible for installing and managing hooks, and the specific modules (`hooks`) that implement behaviors such as masking, forbidding, or modifying files and network ports.

```bash
interceptor
├── core
│   ├── include
│   │   ├── ftrace.h
│   │   ├── init.h
│   │   └── menu.h
│   ├── array.c               # Dynamic hook array management
│   ├── ftrace.c              # Ftrace mechanism implementation
│   ├── init.c                # Init/exit module with default files
│   └── menu.c                # Menu to add/remove files to process
└── hooks
    ├── alterate              # Alteration module        
    │   ├── alterate_api.c
    │   ├── alterate_api.h
    │   ├── alterate.c
    │   └── alterate.h
    ├── forbid                # Forbid module
    │   ├── forbid_api.c
    │   ├── forbid_api.h
    │   ├── forbid.c
    │   └── forbid.h
    └── hide                  # Hiding module
        ├── hide_api.c
        ├── hide_api.h
        ├── hide.c
        └── hide.h
```

## 2. 🏛️ History

Intercepting a system call in the Linux kernel can be done through several approaches, each with trade-offs in terms of performance, stability, and complexity. According to our research, one of the most direct methods consists of replacing an entry in the `sys_call_table` system call table with a pointer to a *wrapper* function. In this approach, whenever the kernel invokes a syscall number corresponding to the targeted index, it's redirected to our routine. While simple to implement and understand, starting from kernel versions 5.x, the table is no longer exported and becomes non-modifiable (write-protected), making this technique quite unstable... at least, we couldn't implement it on our Linux version (6.8.0-58-generic). We wanted to have the most recent kernel version possible, so we looked for another way. Faced with this constraint, we considered using *kprobes*, which insert a *breakpoint* (`int3`) on the target function. When called, an exception is raised, allowing execution of our kprobe handler. However, this technique incurs significant cost due to frequent interruptions. We feared that the presence of many simultaneous *kprobes* would cause a flood of interruptions and performance degradation.

This is why we turned to `ftrace`, the kernel's native instrumentation infrastructure (notably used to measure performance, create call graphs, etc.). The API exposes mechanisms like `register_ftrace_function` or the `ftrace_ops` structure, allowing definition of callbacks that receive the `pt_regs *` structure as parameter. From there, it becomes possible to read or modify registers, decide to call the original function or directly return an error code, and even modify the return value.

## 3. ⚙️ Core Components

### 3.1 🔍 Ftrace Mechanism

The *ftrace* mechanism of the interceptor module relies mainly on two files: core/include/ftrace.h, which defines the structure and prototypes, and core/ftrace.c, which implements the hook installation and removal functions. We describe here in detail each step, from symbol location to effective interception, illustrating with code excerpts.

#### 3.1.1 Foundations
```c
struct ftrace_hook{
    const char *name;       // Symbol name (syscall or function) to intercept
    void *function;         // Address of our hook function (wrapper)
    void *original;         // Native function address (backup)
    unsigned long address;  // Effective symbol address in the kernel
    struct ftrace_ops ops;  // Ftrace structure to manage interception
};
```

At the core of this mechanism is the ftrace_hook structure, which groups all necessary information for a hook to work.
- `name` contains the name of the symbol we want to monitor.
- `function` points to our *custom* function.
- `address` is used to keep the address thanks to the *kallsyms_lookup_name* function.
- `struct ftrace_ops` is used to communicate with the ftrace API.

For our *wrapper* to be able to invoke the original function, we also store the native function address in `original`. In the fh_install_hook code from ftrace.c, we observe:
```c
hook->address = kallsyms_lookup(hook->name);
...
*((unsigned long *)hook->original) = hook->address;
```
Here, the first line resolves the symbol address and stores it in `hook->address`. This value is then used to instruct ftrace. As for `hook->original`, it points to a variable of type `unsigned long` defined in the hook module itself (for example, `__orig_read` in the case of a hook on `read`) and allows the *wrapper*, later, to call the real kernel function.

#### 3.1.2 Macros

To facilitate declaring a hook on a syscall, we introduced three macros. SYSCALL_NAME(name) automatically prefixes the string with *__x64_*, so we don't have to manually write the exact symbol name. The HOOK_SYS(_name, _hook, _orig) macro then simplifies initializing an ftrace_hook element by providing in a single line the syscall name, our hook function address, and the variable where the original pointer will be stored. For kernel functions that aren't syscalls, we can use the more generic HOOK(_name, _hook, _orig) macro.

```c
#define SYSCALL_NAME(name) ("__x64_" name)
#define HOOK_SYS(_name, _hook, _orig) {					\
    .name = SYSCALL_NAME(_name),                        \
    .function = (_hook),                                \
    .original = (_orig),                               	\
}

#define HOOK(_name, _hook, _orig) {					    \
    .name = (_name),                                    \
    .function = (_hook),                                \
    .original = (_orig),                               	\
}
```

The rest of ftrace.h consists mostly of prototypes.
- `fh_init_kallsyms_lookup` is used to retrieve a pointer to the internal `kallsyms_lookup_name()` function, by temporarily installing a kprobe.
- `fh_install_hook` and `fh_remove_hook` respectively handle hook installation and removal.
- `fh_install_hooks` and `fh_remove_hooks` allow bulk management of a hook array.

The complete list of implemented hooks can be found in array.c:
```c
struct ftrace_hook hooks[] = {
    HOOK_SYS("sys_getdents64", getdents64_hook, &__orig_getdents64),
    HOOK_SYS("sys_read", read_hook, &__orig_read),
    HOOK_SYS("sys_openat", openat_hook, &__orig_openat),
    HOOK_SYS("sys_newfstatat", stat_hook, &__orig_newfstatat),
    HOOK_SYS("sys_fstat", stat_hook, &__orig_fstat),
    HOOK_SYS("sys_lstat", stat_hook, &__orig_lstat),
    HOOK_SYS("sys_stat", stat_hook, &__orig_stat),
    HOOK_SYS("sys_recvmsg", recvmsg_hook, &__orig_recvmsg),
    HOOK_SYS("sys_chdir", chdir_hook, &__orig_chdir),

    HOOK("tcp4_seq_show", tcp4_seq_show_hook, &__orig_tcp4_seq_show),
    HOOK("tcp6_seq_show", tcp6_seq_show_hook, &__orig_tcp6_seq_show)
};
```

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>
