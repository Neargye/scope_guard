# scope_guard — review report (2026-10-08)

Base `a94902057688bd86b9b76e911104be8be0a46120` (`v0.9.5`, `master`). Commits were made in an isolated git worktree
and then moved to the review branch `claude/determined-curie-siq6s6`. Toolchains: `g++-13` 13.3.0, `clang++-18`
18.1.3 (libstdc++ 13), cmake 3.28.3, ninja 1.11.1, x86-64.

```
$ git log --oneline a949020..HEAD   (review branch)
cdfae9d [proposal] test: expect the rvalue diagnostic on msvc
4f6d20b review: ci probes 2026-10-08
e37e8f4 review: report 2026-10-08
a8deabd [proposal] fix lvalue action diagnostics
d91b723 fix nodiscard macro override
1c3fb6c fix function action diagnostic
```

Selection. In `hypotheses.md` every item has impact "low" or "low-med", so the step 3 rules allow only fixes
that are trivial and obviously safe: H11 and H12 (regular fixes). H1 changes what compiles (an API mark in
`hypotheses.md`), so it is a separate [proposal] commit. H5 is a candidate in step 2b, but its fix is not
trivially safe, so it is deferred (D5). Everything else went to the decisions (D2–D14).

## Fixes

### fix function action diagnostic (H11)

- **Problem:** if `make_scope_exit/fail/success` gets a function (`make_scope_exit(cleanup)`), the static_assert
  says "use std::move or pass a temporary". A function expression is always an lvalue, so `std::move` does not
  help. The function's address has to be passed instead.
- **Evidence:** `g++-13`/`clang++-18`, `-std=c++11`/`c++17`, `failure_test.cpp` with a function argument:
  `static assertion failed: make_scope_exit requires an rvalue action; use std::move or pass a temporary.`;
  `make_scope_exit(&cleanup)` compiles (step 2b `h11_addr.cpp`).
- **Change:** the three messages now end with "use std::move or pass a temporary; for a function, pass its address."
  The pinned substring "make_scope_exit requires an rvalue action" stays the same.
- **Test:** new compile-fail test `scope_guard-compile-fail-function-action.t` (`failure_test.cpp`,
  `SCOPE_GUARD_TEST_REJECT_FUNCTION_ACTION`), which expects "for a function, pass its address".
  With the old header (current tests + header from before the fix) it fails on both compilers:
  `Compilation of scope_guard-compile-fail-function-action.t failed without expected diagnostic
  'for a function, pass its address'`. With the new header it passes: `Observed expected failure: for a function, pass its address`.
  The commit alone (tree from `git archive`) passes 22/22 with g++-13.
- **Limitations:** this commit changes only the message. MSVC shows the static_assert text too (C2338), but the
  MSVC result of the new test comes only from the existing Windows CI after the push.

### fix nodiscard macro override (H12)

- **Problem:** a user-defined `NEARGYE_SCOPE_GUARD_NODISCARD` was used and then `#undef`'d by the header, so it no
  longer existed after the include. The other overridable macros (`_MAYBE_UNUSED`, `_STR_CONCAT`, `_COUNTER`) are kept.
- **Evidence:** step 2b H12, on both compilers with c++11/17: `#error "user NEARGYE_SCOPE_GUARD_NODISCARD was undefined by the header"` (4/4).
- **Change:** when the header defines the macro itself, it also defines a private flag
  `NEARGYE_SCOPE_GUARD_UNDEF_NODISCARD`. At the end of the namespace it `#undef`s both only when that flag is set.
- **Test:** new config test `scope_guard-user-nodiscard.t` (`config_test.cpp`, `SCOPE_GUARD_TEST_USER_NODISCARD`).
  It defines the macro empty, discards a factory result under `-Werror`, and checks that the macro is still defined.
  With the old header it fails: `REQUIRE( user_nodiscard_defined ) is NOT correct! values: REQUIRE( false )`
  (g++-13 and clang++-18). With the new header it passes. Default path (both compilers × c++11/14/17/20): the macro
  and the flag are undefined after the include, and the discarded result still warns (8/8).
- **Limitations:** none known.

## Proposals

### [proposal] fix lvalue action diagnostics (H1, O1)

- **What changes:** the noexcept-specification of the three factories and the three `operator<<` changes from
  `noexcept(noexcept(scope_exit<F>{action}))` to
  `noexcept(std::is_nothrow_constructible<scope_exit<F>, F>::value && std::is_nothrow_destructible<scope_exit<F>>::value)`.
  The traits evaluate to false for a deleted constructor instead of failing to compile.
- **Why:** with an lvalue argument, the old specification picks the deleted `scope_guard(A&)` outside the immediate context, so:
  1. A direct call reports "use of deleted function … scope_guard(A&)" inside the header *before* the
     intended "make_scope_exit requires an rvalue action".
  2. Just naming the call in `decltype`/`noexcept`/a detection idiom/`requires` is a hard error.

  After the change:
  - A direct call gives 2 errors with the static_assert first (before: 3 errors, the deleted constructor first;
    both compilers, c++11/17).
  - `decltype` on an lvalue or const rvalue call compiles (both compilers × c++11/17/20).
    Before, the result was "use of deleted function" / "call to deleted constructor".
- **Not changed:** the exception specification for valid calls. Comparing the `noexcept` expression
  value and the C++17 function type for 5 action types × {default, NO_THROW, SUPPRESS, NO_THROW_CONSTRUCTIBLE,
  SUPPRESS + `-fno-exceptions`} × c++11/14/17/20 × 2 compilers: `configurations=40 differing=0`.
- **Test:** `test.cpp` gets static_asserts that name all three factories with an lvalue in `decltype` and check the
  declared return type. With the old header `scope_guard-cpp11.t`…`cpp20.t` do not compile (g++-13: `use of deleted function
  'scope_guard::detail::scope_guard<F, P>::scope_guard(A&) [with F = F&; …]'`, clang++-18: `call to deleted
  constructor of 'scope_exit<F &>'`). With the new header they pass.
- **Risks:**
  - Programs that were ill-formed now compile: a detection trait on `make_scope_*` reports *true* for an lvalue or
    const rvalue, and the call itself is still rejected. The test pins this.
  - If the owner prefers SFINAE rejection of lvalues instead, the pinned diagnostic "make_scope_exit requires an
    rvalue action" would be lost (Q1).
  - A throwing action under `SCOPE_GUARD_NO_THROW_ACTION` is still a hard error in detection contexts (class
    static_assert, by design).
  - A const rvalue still ends in "use of deleted function … scope_guard(const A&)" without a friendly message (D13).
  - MSVC/ClangCL behaviour of the new static_asserts comes only from CI.
- **Follow-up `cdfae9d` (found by CI):** with the new specification MSVC no longer stops at the deleted
  constructor (C2280) and reports the intended static_assert (C2338 "make_scope_exit requires an rvalue action").
  `compile-fail-lvalue-action.t` pinned "error C2280" for MSVC and failed on all four MSVC jobs of the windows
  workflow and on review CI msvc-x64/msvc-x86 (run 37834802179). `cdfae9d` makes the test expect the static_assert
  text on every compiler; all workflows are green on it. Reverting the proposal means reverting both commits.

## Found but not fixed

| Item | Reason | Decision |
|---|---|---|
| H2 (FORMAL) factory `noexcept` also counts the guard destructor | FORMAL; widening `noexcept` changes the C++17 function type (API) | D2, Q2 |
| H3 throwing-move semantics differ from LFTS v3 | design, pinned by `config_test.cpp:118-177`; API | D3, Q3 |
| H4 (UNCLEAR) own `__cxa_get_globals` declaration emitted on every Clang | no reliable fix: the breaking toolchain (Clang + libcxxrt) is not available here or in review CI | D4, Q9 |
| H5 `<cxxabi.h>` injects `namespace abi` before C++17 | low impact; the fix is not trivially safe (ABI-library-specific declarations, only libstdc++ verifiable; removes a transitive include) | D5, Q5 |
| H6 (UNCLEAR) branch not keyed on `__cpp_lib_uncaught_exceptions` | no toolchain can confirm | D6, Q10 |
| H7 + O4 ODR: policy / `-fno-exceptions` / standard mixing across TUs | design; README already requires consistent settings | D7, Q6 |
| H8 `SCOPE_GUARD_CATCH_HANDLER` name lookup | documentation gap, README not contradicted | D8, Q7 |
| H9 `~scope_fail` not unconditionally `noexcept` | design, API | D9, Q4 |
| H10 `noexcept` function pointers before C++17 | language rule | D10, Q7 |
| H13 `__LINE__` fallback collisions | low impact, fallback only | D11, Q7 |
| H16 `AnyNewerVersion` package compatibility | packaging policy, API | D12, Q8 |
| H1 residual (detection reports true; const rvalue message) | see proposal risks | D13, Q1 |
| H14, H15 (REJECTED), O5, O11 | no defect | D14 |
| D15 | Review CI closed R1, R2 and R3 (D1 confirmed in practice): the pre-C++17 `uncaught_exceptions()` offset is correct on libc++abi (macOS arm64, Linux Clang 20 + libc++ with ASan/UBSan) and on 32-bit libsupc++; the own `__cxa_get_globals` declaration coexists with `<cxxabi.h>` in both include orders; the MSVC/ClangCL attribute branches work, `[[nodiscard]]` fires from C++17, `_Check_return_` is silent in C++14 and the MSVC `MAYBE_UNUSED` fallback suppresses nothing (it is not load-bearing). No code change. | All 51 probe predictions held on 6 jobs (run 37834802179); project suite green on all jobs at `cdfae9d` (run 37837125058). | Review CI section; `include/scope_guard.hpp:62-76,95-120,122-130,312-335` |

## Questions for the owner

- **Q1:** Do you accept the [proposal] "fix lvalue action diagnostics"? Or should lvalue/const-rvalue actions be rejected by
  SFINAE (`enable_if` on `is_rvalue_reference && !is_const`)? That makes detection traits report false, but the
  diagnostic becomes "no matching function" instead of "make_scope_exit requires an rvalue action".
- **Q2 (H2):** Should the factories be `noexcept` whenever the action's move is, without the guard
  destructor term (`noexcept(std::is_nothrow_constructible<std::decay_t<F>, F>::value)`)?
- **Q3 (H3):** Do you want the LFTS v3 semantics: call the action if moving it into the guard throws, and copy
  instead of using a throwing move in the guard move constructor? This contradicts the current tests.
- **Q4 (H9):** Should `~scope_fail` be unconditionally `noexcept`, as in LFTS?
- **Q5 (H5):** In C++11/14, should `<cxxabi.h>` be replaced by the header's own `__cxa_get_globals` declaration?
  A safe first step is libstdc++ only (`__GLIBCXX__`: `noexcept __attribute__((__const__))`), with other ABI
  libraries unchanged. The alternative is a README note.
- **Q6 (H7/O4):** Should a policy mismatch between TUs be detected (an inline namespace per policy, which is an ABI
  change)? Or should the README also mention mixing language standards and `-fno-exceptions`?
- **Q7 (H8/H10/H13):** Do you want README notes on these? H8: names used by the catch handler must be declared
  before the include. H10: wrap `noexcept` function pointers in a `noexcept` lambda before C++17. H13: the `__LINE__` fallback.
- **Q8 (H16):** `SameMajorVersion` or `SameMinorVersion` instead of `AnyNewerVersion` for 0.x?
- **Q9 (H4):** Should `!defined(__clang__)` be added to the GCC < 4.7 clause? This is only safe to verify with Clang + libcxxrt.
- **Q10 (H6):** Should the `std::uncaught_exceptions()` branch be selected by `__cpp_lib_uncaught_exceptions`?

## Verification on HEAD (after all step 3 commits)

| Configuration | Result |
|---|---|
| g++-13 Debug, Ninja, full CMake suite (incl. 4 integration, 5 compile-fail, examples built) | 23/23 pass |
| clang++-18 Debug, Ninja | 23/23 pass |
| g++-13 Release, Unix Makefiles | 23/23 pass |
| clang++-18 Release, Unix Makefiles | 23/23 pass |
| g++-13 ASan+UBSan (`-fno-sanitize-recover=all`, `detect_leaks=1`) + 3 examples | 23/23 pass, examples rc=0 |
| clang++-18 UBSan trap mode + 3 examples | 23/23 pass, examples rc=0 |
| g++-13 / clang++-18 `-fno-rtti -DNDEBUG` | 23/23 pass each |
| Standards sweep: test.cpp, 6 config scenarios (incl. new USER_NODISCARD, both cxxabi orders), 3 `-fno-exceptions` policies, 5 compile-fail scenarios (diagnostic text checked); g++-13 c++11/14/17/20/23, clang++-18 c++11/14/17/20/23/2c, `-Wall -Wextra -pedantic-errors -Werror` | 165/165 pass |
| noexcept preservation of the [proposal] | 40/40 identical |

Not verified in the container (all but the last two items were then covered by CI, see Review CI):
- Clang ASan/LSan: the runtime is missing (`libclang_rt.asan_static` not found).
- `-m32`: no multilib.
- libc++/libc++abi: no headers.
- MSVC, ClangCL, AppleClang: no toolchain.
- GCC ≠ 13, Clang ≠ 18: only one version of each in the container.
- GCC `-std=c++2c`: not supported by GCC 13 (still not verified: no CI job has a GCC with c++2c).
- H4 (Clang + libcxxrt) and H6 (non-GNU-emulating compiler, old macOS deployment target): no toolchain in the container or in CI; still UNCLEAR.

What covers them: the existing CI after the push (GCC 12/14, Clang 16/17, AppleClang/libc++abi, MSVC x64, ClangCL) runs
the new tests (function-action compile-fail, user-nodiscard, the `test.cpp` static_asserts). Review CI covers
R1/R2/R3 (32-bit, libc++abi, MSVC x86).

CI extension: none needed for the fixed problems. They show up on every GCC/Clang/MSVC configuration, and the new
tests are wired into the existing CMake suite that all three workflows run. Existing CI has no sanitizer job; a
permanent `-fsanitize=address,undefined` GCC Debug job on ubuntu would be the minimal general extension.

## Review CI

Temporary workflow `.github/workflows/review.yml` + `review/probes/` (commit `4f6d20b`), runner from the runbook
unchanged. Runner packages: `llvm.sh 20` from apt.llvm.org, `libc++-20-dev`, `libc++abi-20-dev`,
`libclang-rt-20-dev` (clang-libcxx); `g++-multilib` from the Ubuntu archive (gcc-m32).

Runs:
- `4f6d20b`: review https://github.com/Neargye/scope_guard/actions/runs/37834802179 — probes 51/51 predictions held (5 + 5 + 5 + 12 + 12 + 12 rows);
  project tests red on msvc-x64/msvc-x86 (and on the existing windows workflow, MSVC jobs) because of the [proposal]
  diagnostic change, see the proposal section.
- `cdfae9d`: review https://github.com/Neargye/scope_guard/actions/runs/37837125058, windows 37837124878,
  ubuntu 37837124991, macos 37837124958 — all green.

| job | runner / toolchain | project build + tests (`cdfae9d`) | probes | verdict |
|---|---|---|---|---|
| macos-appleclang | macos-15, AppleClang, libc++abi, arm64 | pass | R1 c++11/14/17 held (643 checks, 0 errors, custom path on c++11/14), R2 c++11/14 held (own declaration: yes, `_LIBCPPABI_VERSION` defined) | R1, R2 closed on libc++abi/arm64 |
| clang-libcxx | ubuntu-24.04, Clang 20 + libc++/libc++abi, ASan+UBSan | pass (first project run with sanitizers on Clang) | R1 held (643/0), R2 held (own declaration: yes) | R1, R2 closed on Linux libc++abi; Clang ASan/UBSan gap from the container closed |
| gcc-m32 | ubuntu-24.04, GCC 13 `-m32` | pass (first 32-bit run of the suite) | R1 held (643/0, `sizeof(void*) = 4`), R2 held (own declaration: no) | R1 closed on ILP32 libsupc++ |
| msvc-x64 | windows-2022, MSVC 17.14 x64 | pass | R3 c++14..23 held; R3-nodiscard-msvc: C4834 from C++17, silent in C++14; R3-unused build-ok | R3 closed on MSVC x64 |
| msvc-x86 | windows-2022, MSVC 17.14 Win32 | pass (first 32-bit MSVC run of the suite) | same as msvc-x64, all held | R3 closed on MSVC x86 |
| clangcl | windows-2022, ClangCL x64 | pass | R3 held; R3-nodiscard-clangcl: diagnosed in every standard (`warn_unused_result` in C++14, `nodiscard` from C++17); R3-unused build-ok | R3 closed on ClangCL |

Not covered by review CI: gcc-latest was left out on purpose (GCC 14 is in the existing CI; libsupc++ LP64 verified
in the container). H4 and H6 have no CI job that can decide them (D4, D6).

Proposed permanent CI extension for the owner (not added to existing workflows): a Linux Clang + libc++ job with
`-fsanitize=address,undefined`, a `-m32` GCC job and an MSVC Win32 job — each found the suite green here, but none of
these configurations is in the existing matrix, and the first two cover the pre-C++17 ABI path (D1).

## Decisions

Source: control run. Decisions from earlier reviews were deliberately NOT picked up (user instruction);
only the known decision for this repository from the runbook's Applicability section was added.

| ID | decision | why | reference |
|---|---|---|---|
| D1 | The pre-C++17 `detail::uncaught_exceptions()` (GCC/Clang, non-MSVC) reading `__cxa_eh_globals::uncaughtExceptions` through `__cxa_get_globals()` at a fixed offset is the same design as yacppl `state_saver`; it is treated as a known portability risk to be verified by the yacppl-style run-time R probe on libc++abi and 32-bit in review CI, not re-proposed as a redesign from the container alone. | Runbook Applicability: "scope_guard — close in design to yacppl's state_saver: a custom pre-C++17 uncaught_exceptions() is the same kind of risk; the yacppl R probe for libc++abi and 32-bit fits almost unchanged." | runbook Applicability; `include/scope_guard.hpp:62-76,122-130` |
| D2 | H2 (FORMAL): the factories' and `operator<<`'s noexcept-specification also counts the destructor of the returned guard, so `make_scope_exit(L{})` is not `noexcept` when only the action call may throw. Left as is; the step 3 [proposal] `a8deabd` rewrites the specification with traits but keeps its value identical (40/40 configurations). | Observable only through the C++17 function type; dropping the destructor term widens `noexcept` (signature change, API); FORMAL, not CONFIRMED. Awaiting the owner (report Q2). | `include/scope_guard.hpp:253,262,271,279,286,293` (base); evidence "Hypothesis verdicts" H2 |
| D3 | H3: if moving the action into a guard throws, the action is not run; a guard move uses the action's move even when it may throw, and a failed move leaves the source active. Left as is. | Deliberate, pinned by `test/config_test.cpp:118-177`; LFTS v3 semantics would change public behaviour (API). Awaiting the owner (Q3). | `include/scope_guard.hpp:222-234`; evidence H3 |
| D4 | H4 (UNCLEAR): the "GCC < 4.7" clause also matches every Clang (`__GNUC__` 4.2), so the header's own `__cxa_get_globals() noexcept` declaration is emitted on all Clang builds. Left as is. | Harmless with libstdc++ (suite passes); the breaking case (Clang + libcxxrt `<cxxabi.h>` without `noexcept`) was only emulated and cannot be built in the container or review CI. Candidate fix `!defined(__clang__)` touches the R2 declaration matrix (libc++abi/OpenBSD/QNX), so no change without that toolchain (Q9). Not a revisit of D1. | `include/scope_guard.hpp:64-66`; evidence H4 |
| D5 | H5: before C++17 the header includes `<cxxabi.h>`, which injects a global `namespace abi`, so a user global `abi` compiles in C++17 and fails in C++11/14. Deferred. | Impact low; the fix (own ABI-library-specific declaration of `__cxa_get_globals` instead of `<cxxabi.h>`) is not trivially safe: only the libstdc++ declaration can be verified here, other ABI libraries (libc++abi, libcxxrt, OpenBSD, QNX) cannot, and it removes a transitive include users may rely on. Options in report Q5. Not a revisit of D1. | `include/scope_guard.hpp:62-63`; evidence H5 |
| D6 | H6 (UNCLEAR): the `uncaught_exceptions()` branch is chosen by compiler identity and `__cplusplus`, not by `__cpp_lib_uncaught_exceptions`. Left as is. | No reachable toolchain (non-GNU-emulating compiler, macOS < 10.12 deployment target) can confirm a failure; mechanism only emulated. Q10. | `include/scope_guard.hpp:122-130`; evidence H6 |
| D7 | H7 (+ O4): TUs with different action policies (or SUPPRESS with and without `-fno-exceptions`) give the same inline `~scope_guard` different definitions; the linker keeps one, behaviour depends on link order, nothing diagnoses it. Left as is. | README already requires consistent settings in every TU; detection (inline namespace per policy) is a design/ABI change; mixing C++14/C++17 TUs is FORMAL on libsupc++. Q6. | `include/scope_guard.hpp:44-56,82-90,240`; evidence H7, O4 |
| D8 | H8: `SCOPE_GUARD_CATCH_HANDLER` is expanded in a class template member in `scope_guard::detail`, so names it uses must be declared before the include. Left as is (documentation gap). | README is incomplete, not contradicted; step 3 changes documentation only when a fix requires it. Q7. | `include/scope_guard.hpp:49,58-60,85`; README "Exception settings"; evidence H8 |
| D9 | H9: under the default policy `~scope_fail` is `noexcept(false)` for a non-noexcept action (LFTS: `noexcept`). Left as is. | Changing it changes `is_nothrow_destructible` and one exotic catchable case into terminate (API, design). Q4. | `include/scope_guard.hpp:240`; evidence H9 |
| D10 | H10: under `SCOPE_GUARD_NO_THROW_ACTION` a pointer to a `noexcept` function is accepted only from C++17 (noexcept is part of the function type since P0012R1). Left as is. | Language rule, not a library defect; at most a README remark (Q7). | `include/scope_guard.hpp:186-191,201-204`; evidence H10 |
| D11 | H13 (+ O6): with the `__LINE__` fallback or a user `NEARGYE_SCOPE_GUARD_COUNTER=__LINE__`, two guards on one line collide and nested `WITH_*` on one line shadow. Left as is. | Impact low: every CI compiler has `__COUNTER__`; the `__COUNTER__` ODR token mismatch is formal. Documentation note at most (Q7). | `include/scope_guard.hpp:342-348,360-376`; evidence H13 |
| D12 | H16: `write_basic_package_version_file(... AnyNewerVersion)` makes `find_package(scope_guard 0.9)` accept any future 1.x/2.x. Left as is. | Changes consumer-visible packaging behaviour (API); owner's versioning policy. Q8. | `CMakeLists.txt:30`; evidence H16, #32 |
| D13 | H1 residual after the [proposal]: a detection idiom on `make_scope_*` reports true for lvalues and const rvalues (the call is then rejected by the static_assert / deleted constructor); a const rvalue gets no friendly message; a throwing action under `SCOPE_GUARD_NO_THROW_ACTION` is still a hard error in detection. Left as is. | SFINAE-rejecting lvalues would replace the pinned diagnostic "make_scope_exit requires an rvalue action" (`compile-fail-lvalue-action`) with "no matching function" (API, owner choice, Q1); the NO_THROW static_assert is the intended configuration diagnostic. | `include/scope_guard.hpp:252-274`; step 3 commit `a8deabd` |
| D14 | H14 and H15 REJECTED (no superlinear library term in compile time; test targets get exactly one `-std=`). O5 (front-end cost per guard, only at thousands of guards in one scope) and O11 (test-only Clang `-Weverything` warnings) need no change. | Evidence shows no project defect. | evidence H14, H15, #29/#30, O5, O11 |

## Hypothesis verdicts

Step 2b. Every "How to check" from `.review/hypotheses.md` was re-run against the real project header
(`/tmp/scope_guard-step2b/include` = copy of `include/` at `a949020`, `diff -r` → identical) with `g++-13` 13.3.0 and
`clang++-18` 18.1.3 (libstdc++ 13). Shorthand: `INC=-I/tmp/scope_guard-step2b/include`; sources in
`/tmp/scope_guard-step2b/h/`, logs in `/tmp/scope_guard-step2b/logs/`. Repository files were not modified.

| H | verdict | command | key output line | comment |
|---|---|---|---|---|
| H1 | CONFIRMED | `for c in g++-13 clang++-18; for s in 11 14 17 20; for d in "" -DLV -DCRV -DNTA: $c -std=c++$s $d $INC -fsyntax-only h1.cpp` (`logs/h1.log`) | `scope_guard.hpp:253:82: error: use of deleted function '...scope_guard(A&) [with F = L&; ...]'` (GCC), `scope_guard.hpp:253:91: error: call to deleted constructor of 'scope_exit<L &>'` (Clang); CRV → `scope_guard(const A&)`; NTA → `scope_guard.hpp:202: static assertion failed: scope_guard requires noexcept invocable action.` | 24/24 LV/CRV/NTA runs are hard errors inside the header (none reached the probe's own `static_assert`: `grep -c 'rejected softly'` → 0); "none" builds 8/8. Control `h1_ctrl.cpp`: `can<int>` is false softly, so only the noexcept-spec/class instantiation is outside the immediate context. C++20 `requires { make_scope_exit(declval<L&>()); }` is a hard error as well on both compilers. Quality / API (detection traits). Same root as O1. |
| H2 | FORMAL | `$c -std=c++$s $INC h2_acc.cpp && ./h2a` (`logs/h2_accuracy.log`); original probe `h2.cpp` (`logs/h2_h3_h9.log`) | original probe: `static assertion failed: factory should be noexcept when the move is` (8/8); corrected probe: `noexcept(f())=0 (f is noexcept, returns G with noexcept(false) dtor)`, `noexcept(fixed(L{}))=0`, C++17/20 `fn type noexcept: fixed<L>=1 make_scope_exit<L>=0` | **The hypothesis' probe was inaccurate**: `noexcept(make_scope_exit(L{}))` counts the destructor of the returned temporary, so it stays `false` even with the proposed fix (`fixed(L{})` → 0, and for any `noexcept` function returning a type with a `noexcept(false)` destructor). Corrected probe compares the factory's own exception specification via its C++17 function type: the fix makes it `noexcept`, today it is not. Observable only through the function type (`&make_scope_exit<L>` does not convert to a `noexcept` function pointer) and codegen; the `noexcept` operator users would write cannot tell. `make_scope_exit(LN{})` is `noexcept` (bits=18 in `h2_detail.cpp`). |
| H3 | CONFIRMED | `$c -std=c++$s -Wall -Wextra $INC h3.cpp -o h3b && ./h3b` (both, c++11/14/17/20; `logs/h2_h3_h9.log`) | `move threw` / `action sees s=''` (8/8) | The source guard stays active after a throwing move and later runs an action whose `std::string` member was already moved from. "Source stays active" and "no call on construction failure" are pinned by `config_test.cpp:118-177` (`ThrowingMoveAction` is move-only, so the copy-fallback case is not tested). Design-level owner question (LFTS copies on non-noexcept move); **API** if changed. |
| H4 | UNCLEAR | `$c -std=c++$s $INC -E p3.cpp \| grep -c 'extern "C" __cxa_eh_globals\* __cxa_get_globals() noexcept;'`; `clang++-18 -dM -E`; emulation `clang++-18 -std=c++11 -isystem h4fake $INC -fsyntax-only h4use.cpp` (`logs/h4_h5_h8_h10_h11_h12.log`, `logs/h4_emul.log`) | clang++-18 c++11/14: `own-decl lines: 1`; g++-13: `0`; clang `#define __GNUC_MINOR__ 2 #define __GNUC__ 4`; emulation: `scope_guard.hpp:72:30: error: exception specification in declaration does not match previous declaration` | Part 1 confirmed: the GCC<4.7 clause fires on every Clang, so the header's declaration is compiled on Clang + libstdc++ (harmless there: suite passes, evidence #2). Part 2 (breaks on Clang + libcxxrt): only emulated with a stand-in `<cxxabi.h>` that declares `__cxa_get_globals(void)` without `noexcept` (hard error with `-isystem` and `-I`). Needs a real Clang + libcxxrt toolchain (NetBSD/DragonFly or Linux libc++ built on libcxxrt), which the container and the review-CI job list (`clang-libcxx` = libc++abi) do not have, so no H4 probe. The libc++abi side of the same declaration is covered by R2. |
| H5 | CONFIRMED | `$c -std=c++$s $INC -fsyntax-only h5.cpp`, s = 11/14/17 (`logs/h4_h5_h8_h10_h11_h12.log`) | g++-13 c++11/14: `error: namespace alias 'abi' not allowed here, assuming '__cxxabiv1'`; clang++-18 c++11/14: `error: redefinition of 'abi' as different kind of symbol`; c++17: rc=0 on both | A user global `abi` compiles in C++17 and fails in C++11/14, because the header pulls `<cxxabi.h>` in before C++17. Portability, low impact. |
| H6 | UNCLEAR | `$c -std={c++11,gnu++11,c++14,c++17} -fsyntax-only h6_lib.cpp`; emulation `clang++-18 -std=c++11 -U__clang__ -U__GNUC__ $INC -fsyntax-only h6.cpp` (`logs/h6_h13.log`) | libstdc++ strict c++11/14: `'uncaught_exceptions' is not a member of 'std'` (gnu++11 and c++17 rc=0); emulation: `scope_guard.hpp:128:10: error: no member named 'uncaught_exceptions' in namespace 'std'` | The mechanism for (a) is demonstrated: the `#else` branch needs `std::uncaught_exceptions`, and a conforming C++11/14 library need not declare it (libstdc++ does not in strict mode). A real non-GNU-emulating compiler is needed (Oracle Developer Studio, classic xlC, EDG without GNU mode). (b) needs libc++ with a macOS deployment target < 10.12, which current Xcode (minimum 10.13) cannot select. No review-CI job can decide either case, so no H6 probe. |
| H7 | CONFIRMED | `$c -std=c++11 $O $INC main.cpp a.cpp b.cpp` in both link orders, `-O0/-O2`, `-O2 -flto -Wodr`; standards mix `s1.cpp` (c++11) + `s2.cpp` (c++17) (`logs/h7.log`) | `-O0`, b.cpp linked first: `terminate called after throwing an instance of 'int'`, exit 134 (both compilers); a.cpp first: `run_b: exception suppressed (MAY_THROW lost)`; `-O2` and `-flto -Wodr`: correct, **no** `-Wodr` diagnostic; standards mix: `in unwinding: s1-view=1 s2-view=1`, `fails: s1=1 s2=1` | Policy mix: the COMDAT `~scope_guard<Thrower,on_exit_policy>` is emitted in both TUs (`nm`) and the linker keeps one, so behaviour depends on link order and optimisation level, and nothing (not even LTO `-Wodr`) diagnoses it. This breaks the README precondition "Configure these settings consistently in every translation unit", so it is a "nothing detects it" quality issue, not a bug. The standards-mixing ODR violation is FORMAL on x86-64 libsupc++ (both definitions read the same counter). |
| H8 | CONFIRMED | `$c -std=c++{11,17} $INC -fsyntax-only h8.cpp`; positive `h8_ok.cpp` (declaration before the include) built with `-Wall -Wextra -pedantic-errors -Werror` and run (`logs/h4_h5_h8_h10_h11_h12.log`) | g++-13: `error: there are no arguments to 'log_failure' that depend on a template parameter, so a declaration of 'log_failure' must be available`; clang++-18: `scope_guard.hpp:244:7: error: use of undeclared identifier 'log_failure'`; positive: run rc=0 on both | A handler declared after the include does not compile. Documentation gap only (README is incomplete, not contradicted). MSVC `/permissive-` not checked in the container. |
| H9 | CONFIRMED | `$c -std=c++$s $INC -fsyntax-only h9.cpp`, s = 11/14/17/20 (`logs/h2_h3_h9.log`) | `static assertion failed: scope_fail dtor should be noexcept` (8/8) | `is_nothrow_destructible<scope_fail<L>>` is false for a non-noexcept action under the default policy (LFTS: `noexcept`). Design / owner decision; **API** (trait value). |
| H10 | CONFIRMED | `$c -std=c++$s $INC -fsyntax-only h10.cpp`, s = 11/14/17/20 (`logs/h4_h5_h8_h10_h11_h12.log`) | c++11/14: `scope_guard.hpp:202: static assertion failed: scope_guard requires noexcept invocable action.` (Clang: `is_nothrow_invocable_action<void (*&)(), true>`); c++17/20: rc=0 | Under `SCOPE_GUARD_NO_THROW_ACTION`, a pointer to a `noexcept` function is accepted only from C++17 on. The cause is in the language (P0012R1), not the library; at most a README remark. |
| H11 | CONFIRMED | `$c -std=c++{11,17} $INC -fsyntax-only h11.cpp` / `h11_addr.cpp` (`logs/h4_h5_h8_h10_h11_h12.log`) | `static assertion failed: make_scope_exit requires an rvalue action; use std::move or pass a temporary.` for `make_scope_exit(std::move(f))`; `&f` → rc=0 | The advice in the message cannot help for functions (a function expression is always an lvalue); `&f` works. Message-only fix; the pinned substring stays. |
| H12 | CONFIRMED | `$c -std=c++{11,17} $INC -fsyntax-only h12.cpp` (`logs/h4_h5_h8_h10_h11_h12.log`); also evidence #22 / O2 | `#error "user NEARGYE_SCOPE_GUARD_NODISCARD was undefined by the header"` (4/4); no error for `NEARGYE_SCOPE_GUARD_MAYBE_UNUSED` | The user's override is consumed by the `#undef` at `scope_guard.hpp:302`; the other overridable macros survive. |
| H13 | CONFIRMED | `$c -std=c++11 -Wall -Wextra -Wshadow $INC h13a.cpp`, `h13b.cpp` (with and without `-Werror`), `h13c.cpp` default (`logs/h6_h13.log`); also evidence #31 / O6 | `__LINE__`: g++-13 `conflicting declaration 'const auto NEARGYE_SCOPE_GUARD_SCOPE_EXIT_3'`, clang++-18 `redefinition of 'NEARGYE_SCOPE_GUARD_SCOPE_EXIT_3'`; nested `WITH_*`: `declaration of 'NEARGYE_SCOPE_GUARD_FLAG_3' shadows a previous local [-Werror=shadow]` (runs correctly without `-Werror`); default `__COUNTER__`: rc=0 | (a) applies only with the `__LINE__` fallback or a user override; every CI compiler has `__COUNTER__`. (b) The `__COUNTER__` ODR token mismatch in inline functions is formal (not observable). Documentation note at most. |
| H14 | REJECTED | front end: `python3 -I t.py $c -std=c++11 -fsyntax-only big_{SCOPE_EXIT,SCOPE_FAIL}_N.cpp`, N = 1000/2000/4000/8000; baselines `base_N.cpp` (hand-written lambda RAII guard) and `plain_N.cpp` (`logs/h14.log`); back end: evidence #29/#30, O5 | g++-13 SCOPE_EXIT: 2.02 s → 6.28 → 20.94 → 87.24 s; lambda-RAII baseline: 0.43 s → 1.03 → 3.65 → 14.42 s; clang++-18 SCOPE_EXIT: 2.11 → 4.34 → 9.19 → 21.71 s, baseline 0.30 → 0.62 → 1.19 → 2.38 s; SCOPE_FAIL ≈ SCOPE_EXIT | The library adds no superlinear term of its own. GCC 13's front end is superlinear for N lambdas in one scope even with the 20-line baseline (×33 for N ×8; library ×43), and Clang is close to linear for both (library ×10.3, baseline ×7.9). Remaining fact: about 4.7–6× (GCC) and 7–9× (Clang) front-end constant cost per guard against the minimal baseline. Only relevant at N in the thousands (O5). |
| H15 | REJECTED | evidence #3 (`ninja -C t1-gcc-debug -t commands test/scope_guard-cpp11.t \| grep -o -- '-std=[^ ]*'`, `nm -C ... \| grep -c __cxa_get_globals`) | each target compiled with exactly one `-std=` matching its name; `scope_guard-cpp11.t` references `__cxa_get_globals`, `scope_guard-cpp17.t` does not | CMake adds no second standard flag; the pre-C++17 path is compiled and tested. Not re-run. |
| H16 | CONFIRMED | `cmake -S $S -B build -G Ninja ...`; `cmake -DVF=<version file> -DREQ=<req> -P check.cmake` with the generated `scope_guardConfigVersion.cmake` and copies faked to 1.0.0 / 2.3.0 (`logs/h16.log`); also evidence #32 | `installed=1.0.0 requested=0.9 compatible=TRUE`, `installed=2.3.0 requested=0.1 compatible=TRUE`; `installed=0.9.5 requested=1.0 compatible=FALSE` | `AnyNewerVersion` accepts any future major version for a 0.x request. Owner question; **API** (packaging). |

Totals: CONFIRMED 11 (H1, H3, H5, H7, H8, H9, H10, H11, H12, H13, H16), FORMAL 1 (H2), REJECTED 2 (H14, H15),
UNCLEAR 2 (H4, H6: neither the container nor the review-CI job list has the toolchain, so there is no H probe).
Most CONFIRMED items are design or documentation questions (H3, H7, H8, H9, H10, H13, H16), and none breaks documented
behaviour. Code-level candidates: H1 (with H2's noexcept spec), H5, H11, H12.

#### Probes for review CI (`.review/probes/`)

| file | probe id | where | std | expect | covers |
|---|---|---|---|---|---|
| `R1.cpp` | R1 | macos-appleclang clang-libcxx gcc-m32 gcc-latest | c++11 c++14 c++17 | run-ok | R1 / D1: pre-C++17 `detail::uncaught_exceptions()` against the known number of in-flight exceptions (nesting 0..64, rethrow, `std::rethrow_exception`), SCOPE_FAIL/SCOPE_SUCCESS decisions; c++17 is the `std::` control |
| `R2.cpp` (+ `R2_last.cpp` via `with:`) | R2 | macos-appleclang clang-libcxx gcc-latest gcc-m32 | c++11 c++14 | run-ok | R2: the header's own `__cxa_get_globals` declaration vs `<cxxabi.h>`, both include orders in one program, plus run-time counting in each TU; prints whether the own declaration was compiled |
| `R3.cpp` | R3 | msvc-x64 msvc-x86 clangcl | c++14 c++17 c++20 c++23 | run-ok | R3: all public macros under `/W4 /WX /permissive-` with the MSVC/ClangCL attribute branches, execution counts |
| `R3_unused.cpp` | R3-unused | msvc-x64 msvc-x86 clangcl | c++14 c++17 c++20 c++23 | build-ok | unmasking the `__pragma(warning(suppress : 4100 4101 4189))` / `[[maybe_unused]]` fallback (evidence #15) |
| `R3_nodiscard_msvc.cpp` | R3-nodiscard-msvc | msvc-x64 msvc-x86 | c++14 c++17 c++20 c++23 | build-fail "C4834"; c++14: build-ok | MSVC NODISCARD branch (`[[nodiscard]]` from C++17; `_Check_return_` silent without `/analyze`) |
| `R3_nodiscard_clangcl.cpp` | R3-nodiscard-clangcl | clangcl | c++14 c++17 c++20 c++23 | build-fail "ignoring return value" | ClangCL takes the `__clang__` branch (`__warn_unused_result__` / `[[nodiscard]]`) |

R4, R5 and R6 can be checked in the container and are covered by evidence #26 / O4 (R4), the suite's move and
unwinding tests in #1/#2/#14 (R5) and #31 / O6 / H13 (R6), so they have no probe.

Local runs of the Appendix A runner (`/tmp/scope_guard-step2b/review/probes/run.py`, copied verbatim, with the
probes and `include/`), `logs/runner-local.log`:
`python3 -I review/probes/run.py --job gcc-latest --style gcc --cxx g++-13`, and the same with `--job clang-libcxx`
and `--job macos-appleclang` using `--cxx clang++-18` (libstdc++, so these are header/build checks, not libc++abi):
R1 c++11/14/17 and R2 c++11/14 are `held` on all three, e.g. `R1 ok: 643 checks, 0 errors; path: custom __cxa_get_globals() + sizeof(void*); sizeof(void*) = 8`,
`R2 ok; own declaration: yes` (clang++-18) / `no` (g++-13).
Sensitivity check (`logs/probe-mutation.log`): with the offset changed in a scratch copy of the header to
`sizeof(void*) + sizeof(unsigned int)`, R1 fails (`R1 FAILED: 643 checks, 573 errors`, rc=1), and R2 fails
(`cxxabi first: fails=0 successes=1 in-flight=0 -> WRONG`, rc=1) on both compilers.
The R3 files were built with the GCC/Clang analogues (`logs/r3-local.log`): R3 and R3-unused `rc=0` on c++14…c++23. The R3-nodiscard
source fails with `ignoring return value of function declared with 'warn_unused_result'` (clang c++14) / `'nodiscard'` (c++17+).
The MSVC/ClangCL outcomes themselves come from review CI only. `gcc-m32` cannot run locally:
`g++-13 -m32 ...` → `fatal error: bits/c++config.h: No such file or directory`.

## Empirical checks (step 2)

### Check table

| # | check | configuration | result | command |
|---|---|---|---|---|
| 1 | T1 baseline, full CMake suite | g++-13, Debug, Ninja | **pass**, 21/21 (cpp11/14/17/20, 3 config, runtime-fail, cxxabi first/last, 3 no-exceptions, 4 compile-fail, 4 integration) — `logs/t1-gcc-debug.log` | `cmake -S $S -B t1-gcc-debug -G Ninja -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_BUILD_TYPE=Debug && cmake --build t1-gcc-debug -j4 && ctest --test-dir t1-gcc-debug --output-on-failure -j4` |
| 2 | T1 baseline, full CMake suite | clang++-18, Debug, Ninja | **pass**, 21/21 — `logs/t1-clang-debug.log` | same as #1 with `clang++-18` |
| 3 | Actual standard flags of test targets | both, from #1/#2 | each target is compiled with exactly one `-std=` (`cpp11`→`-std=c++11` … `cpp20`→`-std=c++20`; config/cxxabi/no-exceptions → `-std=c++11`); `scope_guard-cpp11.t` references `__cxa_get_globals` (pre-C++17 path compiled), `scope_guard-cpp17.t` does not — `logs/std-flags.log` | `ninja -C t1-gcc-debug -t commands test/scope_guard-cpp11.t \| grep -- ' -c ' \| grep -o -- '-std=[^ ]*'`; `nm -C .../scope_guard-cpp11.t \| grep -c __cxa_get_globals` |
| 4 | `test.cpp` on newest standards | g++-13 `-std=c++23`, `-std=c++2b` | **pass**, 98/98 assertions each — `logs/t1-std-g++-13-c++23.log`, `...-c++2b.log` | `g++-13 -std=c++23 -Wall -Wextra -pedantic-errors -Werror $INC $S/test/test.cpp -o bin/t && bin/t` |
| 5 | `test.cpp` on newest standards | clang++-18 `-std=c++23`, `c++2c`, `c++26` | **pass**, 98/98 each — `logs/t1-std-clang++-18-*.log` | same with `clang++-18 -std=c++2c` |
| 6 | GCC c++2c availability | g++-13 | environment: `g++-13: error: unrecognized command-line option '-std=c++2c'` — `logs/gcc13-c++2c.log` | `g++-13 -std=c++2c -x c++ -fsyntax-only /dev/null` |
| 7 | ASan + UBSan, full CMake suite + examples | g++-13 Debug, `-fsanitize=address,undefined -fno-sanitize-recover=all`, `detect_leaks=1` | **pass**, 21/21; 3/3 examples exit 0; instrumentation verified (`nm`: 11 `__asan_report*`, 12 `__ubsan_handle*` in `scope_guard-cpp11.t`; 60 `-fsanitize=address,undefined` in `build.ninja`) — `logs/t1-gcc-asan-ubsan.log`, `logs/t1-asan-*_example.log` | `cmake -S $S -B t1-gcc-asan -G Ninja -DCMAKE_CXX_COMPILER=g++-13 -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_CXX_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer" -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined && cmake --build ... && ASAN_OPTIONS=detect_leaks=1 ctest ...` |
| 8 | ASan + UBSan, newest standard and config scenarios | g++-13: `test.cpp` c++23; `config_test.cpp` 4 scenarios × c++11/c++17 | **pass**: c++23 SUCCESS; 8/8 config — `logs/t1-gcc-asan-cxx23.log`, `logs/t1-gcc-asan-config-stds.log` | `g++-13 -std=c++23 -g -fsanitize=address,undefined -fno-sanitize-recover=all $INC $S/test/test.cpp` |
| 9 | Clang ASan / UBSan runtime | clang++-18 | environment: `ld: cannot find .../libclang_rt.asan_static-x86_64.a`, `.../libclang_rt.ubsan_standalone-x86_64.a` — `logs/clang-asan-probe.log`, `logs/clang-ubsan-probe.log` | `clang++-18 -fsanitize=address bin/empty.cpp` |
| 10 | UBSan trap mode, full CMake suite + examples + c++2c | clang++-18 Debug, `-fsanitize=undefined -fsanitize-trap=undefined` | **pass**, 21/21; 3/3 examples exit 0; `test.cpp` c++2c SUCCESS — `logs/t1-clang-ubsan-trap.log`, `logs/t1-clang-trap-cxx2c.log` | `cmake ... -DCMAKE_CXX_COMPILER=clang++-18 "-DCMAKE_CXX_FLAGS=-fsanitize=undefined -fsanitize-trap=undefined"` |
| 11 | T2 Release + Unix Makefiles generator | g++-13, clang++-18 | **pass**, 21/21 each (`-O3 -DNDEBUG` confirmed in `flags.make`) — `logs/t2-{gcc,clang}-release-make.log` | `cmake -S $S -B t2-gcc-release-make -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Release ...` |
| 12 | T2 `-fno-rtti` | g++-13, clang++-18, Debug | **pass**, 21/21 each — `logs/t2-*-nortti.log` | `cmake ... "-DCMAKE_CXX_FLAGS=-fno-rtti"` |
| 13 | T2 `-fno-rtti -DNDEBUG` | g++-13, clang++-18, Debug | **pass**, 21/21 each — `logs/t2-*-nortti-ndebug.log` | `cmake ... "-DCMAKE_CXX_FLAGS=-fno-rtti -DNDEBUG"` |
| 14 | P8/T2 config, no-exceptions and runtime-fail tests on every standard | g++-13 c++11/14/17/20/23; clang++-18 c++11/14/17/20/23/2c; 5 config scenarios + 3 `-fno-exceptions` policies + throw-during-unwinding (exit 42) | **pass**, 99/99 — `logs/t2-config-stds.log` | loop in log; e.g. `clang++-18 -std=c++2c -Wall -Wextra -pedantic-errors -Werror $INC -DSCOPE_GUARD_TEST_CXXABI -DSCOPE_GUARD_TEST_CXXABI_FIRST $S/test/config_test.cpp` |
| 15 | Unmask suppressions | MSVC `/wd4702` (test, example CMake); `__pragma(warning(suppress: 4100 4101 4189))` MAYBE_UNUSED fallback | not applicable in container — both are MSVC-only; CI only | — |
| 16 | Unmask GCC/Clang `MAYBE_UNUSED` (the attribute that suppresses unused warnings) | both compilers, all standards, TU with `SCOPE_EXIT/FAIL/SUCCESS/DEFER/MAKE_SCOPE_EXIT` | **0** warnings with and **0** without the attribute (`-DNEARGYE_SCOPE_GUARD_MAYBE_UNUSED=`) — `logs/attr.log` | `g++-13 -std=c++11 -Wall -Wextra -Wunused -I$S/include '-DNEARGYE_SCOPE_GUARD_MAYBE_UNUSED=' -fsyntax-only probes/maybe_unused.cpp` |
| 17 | Strict GCC warnings (`-Wall -Wextra -pedantic -Wconversion -Wsign-conversion -Wshadow -Wold-style-cast -Wuseless-cast -Wcast-qual -Wzero-as-null-pointer-constant -Wextra-semi`) | g++-13 × c++11/14/17/20/23 × (`test.cpp`, 3 examples, `config_no_exceptions.cpp`, 4 config scenarios) | **pass**: 0 diagnostics in 45 compilations — `logs/strict-gcc.log` | `g++-13 -std=$std $GCC_FLAGS $INC -fsyntax-only $S/test/test.cpp` |
| 18 | Clang `-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Wno-unsafe-buffer-usage -Wno-switch-default` | clang++-18 × c++11…c++2c, same TUs | header: **0** diagnostics; 7 distinct warnings, all in test files (`-Wmissing-prototypes`, `-Wmissing-variable-declarations`, `-Wmissing-noreturn`) — `logs/strict-clang.log` | `clang++-18 -std=$std $CLANG_FLAGS $INC -fsyntax-only $S/test/test.cpp` |
| 19 | `-pedantic-errors` on the minimum standard | g++-13, clang++-18, `-std=c++11` | **pass**: whole suite already uses `-pedantic-errors -Werror` (#1/#2); all-macros TU with `-Wundef` too — `logs/pedantic-min-std.log` | `g++-13 -std=c++11 -pedantic-errors -Wall -Wextra -Wundef -Werror -I$S/include probes/maybe_unused.cpp` |
| 20 | Attribute macro branches | both × c++11/14/17/20 (preprocessed) | **as claimed**: c++11/14 → `__attribute__((__warn_unused_result__))`, `__attribute__((__unused__))`, `uncaught_exceptions()` via `__cxa_get_globals`; c++17/20 → `[[nodiscard]]`, `[[maybe_unused]]`, `std::uncaught_exceptions` — `logs/attr.log` | `echo '#include <scope_guard.hpp>' \| g++-13 -std=c++11 -I$S/include -E -x c++ - \| grep 'make_scope_exit(F&& action)'` |
| 21 | NODISCARD effect | both × all standards; discard result of 3 factories | **pass**: 3/3 `-Wunused-result` warnings on every compiler/standard — `logs/attr.log` | `g++-13 -std=$std -Wall -Wextra -I$S/include -fsyntax-only probes/nodiscard.cpp` |
| 22 | User override of `NEARGYE_SCOPE_GUARD_NODISCARD` | both × all standards | override honoured inside the header (0 discard warnings), but the user's macro is `#undef`'d after the include (`#pragma message` probe) — see Observation O2 — `logs/attr.log` | `probes/user_nodiscard.cpp` |
| 23 | `detail::uncaught_exceptions()` vs `std::uncaught_exceptions()` (differential, R1/D1 in-container part) | both × gnu++11/gnu++14 (custom path) / c++17 (std path); nesting depth 1, 10, 1000 (up to 1001 simultaneous uncaught), rethrow path; + GCC ASan/UBSan depth 100 | **pass**: 0 mismatches in all 19 runs — `logs/uncaught-diff.log` | `g++-13 -std=gnu++11 -O2 -DLIMIT=1000 -I$S/include probes/uncaught_diff.cpp && ./ud` |
| 24 | Thread-locality of the custom counter | both gnu++11, 8 threads × 20000 iterations of SCOPE_FAIL/SCOPE_SUCCESS; GCC TSan | **pass**: `bad=0` in all 3 runs, no TSan report — `logs/uncaught-threads.log` | `g++-13 -std=gnu++11 -O1 -pthread -fsanitize=thread -I$S/include probes/uncaught_threads.cpp` |
| 25 | Comparison with std counterparts (traits) | g++-13 c++23, clang++-18 c++2c; 27 callable types | `is_noarg_returns_void_action<T>` == `is_invocable_v<T> && is_same_v<invoke_result_t<T>, void>` on all 27; `is_nothrow_invocable_action<T>` == `std::is_nothrow_invocable<T>` on all 27; differs from `std::is_invocable_r<void,T>` exactly for non-void returns (`IntF`, `int(*)()`, `std::function<int()>`, int-returning lambda) — documented "callbacks must return void" (O7) — `logs/std-compare.log` | `clang++-18 -std=c++2c -I$S/include probes/std_compare.cpp && ./sc` |
| 26 | P6 noexcept matrix | both × c++11/c++17 × exceptions on/`-fno-exceptions` × 3 action policies, all with `-fno-rtti` | builds 24/24; consistent across compilers/standards; `~scope_guard` with a non-noexcept action is `noexcept(true)` only for SUPPRESS **with** exceptions, `noexcept(false)` for SUPPRESS under `-fno-exceptions` (O4) — `logs/p6-noexcept.log` | `g++-13 -std=c++11 -fno-exceptions -DSCOPE_GUARD_SUPPRESS_THROW_ACTION -fno-rtti -I$S/include probes/p6_noexcept.cpp` |
| 27 | Limits: deep recursion with 3 guards per frame, exception from the bottom | both × c++11/c++17, depth 100001, `ulimit -s unlimited` | **pass**: exits=fails=100001, succ=0 — `logs/limits.log` | `probes/recursion.cpp` |
| 28 | Limits: 200 nested `WITH_SCOPE_EXIT` | both × c++11/c++17, `-Wshadow` | **pass**: 200 executed, 0 warnings, ≤2.2 s — `logs/limits.log` | `probes/nested_with.cpp` |
| 29 | Limits: 5000 `SCOPE_EXIT` in one scope (timeout 120 s) | both × c++11/c++17, `-O1` | g++-13: **timeout** (>120 s) at both standards; clang++-18: builds in 106–112 s, runs correctly (5000 executed in reverse order) — see O5 — `logs/limits.log` | `timeout 120 g++-13 -std=c++11 -O1 -I$S/include probes/many_guards.cpp` |
| 30 | Compile-time scaling vs a minimal hand-written RAII guard | both, c++11, -O0/-O1, N = 500/1000/2000 | library ≈ 2.3–5.6× time and ≈ 1.4–2.9× peak RSS of the baseline; N=2000: g++-13 14.2–15.4 s / 437–504 MB (baseline 4.4–4.7 s / 181–230 MB), clang++-18 7.6–10.2 s / 288–304 MB (baseline 3.1–3.4 s / 153–154 MB); all binaries run correctly — `logs/limits-compile-time.log` | `python3 -I measure.py g++-13 -std=c++11 -O1 -I$S/include probes/mg_lib_2000.cpp -o bin/mg` |
| 31 | Macro hygiene with `NEARGYE_SCOPE_GUARD_COUNTER=__LINE__` (fallback path) | both, c++11, `-Wshadow -Werror` | `test.cpp` builds and passes; two guards on one line → `conflicting declaration 'const auto NEARGYE_SCOPE_GUARD_SCOPE_EXIT_2'` (GCC) / `redefinition` (Clang), nested `WITH_*` on one line → `-Wshadow`; default `__COUNTER__` path: same TU builds, 0 warnings, correct result (O6) — `logs/macro-hygiene.log` | `g++-13 -std=c++11 -Wshadow -DNEARGYE_SCOPE_GUARD_COUNTER=__LINE__ -I$S/include probes/same_line.cpp` |
| 32 | P1 install + `find_package` consumer | g++-13, clang++-18, Ninja | **pass**: installed `include/scope_guard.hpp`, `lib/cmake/scope_guard/{scope_guardConfig,scope_guardConfigVersion}.cmake`, `share/scope_guard/LICENSE`; consumer builds and prints `version 0.9.5 n=2`; requests `<none>`, `0.9`, `0.9.5`, `0.1` accepted, `0.9.6`, `1.0` rejected ("compatible with requested version") — `logs/p1.log`, `logs/p1-*.log` | `cmake --install p1/build --prefix p1/prefix`; `cmake -S p1/consumer -B ... -DCMAKE_PREFIX_PATH=p1/prefix -DREQ=0.9` |
| 33 | P1 build-tree export | both | **pass**: consumer via `-Dscope_guard_DIR=p1/build` — `logs/p1-buildtree-*.log` | `cmake -S p1/consumer -B ... -Dscope_guard_DIR=$PWD/p1/build -DREQ=0.9` |
| 34 | P1 add_subdirectory + subproject defaults | both (integration tests in #1/#2, #11) | **pass** (`subproject-consumer`, `subproject-defaults`, `install-consumer-*` on Ninja and Makefiles) | ctest from #1/#2/#11 |
| 35 | P1 pkg-config | — | not applicable: project installs no `.pc` file (`find p1/prefix -name '*.pc'` → 0) | — |
| 36 | P2 compile-fail tests fail for the stated reason | both × c++11/17/20 × 4 scenarios | **pass**: each produces its intended `#error`/`static_assert` text; the lvalue scenario additionally emits a *first* error "use of deleted function … scope_guard(A&)" (GCC) / "call to deleted constructor of 'scope_exit<Action &>'" (Clang) from the `noexcept(...)` clause before the static_assert (O1) — `logs/p2.log` | `clang++-18 -std=c++11 -Wall -Wextra -pedantic-errors -Werror -I$S/include -DSCOPE_GUARD_TEST_REJECT_LVALUE_ACTION -fsyntax-only $S/test/failure_test.cpp` |
| 37 | P2 positive controls | both × c++11/17/20 | **pass** 24/24 (single policy; noexcept action under NO_THROW; nothrow-move under NO_THROW_CONSTRUCTIBLE; `std::move`d lvalue to all 3 factories); `make_scope_fail`/`make_scope_success` with lvalue emit their own rvalue diagnostic 12/12 — `logs/p2-positive.log` | `g++-13 -std=c++11 ... -DPOS_RVALUE_ACTION probes/p2_positive.cpp && bin/pos` |
| 38 | P2 runtime-fail test | both, all standards | **pass**: exit 42 with "scope_fail action terminated" (#1/#2 and 11/11 standard runs in #14) | `failure_test.cpp -DSCOPE_GUARD_TEST_THROW_DURING_UNWINDING` |
| 39 | Existing CI after push (T2: GCC 12/14, Clang 16/17, AppleClang, MSVC, ClangCL) | — | not done by step 2: only the orchestrator pushes | — |

Totals: 33 checks passed, 0 project failures; 2 environment failures (#6 no c++2c in GCC 13, #9 no Clang sanitizer runtime);
1 limit exceeded (#29, GCC > 120 s for 5000 guards in one scope — not a correctness failure); 3 not applicable / not done (#15, #35, #39).

### Confirmed problems

None. No test, sanitizer, strict-warning, differential or packaging check produced a project failure in the container.
The items below are observations, not violations of documented behaviour.

### Observations

- **O1 — lvalue rejection: the friendly message is not the first error.** `make_scope_exit(action)` with an lvalue first fails
  inside the `noexcept(noexcept(scope_exit<F>{...}))` clause (deleted `scope_guard(A&)`), then the `static_assert`
  "make_scope_exit requires an rvalue action" follows. Reproduction (`logs/p2.log`):
  `g++-13 -std=c++11 -I$S/include -DSCOPE_GUARD_TEST_REJECT_LVALUE_ACTION -fsyntax-only $S/test/failure_test.cpp` →
  `scope_guard.hpp:253:82: error: use of deleted function '...scope_guard(A&) [with F = Action&; ...]'` then
  `scope_guard.hpp:254:48: error: static assertion failed: make_scope_exit requires an rvalue action; ...`.
  The CMake test passes because it matches a substring anywhere in the output.
- **O2 — user override of `NEARGYE_SCOPE_GUARD_NODISCARD` is consumed.** With `#define NEARGYE_SCOPE_GUARD_NODISCARD`
  before the include, the override is used (0 discard warnings), but the macro is no longer defined after the include
  (header line 302 `#undef`s it). `NEARGYE_SCOPE_GUARD_MAYBE_UNUSED`/`STR_CONCAT`/`COUNTER` are not `#undef`'d. `probes/user_nodiscard.cpp`, `logs/attr.log`.
- **O3 — `MAYBE_UNUSED` hides nothing on GCC 13 / Clang 18**: guard objects have non-trivial destructors, so no unused-variable
  warning is emitted even without the attribute (#16). Its value is MSVC-only (C4189 etc.) — CI only.
- **O4 — exception spec of `~scope_guard` depends on `-fno-exceptions` under `SCOPE_GUARD_SUPPRESS_THROW_ACTION`.**
  For a non-noexcept action the destructor is `noexcept(true)` with exceptions enabled and `noexcept(false)` with
  `-fno-exceptions` (`logs/p6-noexcept.log`). Mixing such TUs gives the same inline destructor different exception
  specifications; README requires consistent *macro* settings across TUs but says nothing about `-fno-exceptions`.
- **O5 — compile cost per guard.** 5000 `SCOPE_EXIT` in one scope exceed 120 s with g++-13 -O1 and take 106–112 s with
  clang++-18 (#29). At N=500…2000 the library costs ≈2.3–5.6× the time of a minimal RAII guard (#30); growth is
  slightly superlinear for both the library and the baseline (GCC N 1000→2000: lib ×2.5, baseline ×2.2), i.e. the
  superlinearity is in the compiler back end, not template recursion. Realistic code is far below these N.
- **O6 — `__LINE__` fallback** (only when `__COUNTER__` is absent or the user overrides `NEARGYE_SCOPE_GUARD_COUNTER`):
  two guards on one line collide (hard error), nested `WITH_*` on one line shadow (`-Wshadow`). Default path is clean (#31).
- **O7 — trait semantics vs std.** `is_noarg_returns_void_action` is "invocable with no args and returns exactly `void`",
  stricter than `std::is_invocable_r<void, T>` (which accepts any return type); consistent with README "callbacks must return void".
  `is_nothrow_invocable_action` matches `std::is_nothrow_invocable` on all 27 probed types (#25).
- **O8 — construction-failure semantics differ from LFTS v3** (LFTS calls `f()` if constructing the stored callable throws
  for `scope_exit`/`scope_fail`); the project deliberately does not call it — asserted by `config_test.cpp`
  "a throwing action move does not establish a guard" (passes on all configurations, #14).
- **O9 — version compatibility.** `find_package(scope_guard 0.1)` accepts installed 0.9.5 (`AnyNewerVersion`, #32). Fact only; relevance is for step 2b/owner.
- **O10 — profile correction.** Profile §3 says "`SCOPE_GUARD_NO_THROW_CONSTRUCTIBLE` positive path not covered";
  `test/test.cpp:26` defines it, so every `scope_guard-cppNN.t` target exercises the positive path.
- **O11 — test-only `-Weverything` findings** (`-Wmissing-prototypes`, `-Wmissing-variable-declarations`,
  `-Wmissing-noreturn` in `test/test.cpp`, `test/config_test.cpp`); the header itself is clean under `-Weverything` and the GCC strict set.
- **O12 — R1/D1 in-container status.** On x86-64 libsupc++ the fixed-offset read agrees with `std::uncaught_exceptions()` at
  depths up to 1001 and across 8 threads (#23, #24). This says nothing about libc++abi, 32-bit or other ABIs (review CI, per D1).

### Not done / could not

| What | Reason |
|---|---|
| Clang ASan / LeakSanitizer / UBSan with runtime; libFuzzer | `libclang_rt.*` missing in container (#9); Clang covered by UBSan trap mode only. P5 not selected in profile. |
| `-m32` (R1 32-bit) | no multilib (`bits/c++config.h` missing for -m32, profile §6) — review CI. |
| libc++ / libc++abi on Linux, R2 `__cxa_get_globals` declaration path, R1 on libc++abi | libc++ headers missing — review CI / macOS CI. |
| MSVC, ClangCL, AppleClang (incl. unmasking `/wd4702` and the `__pragma` MAYBE_UNUSED fallback, R3 MSVC branches) | no toolchain — CI only. |
| GCC ≠ 13, Clang ≠ 18 (newest = oldest available) | single version of each in container — existing CI covers GCC 12/14, Clang 16/17. |
| C++2c on GCC | GCC 13 has no `-std=c++2c` (#6); covered by clang++-18 c++2c. |
| Existing CI after push (T2) | push is the orchestrator's job. |
| T3 boundary search | no confirmed problems to bound. |
| Verification of `.review/hypotheses.md` | per item 9, step 2b's job. The file appeared after T1; its probes need only `g++-13`, `clang++-18`, Ninja/Makefiles, all already exercised in T1/T2, so no extra T2 configuration was added. |
| P3, P4, P5, P7, P9–P12 | not marked [x] in profile §10. |
