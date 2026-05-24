# SIC/XE Linking Loader Task Plan

## Goal

Build a SIC/XE Linking Loader for the current C++ assembler project.

The loader should read one or more SIC/XE object programs, resolve external
symbols, apply relocation/modification records, and output a loaded memory image
plus a loading map for verification.

## Current Project Context

- The existing assembler is implemented in C++.
- The assembler already outputs object records such as `H`, `T`, `M`, and `E`.
- The loader should be implemented as a separate module first, without changing
  the assembler core unless integration becomes necessary.

## Proposed Files

- `linking_loader.h`
- `linking_loader.cpp`
- Optional: `loader_main.cpp`
- Optional: `loader_tests/` for object program examples

## Core Responsibilities

### Pass 1: Build ESTAB

- Read each input object program.
- Process `H` records to identify each control section.
- Process `D` records to collect external symbol definitions.
- Build an External Symbol Table (`ESTAB`).
- Detect duplicate control section names or duplicate external symbols.
- Track:
  - `PROGADDR`: loader start address
  - `CSADDR`: current control section load address
  - `CSLTH`: current control section length

### Pass 2: Load And Relocate

- Process `T` records and write bytes into simulated memory.
- Process `M` records and apply relocation or external symbol modification.
- Process `E` records and determine the execution start address.
- Detect undefined external symbols.
- Detect invalid records and malformed fields.

## Main Data Structures

```cpp
struct ControlSection {
    std::string name;
    int startAddress;
    int length;
};

struct ExternalSymbol {
    std::string name;
    int address;
    std::string controlSection;
};
```

Suggested containers:

- `std::unordered_map<std::string, ExternalSymbol> estab`
- `std::vector<ControlSection> controlSections`
- `std::unordered_map<int, unsigned char> memory`

## Supported Object Records

### Phase 1

- `H`: Header record
- `D`: Define record
- `T`: Text record
- `M`: Modification record with symbol names, such as `+RDREC` or `-LISTA`
- `E`: End record

### Phase 2

- `R`: Reference record
- `M` records that use reference numbers instead of symbol names

## Output

### Loading Map

Example:

```text
Control Section   Symbol   Address   Length
COPY                       004000    001077
                  RDREC    005036
                  WRREC    005061
```

### Memory Dump

Example:

```text
Address  Bytes
004000   14 10 33 48 20 39 00 10
004008   36 28 10 30 30 10 15 48
```

## Implementation Stages

### Stage 1: Object Record Parser

- Parse fixed-width SIC/XE object records.
- Validate record type and field lengths.
- Convert hexadecimal fields to integers.
- Preserve enough source context for useful error messages.

Status: Completed

Implemented in `linking_loader.cpp`. The parser currently supports `H`, `D`,
`R`, `T`, `M`, and `E` records. `R` records are recognized for compatibility,
but reference-number based modification is deferred.

### Stage 2: Pass 1 And ESTAB

- Implement `LinkingLoader::pass1`.
- Parse `H` and `D` records.
- Build the external symbol table.
- Generate a loading map.
- Detect duplicates.

Status: Completed

Implemented in `LinkingLoader::pass1`. The loader builds `ESTAB`, detects
duplicate control sections and external symbols, and can write a deterministic
loading map.

### Stage 3: Memory Loading

- Implement memory storage.
- Load `T` records into memory at `CSADDR + recordAddress`.
- Detect invalid hex text and overlapping writes.

Status: Completed

Implemented in `LinkingLoader::pass2` and `loadTextRecord`. Text records are
loaded into simulated memory and overlapping writes are rejected.

### Stage 4: Modification Records

- Implement half-byte field extraction and write-back.
- Apply `+SYMBOL` and `-SYMBOL` expressions from `M` records.
- Support 20-bit format 4 relocation first.
- Extend later for reference-number based modification.

Status: Completed

Implemented in `applyModificationRecord`. The loader supports both current
assembler relocation records such as `M00000705` and external symbol
expressions such as `M00000105+BETA`.

### Stage 5: CLI Integration

Choose one approach:

- Add `loader_main.cpp` and build a separate executable.
- Or extend `main.cpp` with subcommands, for example:
  - `assemble input.asm output.obj`
  - `load --addr 4000 input1.obj input2.obj`

Status: Completed

Implemented as a separate executable entry point in `loader_main.cpp`.

### Stage 6: Tests And Examples

- Test single object program loading.
- Test multiple control sections.
- Test duplicate external symbol detection.
- Test undefined external symbol detection.
- Test modification records.
- Test execution address handling.

Status: Started

Added fixture-based tests under `loader_tests/` and a Windows regression script
in `run_loader_tests.bat`.

## Open Decisions

- Should the loader produce only a text memory dump, or also a binary memory
  image?
  - Current decision: produce a text memory dump first.
- Should CLI integration be a separate executable or a subcommand in the
  existing assembler executable?
  - Current decision: separate executable, `sicxe_loader.exe`.
- Should the first version support `R` records immediately, or defer them until
  symbol-name based `M` records are stable?
  - Current decision: recognize `R` records, but defer reference-number based
    modification.
- What exact object file format should be treated as canonical for this project:
  textbook SIC/XE format, current assembler output, or both?
  - Current decision: support both section-relative SIC/XE records and current
    assembler records whose `H`, `T`, and `E` addresses start at a non-zero
    source address.

## First Milestone

Implement `linking_loader.h` and `linking_loader.cpp` with:

- Basic record parsing
- Pass 1
- `ESTAB`
- Loading map output

This milestone verifies the linker-side symbol model before memory relocation
logic is added.

## Testing Plan

Testing should be added in layers so each loader stage can be verified before
the next stage depends on it.

### Test Directory Layout

Suggested files:

- `loader_tests/single_section.obj`
- `loader_tests/two_sections_main.obj`
- `loader_tests/two_sections_sub.obj`
- `loader_tests/relocation.obj`
- `loader_tests/duplicate_symbol.obj`
- `loader_tests/undefined_symbol.obj`
- `loader_tests/invalid_record.obj`
- `loader_tests/expected/*.map`
- `loader_tests/expected/*.mem`

### Stage 1 Tests: Record Parser

Verify that the parser accepts valid records and rejects malformed records.

Cases:

- Valid `H`, `D`, `R`, `T`, `M`, and `E` records.
- Hex fields with uppercase letters.
- Short records.
- Invalid record type.
- Invalid hex characters.
- Odd-length text record object code.
- `T` record length that does not match object code byte count.
- `M` record with missing sign or symbol.

Expected result:

- Valid records produce structured fields.
- Invalid records return clear error messages with file name and line number.

### Stage 2 Tests: Pass 1 And ESTAB

Verify external symbol table construction.

Cases:

- Single control section with no `D` record.
- Single control section with multiple symbols in one `D` record.
- Multiple control sections loaded consecutively.
- Duplicate control section name.
- Duplicate external symbol name.
- Symbol address calculated as `CSADDR + relativeAddress`.

Expected result:

- Loading map matches the expected `.map` file.
- Duplicate names fail before Pass 2 starts.

Example expected map:

```text
Control Section   Symbol   Address   Length
MAIN                       004000    000030
                  ALPHA    004012
SUBR                       004030    000020
                  BETA     004038
```

### Stage 3 Tests: Text Record Loading

Verify memory writes before relocation.

Cases:

- One `T` record.
- Multiple adjacent `T` records.
- Multiple non-adjacent `T` records with gaps.
- `T` records from multiple control sections.
- Overlapping `T` records.
- Text record address outside the current control section length.

Expected result:

- Memory dump matches the expected `.mem` file.
- Gaps are not emitted unless the chosen dump format explicitly shows them.
- Overlapping writes should fail unless overwrite behavior is intentionally
  allowed and documented.

### Stage 4 Tests: Modification Records

Verify relocation and external symbol adjustment.

Cases:

- Format 4 relocation with `M00000705+EXTSYM`.
- Subtraction with `M00001005-EXTSYM`.
- Multiple modifications applied to the same field.
- Undefined external symbol.
- Modification field crossing byte boundaries.
- Negative result handling for signed half-byte fields if supported.
- Reference-number based modification after `R` record support is added.

Expected result:

- Modified memory bytes match expected output.
- Undefined symbols fail with a clear error.
- 20-bit fields preserve unrelated high/low nibbles correctly.

### Stage 5 Tests: Execution Address

Verify handling of `E` records.

Cases:

- First control section has `E000000`.
- Later control section has an `E` record without operand.
- No `E` operand in any section.
- `E` operand points to a valid relative address.

Expected result:

- `EXECADDR` is set to the first explicit execution address.
- If no explicit execution address exists, use `PROGADDR`.

### Stage 6 Tests: CLI Integration

Verify the loader through the command line.

Example commands:

```powershell
.\sicxe_loader.exe --addr 4000 loader_tests\single_section.obj
.\sicxe_loader.exe --addr 4000 loader_tests\two_sections_main.obj loader_tests\two_sections_sub.obj
```

Expected result:

- Exit code `0` for successful loads.
- Non-zero exit code for invalid input.
- Generated loading map and memory dump match expected files.

### End-To-End Tests

Use the current assembler to generate `.obj` files, then load them.

Cases:

- Assemble and load `sum.asm`.
- Assemble and load `xe_test.asm`.
- Assemble and load `Assembler_SIC_2022/textbookexample.asm`.
- Assemble and load `Assembler_SIC_2022/textbooksicxe.asm`.

Expected result:

- Loader accepts object files produced by the current assembler.
- Loaded memory contains all text records at `PROGADDR + objectAddress`.
- Modification records from SIC/XE examples are applied correctly.

### Error Handling Tests

The loader should fail cleanly for:

- Missing input file.
- Empty object file.
- Object file without `H` record.
- `D` record before `H` record.
- `T` record before `H` record.
- `M` record before any loaded text.
- Duplicate external symbol.
- Undefined external symbol.
- Invalid hexadecimal field.
- Control section length mismatch.

Expected result:

- No partial success message.
- Error includes enough context to identify the record and input file.

### Regression Test Script

Add a simple script after CLI behavior is stable:

- `run_loader_tests.bat`
- Optional: `run_loader_tests.sh`

The script should:

- Build the loader.
- Run all success cases.
- Compare generated `.map` and `.mem` files against expected outputs.
- Run failure cases and verify non-zero exit codes.

### Completion Criteria

The loader implementation is considered ready when:

- Parser tests cover every supported record type.
- Pass 1 loading map output is deterministic.
- Pass 2 memory dump output is deterministic.
- All success fixtures match expected output.
- All failure fixtures return non-zero exit codes.
- Current assembler-generated `.obj` files can be loaded.
- At least one test covers multiple control sections and external modification.
