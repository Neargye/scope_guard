# scope_guard review — 2026-10-08

Review branch only: the owner takes the fix and `[proposal]` commits; this file, `review/` and
`.github/workflows/review.yml` are review-only and are not for merging.

Base: `a94902057688bd86b9b76e911104be8be0a46120` ("v0.9.5"). Commits were made in an isolated worktree and moved to the
review branch `claude/great-keller-kgo1pw` by the orchestrator (hashes below are the review-branch hashes).
Build and probe directory: `/tmp/scope_guard-step3/` (container only).
Toolchains: g++-13.3.0, clang++-18.1.3 (libstdc++ 13, x86-64), cmake 3.28.3, ninja 1.11.1.
"Old header" means `git show a949020:include/scope_guard.hpp`.

```
d111547 [proposal] delete array new for scope guard
09eda2d [proposal] separate guards by action setting
9374b11 fix integration tests make program
ba6d22e fix config macros
```

Selection: every hypothesis already had a verdict (evidence, "Hypothesis verdicts"), so no new probes were needed. Step 2 found
no confirmed problems. Step 2b confirmed 16 hypotheses. By impact, only H4 is med; all other confirmed items are low.
Fixed: H4 (med) as a [proposal], because it changes mangled names. Also fixed: the low items whose fix is trivial and obviously
safe, H3, H9 and H21(a). H12 (low, trivial, but it rejects code that compiled before) is a separate [proposal]. Everything else
is in Decisions D1–D19.

## Fixes

### ba6d22e `fix config macros` (H3, H9)

**H3: SUPPRESS mode under Clang `-fexceptions -fno-cxx-exceptions`.**
- Problem: the exceptions-enabled check `defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)` is true for Clang with `-fexceptions -fno-cxx-exceptions`, where only `__EXCEPTIONS` is defined. So the SUPPRESS destructor got `try {` and the TU did not compile.
- Evidence (old header): `clang++ -std=c++11 -fexceptions -fno-cxx-exceptions -DSCOPE_GUARD_SUPPRESS_THROW_ACTION test/config_no_exceptions.cpp` → `scope_guard.hpp:242:7: error: cannot use 'try' with exceptions disabled` (same for c++17).
- Change: `__EXCEPTIONS` is used only when `!defined(__clang__)`. Clang defines `__cpp_exceptions` whenever C++ exceptions are on. clang-cl defines `_CPPUNWIND` only with C++ exceptions.
- Test: `scope_guard-no-cxx-exceptions-suppress-throw-action.t` (Clang, non-MSVC only): `config_no_exceptions.cpp` with SUPPRESS and `-fexceptions -fno-cxx-exceptions`. Old header: build fails. New header: builds, `run rc=0` (c++11, c++17). It cannot fail on GCC, because the flag is Clang-only. CI jobs that run it: ubuntu clang-16/17/18, macos AppleClang.
- Remaining: none known. Extra check: clang++ c++11/14/17/20/2c × {SUPPRESS, MAY_THROW, NO_THROW} with `-fexceptions -fno-cxx-exceptions`: 15/15 build+run.

**H9: a user-provided `NEARGYE_SCOPE_GUARD_NODISCARD` was `#undef`-ed.**
- Problem: the macro is overridable (`#if !defined(...)`), but the header undefined it unconditionally at the end. The other overridable macros (`MAYBE_UNUSED`, `STR_CONCAT`, `COUNTER`) are kept.
- Evidence (old header): `config_test.cpp -DSCOPE_GUARD_TEST_USER_NODISCARD` → `error: #error "scope_guard.hpp must keep a user-provided NEARGYE_SCOPE_GUARD_NODISCARD."` on g++-13 and clang++-18, c++11 and c++17.
- Change: when the header defines the macro itself, it also sets an internal `NEARGYE_SCOPE_GUARD_UNDEF_NODISCARD`. At the end it undefines both only in that case.
- Test: new scenario `SCOPE_GUARD_TEST_USER_NODISCARD` in `test/config_test.cpp`, target `scope_guard-user-nodiscard.t`. Old header: `#error` (build fails). New header: `[doctest] test cases: 1 | 1 passed`. Every config test also `#error`s if the header's own definition is still defined after the include.
- Remaining: none.

### 9374b11 `fix integration tests make program` (H21 part a)

- Problem: `test/integration/run_consumer.cmake` re-configures nested builds with the parent's generator and compiler, but not with `CMAKE_MAKE_PROGRAM`. So with `-G Ninja -DCMAKE_MAKE_PROGRAM=<path>` and no ninja on `PATH`, all 4 integration tests failed.
- Evidence (old tree, PATH without ninja): `0% tests passed, 4 tests failed out of 4`, `"Ninja".  CMAKE_MAKE_PROGRAM is not set.`
- Change: `test/integration/CMakeLists.txt` passes `-DSCOPE_GUARD_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}`. `run_consumer.cmake` forwards it only for generators matching `Ninja|Makefiles`, so Visual Studio and Xcode behave as before.
- Test: the 4 existing integration tests are the regression test. The failure needs an environment without the make program on `PATH`, so it is not a ctest case. New tree, same setup: `100% tests passed, 0 tests failed out of 4`.
- Remaining: `CMAKE_CXX_FLAGS` and `CMAKE_TOOLCHAIN_FILE` are still not forwarded (H21 part b, D15). Consumers in sanitizer or cross builds are built with default flags.

## Proposals

### 09eda2d `[proposal] separate guards by action setting` (H4, impact med)

- What changes: `scope_guard`, the `scope_exit/fail/success` aliases, the factories, the tags and `operator<<` move into an inline namespace inside `scope_guard::detail`. The namespace is named after the effective action setting: `may_throw_action`, `no_throw_action` or `suppress_throw_action`. SUPPRESS without C++ exceptions produces the same tokens as MAY_THROW, so it uses `may_throw_action`. Qualified names (`scope_guard::detail::scope_exit<F>`, `scope_guard::make_scope_exit`), ADL for the macros and all macros are unchanged.
- Why: the action setting changes the destructor body and `noexcept` of the same specialization. Two TUs with different settings (for example, two header-only libraries in one program) silently violate the ODR, and the linker keeps only one destructor.
  - Evidence (old header): a default TU and a SUPPRESS TU both call `make_scope_exit(&throwing_action)`. On g++-13 and clang++-18, c++11 and c++17, `-O0` and `-O3 -fno-inline`, link order A gives `REQUIRE( suppressed_in_other_translation_unit() ) THREW exception: "cleanup failure"` and link order B gives `REQUIRE_THROWS_AS(...) did NOT throw at all!`.
  - With plain `-O3` the destructor is inlined and both orders pass, so optimized builds hide the bug.
  - New header: both orders pass in all 24 builds. `nm -C` shows `scope_guard::detail::suppress_throw_action::scope_guard<void (*)(), ...>::~scope_guard()` and `...::may_throw_action::...` as separate symbols.
- Test: `scope_guard-mixed-action-settings.t` links `test/config_mixed_test.cpp` (default setting) with `test/config_mixed_suppress.cpp` (SUPPRESS). It is built with `-fno-inline` (GCC/Clang) or `/Ob0` (MSVC), so a shared destructor is observable in Release too. Fails on the old header in both link orders, passes on the new one.
- Risks:
  - The mangled names of guard types change. This is an ABI break for anyone who exports functions taking `scope_guard::detail::*` types across a binary boundary built with the old header. A mismatch now gives a link error instead of a silent ODR violation.
  - Code that forward-declares `scope_guard::detail::scope_guard` would break (it is a `detail` name).
  - On MSVC, `/Ob0` after `/Ob2` gives command-line warning D9025, which `/WX` does not turn into an error. This is verified only by CI.
- Limitations (D18): `SCOPE_GUARD_CATCH_HANDLER` and `SCOPE_GUARD_NO_THROW_CONSTRUCTIBLE` are not encoded. The handler is arbitrary code, and NO_THROW_CONSTRUCTIBLE only adds a `static_assert`. The README rule "Define exception settings consistently in every translation unit" is still correct and unchanged.
- To reject: `git revert 09eda2d`.

### d111547 `[proposal] delete array new for scope guard` (H12, impact low)

- What changes: `operator new[]` and `operator delete[]` of `scope_guard` are `= delete`, like the scalar forms.
- Why: the scalar delete is meant to keep guards off the heap, but `new G[1]{std::move(g)}` compiled and ran (old header: `rc=0` on g++/clang++ c++11/c++17).
- Test: scenario `SCOPE_GUARD_TEST_REJECT_ARRAY_NEW` in `test/failure_test.cpp`, target `scope_guard-compile-fail-array-new.t`, expected text `deleted function`.
  - Old header: the scenario compiles, so the test reports "expected to fail, but succeeded".
  - New header: g++ reports `use of deleted function '... operator new [](std::size_t) ...'`, clang++ reports `call to deleted function 'operator new[]'`.
  - The expected text also matches MSVC C2280 ("attempting to reference a deleted function"). CI only.
- Risks: code that heap-allocated arrays of guards stops compiling, which is the intent. `::new` and allocators are unaffected.
- To reject: `git revert d111547`.

## Found but not fixed

| item | reason | decision |
|---|---|---|
| H1 cxxabi offset on non-x86-64 ABIs | UNCLEAR, needs review CI (`H1.cpp`, `R1.cpp`) | D1 |
| H2 `< 407` clause matches Clang | FORMAL, no failure with real headers; CI `R2.cpp` | D2 |
| H5 throwing move does not run the action | intentional, pinned by test; std parity = API | D3, Q2 |
| H6 factories not SFINAE-friendly | changes pinned diagnostic and overload behavior (API) | D4, Q3 |
| H7 misleading diagnostics for const/volatile rvalues, functions | low impact, belongs with H6 | D5 |
| H8 factory `noexcept` includes destructor | low impact, changes function type (API) | D6 |
| H10, H11 MSVC attribute/pragma | UNCLEAR, CI only | D7 |
| H13 CATCH_HANDLER name binding | low, README note only | D8 |
| H14 noexcept function pointers before C++17 | language limitation | D9 |
| H15 forced unwind in SUPPRESS mode | no reliable portable fix | D10 |
| H16, H17 formal ODR (`__cplusplus`, `__COUNTER__`) | no observable effect | D11 |
| H18 implementation chosen by compiler identity | UNCLEAR, no test platform | D12 |
| H19 unprefixed macros | design, API | D13 |
| H20 `AnyNewerVersion` | packaging policy | D14, Q4 |
| H21(b) flags not forwarded to consumers | coverage choice | D15 |
| H22 examples without `-std=` | low impact, examples only | D16 |
| H23 compile time with thousands of guards | unrealistic | D17 |
| O2, O4, O6, O7 | compiler semantics / style / std library | D19 |

## Questions for the owner

- Q1: Accept `[proposal] separate guards by action setting` (H4)? It changes the mangled names of guard types. In return, TUs with different action settings no longer share a destructor.
- Q2: Should a throwing action move run the action, as P0052 / LFTS v3 do (H5)? Today `config_test.cpp` pins the current behavior ("a throwing action move does not establish a guard").
- Q3: Should the factories become SFINAE-friendly for lvalues (H6)? The cost is the current `static_assert` message pinned by `compile-fail-lvalue-action.t`. The H7 diagnostic improvements depend on this choice.
- Q4: Keep `COMPATIBILITY AnyNewerVersion` for a 0.x package (H20), or switch to `SameMinorVersion` (and to `SameMajorVersion` from 1.0)?
- Q5: Accept `[proposal] delete array new for scope guard` (H12)?

## Verification on HEAD (container)

| configuration | result |
|---|---|
| g++-13 Debug, full ctest | 24/24 passed (re-run by the orchestrator at acceptance) |
| clang++-18 Debug, full ctest | 25/25 passed (includes the no-cxx-exceptions test; re-run at acceptance) |
| g++-13 Release (-O3 -DNDEBUG) | 24/24 |
| clang++-18 Release | 25/25 |
| g++-13 Debug ASan+UBSan (`-fsanitize=address,undefined -fno-sanitize-recover=all`) | 24/24, no sanitizer report |
| clang++-18 Debug UBSan trap | 25/25 |
| g++-13 / clang++-18 Debug `-fno-rtti` | 24/24, 25/25 |
| test.cpp: g++ c++23, clang++ c++23/c++2c. config_test.cpp × 5 scenarios (incl. USER_NODISCARD), the mixed test (`-fno-inline`) and `-fno-exceptions` SUPPRESS: × g++ {c++14,17,20,23,gnu++11} and × clang++ {the same + c++2c}. Clang `-fexceptions -fno-cxx-exceptions` × 3 settings × 5 stds | 95/95 build+run |

In the sanitizer build, the 4 integration tests still build their consumer without `CMAKE_CXX_FLAGS` (D15).

Not verified here; review CI or the existing CI after push covers:
- MSVC and ClangCL: the new `/Ob0` option on `mixed-action-settings.t`, the C2280 text for `compile-fail-array-new.t`, and the inline namespace with the MSVC pre-C++17 branches.
- AppleClang/libc++abi: `no-cxx-exceptions-suppress-throw-action.t` runs there, because the compiler ID matches `Clang`.
- GCC 12/14, Clang 16/17, -m32.
- Clang ASan: no compiler-rt in the container, so UBSan trap mode was used instead.
- The H1/H2/H10/H11 probes listed in the evidence.

## CI extension proposal (not applied)

The fixed problems now have tests that run in the existing jobs (ubuntu GCC/Clang, macos, windows), so they need no new job. The existing CI has no sanitizer job. A minimal permanent addition is one ubuntu job: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=g++-14 -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all"`, followed by `ctest --test-dir build --output-on-failure`. Review CI (below) also suggests adding 32-bit jobs (Linux `-m32`, MSVC Win32) if its probes find anything there.

## Decisions

Source: no earlier review branch found (git log --all -- review-report.md is empty on 2026-10-08); file created fresh.

| ID | decision | why | reference |
|---|---|---|---|
| D1 | H1 (pre-C++17 `__cxa_eh_globals` offset) left as is | UNCLEAR: correct on x86-64 libstdc++ (evidence H1, row 28); other ABIs need review CI (`H1.cpp`, `R1.cpp`: macos-appleclang, clang-libcxx, gcc-m32, gcc-latest). Awaiting CI | hypotheses H1; evidence verdict H1 |
| D2 | H2 (`< 407` clause also matches Clang, `noexcept` redeclaration) left as is | FORMAL: no failure with any real ABI header (libstdc++ declares it `noexcept`); breakage only with a simulated header. libc++abi checked by review CI `R2.cpp`. Awaiting CI | hypotheses H2; evidence verdict H2 |
| D3 | H5 (throwing action move does not run the action, unlike P0052) left as is | Intentional behavior, pinned by `test/config_test.cpp` "a throwing action move does not establish a guard". std parity would be an API change: owner question | hypotheses H5; evidence verdict H5; report Q2 |
| D4 | H6 (factories not SFINAE-friendly for lvalues) left as is | Changing it moves the check out of the pinned `static_assert` diagnostic (`compile-fail-lvalue-action.t`) and changes overload behavior: API. Low impact | hypotheses H6; evidence verdict H6; report Q3 |
| D5 | H7 (misleading diagnostics for const/volatile rvalues and `std::move(function)`) deferred | Low impact, compile-time diagnostics only; belongs together with the H6 decision (static_assert vs SFINAE) | hypotheses H7; evidence verdict H7 |
| D6 | H8 (factory `noexcept` includes the guard destructor) deferred | Low impact (visible only in the C++17 function type); the fix changes the factories' function type: API | hypotheses H8; evidence verdict H8 |
| D7 | H10, H11 (MSVC C++14 NODISCARD, `warning(suppress)` pragma) left as is | UNCLEAR: MSVC only, not in container. Review CI probes `H10.cpp`, `H11.cpp`, `R6.cpp`, `R7.cpp`. Awaiting CI | evidence verdicts H10, H11 |
| D8 | H13 (`SCOPE_GUARD_CATCH_HANDLER` names bound at the include point) left as is | Low impact; README-only clarification ("declare names used by the handler before the include"); not necessary for a fix | hypotheses H13; evidence verdict H13 |
| D9 | H14 (NO_THROW_ACTION rejects `noexcept` function pointers before C++17) left as is | Language limitation (P0012: `noexcept` is part of the function type only since C++17) | hypotheses H14; evidence verdict H14 |
| D10 | H15 (SUPPRESS `catch (...)` swallows glibc forced unwind → abort on thread cancellation) left as is | Design limit of a `noexcept` destructor with `catch (...)`; no reliable portable fix (rethrowing is impossible in a `noexcept` destructor, `abi::__forced_unwind` is glibc-specific) | hypotheses H15; evidence verdict H15 |
| D11 | H16, H17 (inline `uncaught_exceptions()` differs by `__cplusplus`; `__COUNTER__` names differ between TUs) left as is | FORMAL ODR only, no observable effect (evidence H16, H17) | evidence verdicts H16, H17 |
| D12 | H18 (implementation chosen by compiler identity, `__cxa_get_globals` in `-fno-exceptions` C++11/14) left as is | UNCLEAR: affected runtimes (Emscripten no-exception libc++abi, non-GNU C++11 compilers) are neither in the container nor in review CI | evidence verdict H18 |
| D13 | H19 (unprefixed `SCOPE_EXIT`/`DEFER` macros clash with folly etc.) left as is | Design; renaming or `#ifndef` guards change the public API | hypotheses H19; evidence verdict H19 |
| D14 | H20 (`AnyNewerVersion` package compatibility for 0.x) left as is | Packaging policy is the owner's choice; changing it changes which `find_package` requests succeed | hypotheses H20; evidence verdict H20; report Q4 |
| D15 | H21 part (b): integration consumers are not built with `CMAKE_CXX_FLAGS`/`CMAKE_TOOLCHAIN_FILE` left as is | Only `CMAKE_MAKE_PROGRAM` (the observed failure) was forwarded in `fix integration tests make program`; forwarding flags changes what the integration tests check (coverage choice, not a failure) | hypotheses H21; evidence verdict H21 |
| D16 | H22 (examples built without `-std=`, CMP0128 OLD) deferred | Low impact: examples are not tests, C++11 paths are covered by `scope_guard-cpp11.t`; a fix needs a policy change in example/CMakeLists.txt | hypotheses H22; evidence verdict H22 |
| D17 | H23 / O8 (compile time with thousands of guards in one function) left as is | Unrealistic usage; most of the growth is the compilers' EH cleanup handling | evidence verdict H23, O8 |
| D18 | `[proposal] separate guards by action setting` does not encode `SCOPE_GUARD_CATCH_HANDLER` or `SCOPE_GUARD_NO_THROW_CONSTRUCTIBLE`; README "configure consistently" kept | The handler is an arbitrary statement (cannot be named in a namespace); NO_THROW_CONSTRUCTIBLE only adds a `static_assert` (no code difference). Consistent configuration is still the documented rule | step 3 report, H4 |
| D19 | O2 (`(void)` does not silence GCC `warn_unused_result` in C++11/14), O4 (Clang `-Weverything` notes in test sources), O6 (`-Wpadded`, `-Wunsafe-buffer-usage` in the header), O7 (libstdc++ 13 `std::experimental::scope_exit` release+move) left as is | GCC attribute semantics / style notes outside the project's warning set / expected for the design / standard-library defect, not the project | evidence O2, O4, O6, O7 |

## Hypothesis verdicts

Step 2b, revision `a94902057688bd86b9b76e911104be8be0a46120`, working tree clean (`git status --short` empty before and after). No repository file was changed.
Work dir: `/tmp/scope_guard-step2b/` (logs in `logs/`; build trees, binaries and some probe sources were deleted at the end to free space — the probe texts are the ones in hypotheses.md, with the changes listed per row). `INC=/home/user/scope_guard/include`, `FLAGS="-Wall -Wextra -pedantic-errors -Werror"`.
Compilers: g++-13.3.0, clang++-18.1.3 (libstdc++ 13, x86-64). Probes run as written in hypotheses.md unless the comment says "probe fixed".

| H | verdict | command | key output line | comment |
|---|---|---|---|---|
| H1 | UNCLEAR | `for c in g++ clang++; for s in c++11 c++14: $c -std=$s $FLAGS -I$INC h1.cpp && ./h1`; also g++ `-fsanitize=address,undefined` | `ok` rc=0 in all 6 runs (logs/H1.log) | The fixed offset is right on x86-64 libstdc++ (container; also evidence row 28, depth 2000). Negative control: header copy with offset `+ sizeof(void*) + sizeof(int)` → H1 probe prints `expected 2, got 0` / `fail=0 success=2`, R1 prints `fail_runs=0 success_runs=6` (logs/R1-negative-control.log), so the probes do detect a wrong offset. Needs -m32 multilib (`-m32`: `fatal error: bits/c++config.h: No such file or directory`, logs/runner-gcc-m32.log), libc++abi, AppleClang arm64 → `H1.cpp`, `R1.cpp` |
| H2 | FORMAL | (1) probe fixed: the original `grep -c '__cxa_get_globals() noexcept'` also counted libstdc++'s own declaration (g++ 1, clang++ 2), so it now counts `extern "C" __cxa_eh_globals* __cxa_get_globals` lines from scope_guard.hpp; (2) `clang++ -std=c++11 $FLAGS -isystem fake -I$INC h2a.cpp -c` with a `fake/cxxabi.h` that has no exception spec | (1) `g++ c++11/c++14: 0`, `clang++ c++11/c++14: 1`, both `c++17: 0`; (2) `scope_guard.hpp:72:30: error: exception specification in declaration does not match previous declaration` (clang c++11/14); g++ all std and clang++ c++17: rc=0 (logs/H2.log) | The `< 407` clause matches every Clang (fact). With real ABI headers there is no effect: libstdc++ declares it `noexcept` (cxxabi-first/last pass, evidence rows 12, 23; R2 holds in the runner), and libc++abi is the intended target of the branch. Breakage needs an ABI header without a spec (simulated only). libc++abi/AppleClang → `R2.cpp` |
| H3 | CONFIRMED | `clang++ -std=c++11 $FLAGS -fexceptions -fno-cxx-exceptions -I$INC h3.cpp` (and c++17) | `scope_guard.hpp:242:7: error: cannot use 'try' with exceptions disabled` | Controls: clang++/g++ `-fno-exceptions` build and run rc=0; the same TU without SUPPRESS builds with `-fexceptions -fno-cxx-exceptions` (rc=0). Macros: only `#define __EXCEPTIONS 1` (logs/H3.log) |
| H4 | CONFIRMED | `$c -std=$s -O0 $FLAGS -I$INC h4_a.cpp h4_main.cpp -o a && ./a`, and the reverse link order; g++/clang++ × c++11/c++17 × -O0/-O2 | -O0: order a,main prints `0`, order main,a prints `11` (rc=1) on both compilers and both standards; `nm -C a`: one `W scope_guard::detail::scope_guard<void (*)(), ...on_exit_policy>::~scope_guard()` | At -O2 the destructor is inlined and both orders print `1`. Documented requirement ("configure consistently"); observable at -O0 (logs/H4.log) |
| H5 | CONFIRMED | `h5.cpp`: action whose move/copy throw; project `make_scope_exit` vs libstdc++ `std::experimental::scope_exit` (g++/clang++, c++11/c++20); existing config test `-DSCOPE_GUARD_TEST_THROWING_MOVE_CONSTRUCTION` | `project: caught move, runs=0` vs `std::experimental: caught copy, runs=1`; config test `4 passed` | Intentional, pinned by `config_test.cpp:118-127`. Differs from P0052 (std calls `f()` when storing throws) (logs/H5.log) |
| H6 | CONFIRMED | `$c -std={c++11,c++17} $FLAGS -I$INC -fsyntax-only h6.cpp` | g++: `scope_guard.hpp:253:82: error: use of deleted function '...scope_guard(A&) [with F = A&; ...]'`; clang++: `scope_guard.hpp:253:91: error: call to deleted constructor of 'scope_exit<A &>'` | Hard error from the factory's noexcept-spec, not a substitution failure. Control `-DRVALUE_ONLY` (no lvalue query): rc=0 (logs/H6.log) |
| H7 | CONFIRMED | `$c -std={c++11,c++17} $FLAGS -D{CONST_RVALUE,VOLATILE_RVALUE,FUNC_MOVE,FUNC_PTR} -I$INC -fsyntax-only h7.cpp` | CONST_RVALUE: `rvalue-msg=0`, `use of deleted function '...scope_guard(const A&)'`; FUNC_MOVE: `static assertion failed: make_scope_exit requires an rvalue action; use std::move or pass a temporary.` (for `std::move(func)`) | VOLATILE_RVALUE also misses the rvalue message, but reports `no matching function/constructor` and `discards qualifiers` instead of "deleted". FUNC_PTR (`&func`) builds (logs/H7.log) |
| H8 | CONFIRMED | `$c -std={c++17,c++20} $FLAGS [-DSCOPE_GUARD_SUPPRESS_THROW_ACTION] -I$INC -fsyntax-only h8.cpp`; C++11 `noexcept(G{A{}})` print | default: `h8.cpp:9: static assertion failed: factory noexcept follows the action move only`; SUPPRESS: rc=0; C++11: `noexcept(G{A{}})=0 is_nothrow_move_constructible<A>=1` (default), `=1` (SUPPRESS) | Visible only in the C++17 function type (`&make_scope_exit<A>` is not `noexcept`). A call-site `noexcept(make_scope_exit(...))` includes the destructor anyway. The extra assert at h8.cpp:5 (`is_nothrow_constructible<G, A&&>`) also fails, but for an unrelated reason: the trait includes the destructor (U5), so it is not evidence (logs/H8.log) |
| H9 | CONFIRMED | `$c -std=c++17 $FLAGS -I$INC -fsyntax-only h9.cpp` (user defines NODISCARD and MAYBE_UNUSED before the include) | `h9.cpp:8:4: error: #error user-provided NEARGYE_SCOPE_GUARD_NODISCARD was undefined by scope_guard.hpp` | User-defined `NEARGYE_SCOPE_GUARD_MAYBE_UNUSED` survives (no error on line 5), so the behavior is inconsistent (logs/H9.log) |
| H10 | UNCLEAR | — | — | Needs MSVC (not in container) → `H10.cpp` (msvc-x64, msvc-x86). ClangCL uses the `__clang__` branch; the GCC/Clang analogue warns on c++14 and c++17 (`-Wunused-result`, logs/msvc-probes-local-sanity.log; evidence row 19) |
| H11 | UNCLEAR | — | — | Needs MSVC → `H11.cpp` (msvc-x64, msvc-x86). GCC/Clang do not use the pragma branch (`unused variable 'unused_in_action'` on c++14 and c++17) |
| H12 | CONFIRMED | `$c -std={c++11,c++17} $FLAGS -I$INC h12.cpp && ./h12`; g++ ASan+UBSan | `new G[1]{std::move(g)}` builds, rc=0, no sanitizer report | Control `-DSCALAR` (`new G{...}`): g++ `use of deleted function 'static void* ...operator new(std::size_t)'`, clang++ `call to deleted function 'operator new'` (logs/H12-H13.log) |
| H13 | CONFIRMED | `$c -std={c++11,c++17} $FLAGS -I$INC h13.cpp` (handler function declared after the include) | g++: `h13.cpp:2:35: error: there are no arguments to 'report' that depend on a template parameter, so a declaration of 'report' must be available`; clang++: `scope_guard.hpp:244:7: error: use of undeclared identifier 'report'` | Control `-DDECLARE_FIRST`: build and run rc=0. README-only issue (logs/H12-H13.log) |
| H14 | CONFIRMED | `$c -std={c++11,c++14,c++17,c++20} $FLAGS -I$INC h14.cpp` (`SCOPE_GUARD_NO_THROW_ACTION`, `&cleanup` with `void cleanup() noexcept`) | c++11/c++14: `scope_guard.hpp:202: static assertion failed: scope_guard requires noexcept invocable action.`; c++17/c++20: rc=0 | Same result on both compilers. Language limitation (P0012) (logs/H14-H15.log) |
| H15 | CONFIRMED | `$c -std={c++11,c++17} $FLAGS [-D...] -pthread -I$INC h15.cpp && ./h15` | default: `canceled` rc=0; SUPPRESS: `FATAL: exception not rethrown`, rc=134 (abort) | Same result on both compilers and standards. Extra fact: with `SCOPE_GUARD_NO_THROW_ACTION` the macro lambda is `noexcept`, so the forced unwind gives `terminate called without an active exception` (rc=134). That is a language rule for `noexcept` functions, not the `catch (...)` (logs/H14-H15.log) |
| H16 | FORMAL | `g++ -std=c++11 -O0 -c t11.cpp; g++ -std=c++17 -O0 -c t17.cpp; nm -C`; link both orders, `objdump` | `W scope_guard::detail::uncaught_exceptions()` in both objects; t11 needs `U __cxa_get_globals`, t17 `U std::uncaught_exceptions()`; order 11,17 keeps `call __cxa_get_globals@plt`, order 17,11 keeps `call std::uncaught_exceptions()@plt`; both print `0 0` | Two different inline definitions, and the linker picks one by order; on libstdc++ both read the same counter (H1), so no behavior difference (logs/H16.log) |
| H17 | FORMAL | `g++ -std=c++11 -E` of a.cpp (one `__COUNTER__` use before) and b.cpp, both including `inline void f() { SCOPE_EXIT{...}; }`; link and run on g++/clang++ -O0/-O2 | a: `NEARGYE_SCOPE_GUARD_SCOPE_EXIT_1`, b: `NEARGYE_SCOPE_GUARD_SCOPE_EXIT_0`; run `calls=2` rc=0 on all 4 builds | Token sequences differ; no observable effect (logs/H17.log) |
| H18 | UNCLEAR | (b) `$c -std={c++11,c++17} -fno-exceptions -I$INC test/config_no_exceptions.cpp && nm \| grep -c __cxa_get_globals` | c++11: `refs=1`, c++17: `refs=0`; runs rc=0 | The dependency on `__cxa_get_globals` in C++11/14 `-fno-exceptions` builds exists. On libstdc++ it links and runs. The effect needs Emscripten (no-exception libc++abi) or a non-GNU, non-MSVC C++11 compiler. Neither is in the container or in review CI, so there is no probe. Fact for the fix idea: libstdc++ defines `__cpp_lib_uncaught_exceptions 201411L` in gnu++11 and `std::uncaught_exceptions` compiles in gnu++11 but not c++11 (`'uncaught_exceptions' is not a member of 'std'`) (logs/H18.log) |
| H19 | CONFIRMED | `$c -std=c++11 $FLAGS -I$INC h19.cpp` (`#define SCOPE_EXIT` before, h19r.cpp after the include) | g++: `scope_guard.hpp:365: error: "SCOPE_EXIT" redefined`; clang++: `error: 'SCOPE_EXIT' macro redefined [-Werror,-Wmacro-redefined]` | Same in both orders. Also evidence row 40. Design (unprefixed API macros) (logs/H19.log) |
| H20 | CONFIRMED | install, then `find_package(scope_guard <v> CONFIG REQUIRED)` for v = 0.1, 0.9, 0.9.5, 1.0, 0.9.6; then the installed ConfigVersion with `PACKAGE_VERSION "2.0.0"` and a request for 0.9 | 0.1/0.9/0.9.5: `found=1 version=0.9.5`; 1.0/0.9.6: `Could not find a configuration file ... compatible`; simulated 2.0.0 for request 0.9: `found=1 version=2.0.0` | `COMPATIBILITY AnyNewerVersion` (CMakeLists.txt:30). Packaging policy question (logs/H20.log) |
| H21 | CONFIRMED | (b) `cmake -S $S -B b21 -G Ninja -DCMAKE_CXX_FLAGS=-DH21_MARK`, `ctest -R install-consumer-default`; (a) PATH without ninja, `-DCMAKE_MAKE_PROGRAM=/tmp/scope_guard-step2b/tools/myninja` | (b) package-build and consumer-build `CMakeCache.txt`: `CMAKE_CXX_FLAGS:STRING=` (empty), 0 files contain `H21_MARK`; (a) configure and build pass, then `81% tests passed, 4 tests failed out of 21` with `"Ninja".  CMAKE_MAKE_PROGRAM is not set.` | All 4 integration tests fail; the rest of the suite passes (logs/H21.log) |
| H22 | CONFIRMED | `cmake -S $S -B b22 -G Ninja -DCMAKE_CXX_COMPILER={g++,clang++}`, `ninja -t commands example/scope_exit_example` | example compile line has no `-std=` (`/usr/bin/g++ -I.../include -Wall -Wextra -pedantic-errors -Werror ... -c .../scope_exit_example.cpp`), while `test/scope_guard-cpp11.t` has `-std=c++11` | g++-13's default is `__cplusplus 201703L`, so the examples build as gnu++17 and `CXX_EXTENSIONS OFF` has no effect (logs/H22.log) |
| H23 | CONFIRMED | `h23/run.sh`: n = 1000/10000 `SCOPE_EXIT` in one function, `-std=c++17`, `timeout 120` (date-based timing, because `/usr/bin/time` is not installed) | n=1000: g++ -O0 4.6 s, -O2 4.8 s; clang++ -O0 3.6 s, -O2 7.0 s. n=10000: g++ -O0 and -O2 and clang++ -O2 timeout (rc=124, >25x); clang++ -O0 45.9 s (12.9x), run rc=0. NO_THROW_ACTION: g++ -O0 10000 timeout, clang++ -O0 46.9 s | Meets the hypothesis' criterion (timeout). Baseline plain RAII lambda holder (step 2 `limits/gen.py`): g++ -O0 2.0 s → 48.7 s (24x), clang++ -O2 2.0 s → timeout. Most of the superlinear growth is the compilers' handling of many nested cleanups; the library adds about 2-2.5x and pushes g++ -O0 past 120 s. Unrealistic usage (also evidence O8) (logs/H23.log) |

**Counts:** CONFIRMED 16 (H3, H4, H5, H6, H7, H8, H9, H12, H13, H14, H15, H19, H20, H21, H22, H23), REJECTED 0, FORMAL 3 (H2, H16, H17), UNCLEAR 4 (H1, H10, H11, H18).
Several confirmed items are documented design or owner questions rather than defects: H4, H5, H14, H15, H19, H20, H23.

### Review CI probes (`.review/probes/`)

| file | checks | where | std | expect |
|---|---|---|---|---|
| `H1.cpp` | H1: `detail::uncaught_exceptions()` count 0/1/2 and SCOPE_FAIL/SUCCESS in C++11/14 | macos-appleclang clang-libcxx gcc-m32 gcc-latest | c++11 c++14 | run-ok |
| `H10.cpp` | H10: discarded `make_scope_exit` on MSVC | msvc-x64 msvc-x86 | c++14 c++17 c++20 | build-fail "C4834"; c++14: build-ok |
| `H11.cpp` | H11: the pre-C++17 pragma leaking into the action body | msvc-x64 msvc-x86 | c++14 c++17 | build-fail "C4189"; c++14: build-ok |
| `R1.cpp` | R1: nested in-flight counts up to 4, agreement with `std::uncaught_exception()`, fail/success at each depth, moved scope_fail | macos-appleclang clang-libcxx gcc-latest gcc-m32 | c++11 c++14 | run-ok |
| `R2.cpp` (+ `R2_last.cpp`) | R2: `<cxxabi.h>` before and after scope_guard.hpp in two TUs | macos-appleclang clang-libcxx gcc-latest gcc-m32 | c++11 c++14 c++17 | run-ok |
| `R6.cpp` | R6: all macros at /W4 /WX on MSVC incl. pre-C++17 pragma/_Check_return_ branches; fail/success on x86 and x64 | msvc-x64 msvc-x86 | c++14 c++17 c++20 | run-ok |
| `R7.cpp` | R7: same body under clang-cl (`#error` unless `__clang__` and `_MSC_VER`) | clangcl | c++14 c++17 c++20 | run-ok |

No probes for R3, R4 and R5, which the container already checked (R3: evidence row 28 and test pins; R4: H4; R5: evidence row 25 and H3).
Runner check (`run.py` copied unchanged to `/tmp/scope_guard-step2b/review/probes/` with the probes and `include/`):
`python3 run.py --job {gcc-latest,clang-libcxx,macos-appleclang} --style gcc --cxx {g++,clang++,clang++}` → H1, R1, R2 `held` on every std (libstdc++ stand-ins for the real jobs; logs/runner-*.log).
`--job gcc-m32 --flags=-m32` → build-fail `bits/c++config.h: No such file or directory` (no multilib; CI decides).
MSVC probes cannot run here: R6's body builds and runs `ok` with g++/clang++ c++11..c++20 under `$FLAGS`, R7's body is identical to R6's, and R7 stops at `#error R7 must be built with clang-cl` outside clang-cl. H10 and H11 parse, and give the expected GCC/Clang analogues (logs/msvc-probes-local-sanity.log).

## Empirical checks (step 2)

| # | check | configuration | result | command |
|---|---|---|---|---|
| 1 | T1 baseline full ctest | g++-13, Debug, Ninja (test.cpp c++11/14/17/20 + config/no-exceptions/compile-fail/integration) | PASS, 21/21 | `cmake -S $S -B b-gcc-debug -G Ninja -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_BUILD_TYPE=Debug && cmake --build b-gcc-debug -j4 && ctest --test-dir b-gcc-debug -j4 --output-on-failure` → `100% tests passed, 0 tests failed out of 21` (logs/T1-gcc-debug-ctest.log) |
| 2 | T1 baseline full ctest | clang++-18, Debug | PASS, 21/21 | same with `clang++-18` (logs/T1-clang-debug-ctest.log) |
| 3 | T1 test.cpp, every standard | g++-13: c++11, c++14, c++17, c++20, c++23, gnu++11, gnu++14 | PASS, 20/20 test cases each | `g++-13 -std=$s -Wall -Wextra -pedantic-errors -Werror -DDOCTEST_CONFIG_USE_STD_HEADERS -I$S/include -I$S/test/3rdparty/doctest $S/test/test.cpp` + run (logs/gcc-test-*.{build,run}.log) |
| 4 | T1 test.cpp, every standard | clang++-18: c++11, c++14, c++17, c++20, c++23, c++2c, gnu++11 | PASS, 20/20 each | same with clang++-18 (logs/clang-test-*.log) |
| 5 | T1 ASan+UBSan, full ctest | g++-13 Debug, `-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer` (all test targets c++11..c++20, config tests, -fno-exceptions tests) | PASS, 21/21, no sanitizer report | `cmake ... -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer"`; flags confirmed in `ninja -t commands` (32 commands) and `__asan_init` in `scope_guard-cpp11.t` (logs/T1-gcc-asan-ctest.log). Integration tests (4 of 21) build their consumer without CMAKE_CXX_FLAGS, so they are not sanitized |
| 6 | T1 ASan+UBSan, test.cpp extra standards | g++-13 c++14, c++23, gnu++11 | PASS, 20/20 each | logs/T1-gcc-asan-test-*.log |
| 7 | T1 ASan+LSan on examples (P12-like) | g++-13 ASan build, 3 examples, `ASAN_OPTIONS=detect_leaks=1` | PASS, rc=0 each, no report | logs/T1-gcc-asan-scope_*_example.log |
| 8 | T1 Clang UBSan trap mode, full ctest | clang++-18 Debug, `-fsanitize=undefined -fsanitize-trap=undefined` (ASan runtime missing, see "Not done") | PASS, 21/21; 4 `ud2` traps present in `scope_guard-cpp11.t` | logs/T1-clang-ubtrap-ctest.log; same on test.cpp c++23/c++2c: PASS (logs/T1-clang-ubtrap-test-*.log) |
| 9 | Memcheck instead of Clang ASan | valgrind on the clang Debug build: cpp11.t, cpp20.t, suppress-throw-action.t, throwing-move-action.t, cxxabi-first.t | PASS, rc=0, no errors (`--error-exitcode=99 --leak-check=full`) | logs/VG-summary.log |
| 10 | T2 Release (NDEBUG, -O3), full ctest | g++-13 and clang++-18 | PASS, 21/21 each | `-DCMAKE_BUILD_TYPE=Release` (`-O3 -DNDEBUG` confirmed in `ninja -t commands`), logs/T2-*-release-ctest.log |
| 11 | T2 -fno-rtti, full ctest | g++-13 and clang++-18 Debug, `CMAKE_CXX_FLAGS=-fno-rtti` | PASS, 21/21 each | logs/T2-*-nortti-ctest.log |
| 12 | T2 config tests on C++14+ (std::uncaught_exceptions path) and gnu++11 | `config_test.cpp` × {no-throw-action, suppress-throw-action, throwing-move, cxxabi-first, cxxabi-last} × g++-13 {c++14,17,20,23,gnu++11} + clang++-18 {c++14,17,20,23,2c,gnu++11} | PASS, 55/55 builds and runs | `./cfg.sh` (logs/T2-config-tests-all-std.log) |
| 13 | Unmask suppressions | `/wd4702` (test, example CMake) and `__pragma(warning(suppress : 4100 4101 4189))` (hpp:330) | NOT APPLICABLE in container: both are MSVC-only. Closest GCC/Clang analogue: Clang `-Weverything` (contains `-Wunreachable-code-aggressive`, `-Wunused-*`) gives 0 project-header warnings of those kinds (row 15). MSVC → review CI | profile §4 |
| 14 | Strict GCC warnings | g++-13 c++11/c++17/c++23, `-Wall -Wextra -pedantic -Wconversion -Wsign-conversion -Wshadow -Wold-style-cast -Wuseless-cast -Wcast-qual -Wzero-as-null-pointer-constant -Wextra-semi`, doctest via `-isystem`, on test.cpp, 3 examples, config_test.cpp × 4 scenarios; plus a TU that uses every macro (`hdr_only.cpp`) with `-Weffc++` added | PASS, 0 warnings | logs/W-gcc-c++*.log, logs/W-gcc-header-only.log |
| 15 | Strict Clang warnings | clang++-18 c++11/c++17/c++2c, `-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-pre-c++14-compat -Wno-pre-c++17-compat -Wno-c++20-compat -Wno-unsafe-buffer-usage`, same sources | 7 warnings, all in test sources, none in the header: test.cpp `-Wmissing-prototypes` ×3, `-Wmissing-variable-declarations` ×2, `-Wmissing-noreturn` ×1 (test.cpp:76); config_test.cpp:23 `-Wmissing-variable-declarations` ×1 | logs/W-clang-c++*.log |
| 16 | Clang -Weverything on header only (without -Wno-padded / unsafe-buffer) | `hdr_only.cpp`, c++11..c++2c | header: `-Wpadded` (hpp:211, `policy_` before `action_`) for each instantiation; `-Wunsafe-buffer-usage` hpp:124 (pre-C++17 only). See Observation O6 | logs/W-clang-header-only.log |
| 17 | Newest compilers, -pedantic-errors, minimum standard | g++-13 and clang++-18, `-std=c++11 -pedantic-errors -Werror` (CMake cpp11.t, config, no-exceptions, examples) | PASS (rows 1–2); g++-13 does not accept `-std=c++2c`/`c++26` (`unrecognized command-line option`), so c++23 is GCC's newest here | `g++-13 -std=c++26 -fsyntax-only empty.cpp` |
| 18 | Attributes: NODISCARD expansion | g++-13, clang++-18 × c++11/14/17/20 | As documented: `__attribute__((__warn_unused_result__))` for <C++17, `[[nodiscard]]` for ≥C++17 on both compilers | `-E -P attr_probe.cpp` (logs/attr-expansion.log) |
| 19 | Attributes: NODISCARD effect | discard `make_scope_exit/fail/success(...)` | PASS: 3/3 `-Wunused-result`/nodiscard warnings on every compiler × standard. `(void)` cast does not silence it on g++-13 c++11/c++14 (O2) | logs/attr-behaviour.log, logs/attr-gcc-voidcast.log |
| 20 | Attributes: MAYBE_UNUSED expansion and effect | same matrix | Expands to `__attribute__((__unused__))` (<C++17) / `[[maybe_unused]]` (≥C++17), never empty. With the macro forced empty there are still 0 unused-variable warnings on GCC/Clang (O3) | logs/attr-expansion.log, logs/attr-behaviour.log |
| 21 | `__COUNTER__` detection | `#if defined(__COUNTER__)` | true on g++-13 and clang++-18 → `NEARGYE_SCOPE_GUARD_COUNTER` = `__COUNTER__`; `-E` shows unique names `..._SCOPE_EXIT_0`, `FLAG_1`, `OBJECT_2` | logs/attr-expansion.log |
| 22 | uncaught_exceptions branch by standard | g++-13/clang++-18 × c++11/14/17/20 | c++11/14 → `__cxa_get_globals` path, c++17/20 → `std::uncaught_exceptions`, as documented | logs/attr-expansion.log |
| 23 | `__cxa_get_globals` forward declaration branch | same | Taken on clang++-18 (c++11/14: 1 declaration in `-E` output), not on g++-13: Clang defines `__GNUC__ 4`, `__GNUC_MINOR__ 2` → `(4*100+2) < 407`. Compiles with libstdc++'s `<cxxabi.h>` before and after (rows 1–2, 12). O1; hypothesis H2 covers it | `clang++-18 -std=c++11 -dM -E -x c++ /dev/null` |
| 24 | Exceptions-enabled detection macros | `-fno-exceptions` vs default | `__cpp_exceptions` and `__EXCEPTIONS` both defined by default on both compilers, both undefined with `-fno-exceptions` (rows 25) | `-dM -E` |
| 25 | P6 build modes matrix | `config_no_exceptions.cpp` × {g++-13 c++11..c++23, clang++-18 c++11..c++2c} × 7 configs {default, MAY_THROW, NO_THROW, SUPPRESS, SUPPRESS+CATCH_HANDLER=`std::abort();`, NO_THROW_CONSTRUCTIBLE, NO_THROW_CONSTRUCTIBLE+NO_THROW} × 4 modes {`-fno-exceptions`, `-fno-exceptions -fno-rtti`, `-fno-exceptions -DNDEBUG -O2`, `-fno-rtti -DNDEBUG -O2`} | PASS, 308/308 build+run | `./p6.sh` (logs/P6-matrix.log) |
| 26 | Comparison with `std::experimental::scope_*` (libstdc++ 13 `<experimental/scope>`) | g++-13 and clang++-18, c++20 and c++23; `static_assert` on copy/move ctor/assign, default ctor, nothrow move, nothrow dtor | Equal: copy-ctor/assign deleted, move-assign deleted, default ctor absent, move ctor present, nothrow move follows the action, nothrow dtor for noexcept action. Different by design: dtor of guard with throwing action is `noexcept(false)` here (default MAY_THROW, documented), `noexcept` for std scope_exit/scope_fail. Runtime exit/fail/success counts on normal and throwing paths equal | `cmp/std_compare.cpp` (logs/cmp-std-experimental-scope.log) |
| 27 | Comparison: dismiss/release then move | same | project: dismissed guard stays dismissed after move (0 runs). libstdc++ 13 `std::experimental::scope_exit`: runs after `release()` + move (1 run) — standard-library defect, not project (O7) | `cmp/release_move.cpp` (logs/cmp-release-move.log) |
| 28 | Limits: nested in-flight exceptions | depth 500 and 2000 of destructors throwing during unwinding; checks `detail::uncaught_exceptions() == depth` (and `== std::uncaught_exceptions()` in gnu++11/c++17+), SCOPE_FAIL fires at every level, SCOPE_SUCCESS never | PASS on g++-13/clang++-18 × c++11, gnu++11, c++17, c++20 (failures=0, fail_fired=2000, success_fired=0); also under g++ ASan+UBSan c++11 | `limits/nested_unwind.cpp` (logs/L-limits.log) |
| 29 | Limits: many guards in one function | N = 1000 and 3000 `SCOPE_EXIT` in one function vs a plain RAII lambda holder, -O0/-O1, c++11 | PASS functionally (all run, count = N). Compile time grows faster than for plain RAII (O8). 15000 mixed guards with g++-13 -O1: compile exceeded the 120 s probe timeout (rc=124) | `limits/gen.py` (logs/L-many-guards-scaling.log, logs/L-limits.log) |
| 30 | P1 install tree | `cmake --install` into `p1/prefix` | Installs `include/scope_guard.hpp`, `lib/cmake/scope_guard/scope_guardConfig.cmake`, `scope_guardConfigVersion.cmake`, `share/scope_guard/LICENSE` | logs/P1-install.log |
| 31 | P1 find_package consumer | g++-13 and clang++-18; `find_package(scope_guard <none>/0.9/0.9.5/0.1 CONFIG REQUIRED)` | PASS: configure, build, run `n=1 version=0.9.5` (consumer at compiler default `__cplusplus=201703`) | logs/P1-summary.log |
| 32 | P1 find_package version rejection | same; requested 1.0 and 0.9.6 | configure fails (rc=1) on both compilers, as expected for `AnyNewerVersion` | logs/P1-g++-13-1.0.log etc. |
| 33 | P1 add_subdirectory consumer | g++-13 and clang++-18, `CMAKE_CXX_STANDARD=11`, `CMAKE_CXX_EXTENSIONS=OFF` | PASS: `SCOPE_GUARD_OPT_BUILD_EXAMPLES/TESTS/INSTALL=OFF`, no test/example targets, run `n=1`, `__cplusplus=201103` | logs/P1-sub-*.log |
| 34 | P1 integration tests in ctest | install-consumer-default, install-consumer-custom-includedir, subproject-consumer, subproject-defaults × {gcc, clang} × {Debug, Release, -fno-rtti} | PASS (part of rows 1, 2, 10, 11) | ctest logs |
| 35 | P2 compile-fail tests reach the right diagnostic | 4 scenarios × g++-13/clang++-18, c++11, project flags, `-fsyntax-only` | PASS: MULTIPLE_THROW_POLICIES → `#error Only one of ...`; REJECT_THROWING_ACTION → `static assertion failed: scope_guard requires noexcept invocable action.` (only error); REJECT_THROWING_MOVE → `... nothrow move-constructible action.` (only error); REJECT_LVALUE_ACTION → `static assertion failed: make_scope_exit requires an rvalue action...` plus 2 follow-up `use of deleted function scope_guard(A&)` errors (O5) | logs/P2-<compiler>-<scenario>.log |
| 36 | P2 ctest matches the stated text | `ctest -R 'compile-fail\|throwing-scope-fail' -V` on gcc and clang Debug trees | PASS: 5× `Observed expected failure: <text>` on each compiler | logs/P2-*-compile-fail-verbose.log |
| 37 | P2 positive controls | fixed variants (single policy; `noexcept` lambda with NO_THROW_ACTION; nothrow-move functor with NO_THROW_CONSTRUCTIBLE; `std::move(action)`) × g++-13/clang++-18 × c++11/c++17 | PASS: 16/16 compile with project flags | `p2/pc_*.cpp` (logs/P2-poscontrol-summary.log) |
| 38 | P2 runtime-terminate test | `throwing-scope-fail-action.t` | PASS, output `scope_fail action terminated` checked by `expect_failure.cmake` | logs/P2-*-compile-fail-verbose.log |
| 39 | P7 (partial) windows.h-style macros before the header | `min`, `max`, `near`, `far`, `ERROR`, `DELETE`, `interface`, `small`, `IN`, `OUT`, `OPTIONAL`, `CONST`, `VOID`, `FAILED(hr)` defined first; g++-13/clang++-18 c++11/17/20/23 | PASS 8/8 build+run. A `byte` macro breaks libstdc++'s `<cstddef>` itself in C++17 (`unnamed scoped enum is not allowed`), so it was dropped as unrealistic (windows.h has no `byte` macro) | `p7.cpp` (logs/P7-summary.log, logs/P7-byte-stdlib-only.log) |
| 40 | P7 macro-name clash | `#define SCOPE_EXIT ...`/`#define DEFER ...` before the header | `"SCOPE_EXIT" redefined` / `'SCOPE_EXIT' macro redefined [-Wmacro-redefined]` warnings on both compilers (error with -Werror). Known design (unprefixed public macros), hypothesis H19 | logs/P7-macro-clash.log |
| 41 | P9 (~) version consistency | CMakeLists.txt `project(VERSION "0.9.5")`, header `SCOPE_GUARD_VERSION_*` 0/9/5, banner `version 0.9.5`, installed ConfigVersion → `0.9.5` | consistent | `grep` (above) and row 31 |

### Observations

- **O1 (hpp:65, row 23).** The pre-C++17 `__cxa_get_globals` forward declaration branch is taken on every Clang, because Clang
  reports `__GNUC__ 4`, `__GNUC_MINOR__ 2` and the `< 407` clause is meant for old GCC. With libstdc++ the redeclaration is
  compatible (cxxabi-first/last pass on c++11..c++2c). With libc++abi it can only be checked in CI. Hypothesis H2 covers this.
- **O2 (hpp:96-98, row 19).** In C++11/14 on GCC, `NODISCARD` is `__attribute__((__warn_unused_result__))`, which an explicit `(void)` cast does not silence:
  `g++-13 -std=c++11 -Wall`: `nodiscard_probe.cpp:7:45: warning: ignoring return value of '...make_scope_exit...', declared with attribute 'warn_unused_result' [-Wunused-result]`
  for `(void)scope_guard::make_scope_exit([]() {});`. Clang and C++17 `[[nodiscard]]` accept the cast. This is GCC attribute semantics. Discarding a guard is almost always a mistake anyway.
- **O3 (row 20).** On GCC/Clang the MAYBE_UNUSED attribute on `SCOPE_EXIT` objects is not needed: with the macro forced empty, `-Wall -Wextra -Wunused`
  gives 0 warnings because the guard type has a non-trivial destructor. Harmless. It may still matter for MSVC (C4189), which is CI only.
- **O4 (row 15).** Test sources (not the header) trigger Clang `-Weverything` diagnostics: `-Wmissing-prototypes` (test.cpp:113, 117, 125),
  `-Wmissing-variable-declarations` (test.cpp:110, 111; config_test.cpp:23), `-Wmissing-noreturn` (test.cpp:76). The project builds with `-Wall -Wextra` only,
  so these are style notes for step 3, not failures.
- **O5 (row 35).** For an lvalue action GCC and Clang print the intended `static_assert` text and then 2 more "use of deleted function/constructor
  `scope_guard(A&)`" errors, from the noexcept-spec and the body. The ctest regex still matches. Hypotheses H6/H7 cover the diagnostic quality.
- **O6 (row 16).** Under `-Weverything`, the header gives `-Wpadded` (policy before action; for example, 7 bytes of padding for a reference-capturing lambda with `on_exit_policy`)
  and, in C++11/14 only, `-Wunsafe-buffer-usage` at hpp:124 (the `__cxa_eh_globals` offset arithmetic). Both are expected noise for this design.
- **O7 (row 27).** libstdc++ 13 `std::experimental::scope_exit` runs its function after `release()` followed by a move.
  The move constructor at `/usr/include/c++/13/experimental/scope:84-94` does not copy `_M_execute_on_destruction`. The project's `dismiss()` + move correctly stays dismissed.
  Anyone who uses libstdc++ 13's experimental header as a reference for differential tests must not count this as a project deviation.
- **O8 (row 29).** Compile time for very many guards in one function. g++-13 -O0 c++11: 1000 `SCOPE_EXIT` 4.8 s vs plain RAII holder 2.2 s,
  3000: 26.1 s vs 6.7 s. clang++-18: 1000 → 3.5 s vs 1.4 s, 3000 → 11.3 s vs 3.9 s. Object at N=3000: about 3.3–3.5 MB vs 1.6–2.0 MB.
  `SCOPE_GUARD_NO_THROW_ACTION` changes little (22.4 s / 9.9 s), so the cost is not the noexcept(false) destructor. Thousands of guards in one function are unrealistic.
  This is recorded as a fact only (P4 is not selected in the profile).
- **O9 (row 5).** The 4 CMake integration tests do not pass `CMAKE_CXX_FLAGS` to their nested builds, so they ran unsanitized inside the sanitizer build.
  Hypothesis H21 covers this. It does not affect the header checks.

### Not done / could not

- **Clang ASan/LSan/MSan, libFuzzer:** `clang++-18 -fsanitize=address` fails with `cannot find .../libclang_rt.asan_static-x86_64.a` (no compiler-rt).
  Replaced by Clang UBSan trap mode (row 8) and Valgrind memcheck (row 9). Not installed, because installing packages needs the owner's permission.
- **-m32, libc++/libc++abi, GCC 14/15, Clang 19+:** not in the container (profile §6). The pre-C++17 `__cxa_eh_globals` offset (R1/H1) and the libc++abi `<cxxabi.h>`
  redeclaration (R2/H2) on non-libstdc++ ABIs → review CI (`gcc-m32`, `clang-libcxx`, `macos-appleclang`, `gcc-latest`).
- **GCC `-std=c++2c`:** g++-13 rejects it. C++2c was checked on clang++-18 only.
- **MSVC / ClangCL / AppleClang:** suppressions `/wd4702` and `__pragma(warning(suppress ...))` (row 13), `_Check_return_` NODISCARD, Win32 x86 → CI only (R6, R7).
- **Real windows.h:** emulated macro set only (row 39). The real header → CI (`msvc-x64`, `msvc-x86`).
- **P3, P4, P5, P10, P11:** not selected in profile §10. P4-style numbers exist only as an observation (O8).
- **T3:** not run because T1/T2 found no project failure that needs a version or standard boundary.
- **Step 9 (hypotheses.md):** present when T1 finished. All its container probes use g++-13/clang++-18 with standard or probe-local flags
  (for example H3's `-fexceptions -fno-cxx-exceptions`), so they need no extra T2 configuration. Hypotheses were not verified here (step 2b does that).
- **Existing CI after push:** only the orchestrator pushes, so it was not observed in this step.
