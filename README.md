# MatfSimpleMem2Reg

## Intro

LLVM IR often represents local variables as stack slots using `alloca`, then **loads** and **stores** to read/write them. 
**Promoting to SSA** removes those memory operations and replaces them with **register-like SSA values**, which is simpler and faster.

This pass implements a subset of mem2reg:

1. **Single-store promotion**: if a stack slot has exactly one store that dominates all its loads, replace the loads with that stored value and delete the alloca + store.
2. **Restricted φ** (enabled with `-matf-phi`): if there’s a clean `if/else` diamond with one store to the same slot in each branch, insert one φ at the merge and delete the slot + branch stores.
3. **Zero-store**: if a slot is read but never written, replace loads with `undef` and delete the slot.
4. **Dead-store-only**: if a slot is written but never read, delete those stores and the slot.

## Build & run

* Build as part of `llvm/lib/Transforms/MatfSimpleMem2Reg` (`add_llvm_library(MatfSimpleMem2Reg)`).
* Run:

```
/bin/opt -load lib/MatfSimpleMem2Reg.so -enable-new-pm=0 -matf-simple-mem2reg -S input.ll -o output.ll
```

Enable the restricted φ case:

```
/bin/opt -load lib/MatfSimpleMem2Reg.so -enable-new-pm=0 -matf-simple-mem2reg -matf-phi -S input.ll -o output.llg 
```
---

## Minimal examples (C, and LLVM IR before and after)

### 1) Single-store promotion

C

```c
int f(int a, int b) {
    int x;
    x = a;
    return x + b + x;
}
```

BEFORE

```
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  %x = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  store i32 %b, i32* %b.addr, align 4
  %0 = load i32, i32* %a.addr, align 4
  store i32 %0, i32* %x, align 4
  %1 = load i32, i32* %x, align 4
  %2 = load i32, i32* %b.addr, align 4
  %add = add nsw i32 %1, %2
  %3 = load i32, i32* %x, align 4
  %add1 = add nsw i32 %add, %3
  ret i32 %add1
```

AFTER

```
  %add = add nsw i32 %a, %b
  %add1 = add nsw i32 %add, %a
  ret i32 %add1
```

---

### 2) Diamond φ

C

```c
int sel(int c, int a, int b) {
    int x;
    if (c) x = a; else x = b;
    return x + 1;
}
```

BEFORE

```
  %c.addr = alloca i32, align 4
  %a.addr = alloca i32, align 4
  %b.addr = alloca i32, align 4
  %x = alloca i32, align 4
  store i32 %c, i32* %c.addr, align 4
  store i32 %a, i32* %a.addr, align 4
  store i32 %b, i32* %b.addr, align 4
  %0 = load i32, i32* %c.addr, align 4
  %tobool = icmp ne i32 %0, 0
  br i1 %tobool, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  %1 = load i32, i32* %a.addr, align 4
  store i32 %1, i32* %x, align 4
  br label %if.end

if.else:                                          ; preds = %entry
  %2 = load i32, i32* %b.addr, align 4
  store i32 %2, i32* %x, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %3 = load i32, i32* %x, align 4
  %add = add nsw i32 %3, 1
  ret i32 %add
```

AFTER

```
  %tobool = icmp ne i32 %c, 0
  br i1 %tobool, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  br label %if.end

if.else:                                          ; preds = %entry
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %x.phi = phi i32 [ %b, %if.else ], [ %a, %if.then ]
  %add = add nsw i32 %x.phi, 1
  ret i32 %add
```

---

### 3) Zero-store to `undef`

C

```c
int g(void) {
    int u;
    return u;
}
```

BEFORE

```
  %u = alloca i32, align 4
  %0 = load i32, i32* %u, align 4
  ret i32 %0
```

AFTER

```
ret i32 undef
```

---

### 4) Dead-store-only

C

```c
int h(int a) {
    int dead;
    dead = a;
    return 0;
}
```

BEFORE

```
  %a.addr = alloca i32, align 4
  %dead = alloca i32, align 4
  store i32 %a, i32* %a.addr, align 4
  %0 = load i32, i32* %a.addr, align 4
  store i32 %0, i32* %dead, align 4
  ret i32 0
```

AFTER

```
ret i32 0
```

---

## Authors:
  - Milica Šopalović
  - Luka Arambašić
  - Željko Milovanović
