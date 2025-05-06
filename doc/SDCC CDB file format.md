# CDB File Format

This document is based on this [CDB File Format Documentation](https://sourceforge.net/p/sdcc/wiki/CDB%20File%20Format/)

## 1. Conventions

The record examples and grammar shown in this document are displayed on multiple lines for readability. However, the records within the CDB files are always encoded on a single line.

- **Record Elements**: Enclosed with `<` and `>`.
- **Repeating Elements**: Enclosed in `[` and `]`.
- **Comments**: Starting with `//`.

## 2. Record Formats

### 2.1 Basic Record Format

| Type | Description                     | Source           |
| ---- | ------------------------------- | ---------------- |
| `M`  | Module Record                   | Compiler         |
| `F`  | Function Record                 | Compiler         |
| `S`  | Symbol Record                   | Compiler         |
| `T`  | Structure (Complex Type) Record | Compiler         |
| `L`  | Linker Record                   | Assembler/Linker |

```text
<Type>:<Record>
```

#### 2.1.1 Blocks and Levels

The level & block are used to further scope local variables since C allows unique definitions across different scope blocks. When using the symbol records, it is always important to include the level and block information as part of the identification. It is possible to have two symbols that share the same name, but have different scope information.

The linker address records contain not only the name of the symbol, but the Scope information as well, which is instrumental in determining the correct instantiation of the symbol.

Linker C line records also contain the Scope information (see below).

The following code fragment illustrates a simple scope example:

```c
foo()
{
  int c; /* block #1, level #1 */
  {
    int c; /* block #2, level #2 */
    ...
  }
  {
  int c; /* block #3, level #2 */
  ...
  }
}
```

#### 2.1.2 Scopes

| Field           | Description                                          |
| --------------- | ---------------------------------------------------- |
| `Scope`         | `G`: Global scope                                    |
|                 | `F<Filename>`: File scope                            |
|                 | `L<Filename>.<Function>`: Function scope             |
|                 | `S`: Structure (Only in symbol records)              |

### 2.2 Module Record

| Type       | Description                                              |
| ---------- | -------------------------------------------------------- |
| `M`        | Module record type indicator                             |
| `Filename` | The filename of the module represented by this CDB file. |

**Format**:

```text
M:<Filename>
```

**Purpose**: Defines a source module. Usually appears at the beginning of module-specific CDB files and multiple times in the final CDB file produced by the linker. Module-specific scope information is encoded in the respective symbol entries.

**Examples**:

```text
M:Timer0
M:_bp
```

### 2.3 Symbol Record

| Field           | Description                                          |
| --------------- | ---------------------------------------------------- |
| `Scope`         | [Scope](#212-scopes)                                 |
| `Name`          | Symbol name                                          |
| `Level`         | Scope level                                          |
| `Block`         | Scope block                                          |
| `TypeChain`     | [Type chain record](#231-type-chain-record)          |
| `Address Space` | [Address space code](#232-address-space-codes)       |
| `On Stack`      | Indicates if the symbol is on the stack              |
| `Stack`         | Stack offset                                         |
| `Reg`           | If the address space is 'R', this field indicates the register that the symbol is allocated. The register name is in its native form: R1, AX, etc. |

**Format**:

```text
S:<Scope>$<Name>$<Level>$<Block>(<TypeRecord>),<AddressSpace>,<OnStack>,<Stack>,[<Reg>]
```

**Purpose**: Generated for each named symbol in the source file, including local, global, and parameter symbols.

**Examples**:

```text
S:LcheckSerialPort$pstBuffer$1$1({3}DG,STTTinyBuffer:S),R,0,0,[r2,r3,r4]
S:Ltimer0LoadExtended$count$1$1({2}SI:S),B,1,-4
```

#### 2.3.1 Type Chain Record

| Field       | Description                               |
| ----------- | ----------------------------------------- |
| `<Size>`    | Item size in decimal                      |
| `<DCLType>` | [Encoded type](#2311-dcl-types)           |
| `<Sign>`    | Encoded as `U` (unsigned) or `S` (signed) |

**Format**:

```text
{<Size>}[<DLCType>]:<Sign>
```

#### 2.3.1.1 DCL Types

| Code | Description          |
| ---- | -------------------- |
| `DA<n>d` | Array of `n` elements  |
| `DF` | Function             |
| `DG` | Generic pointer      |
| `DC` | Code pointer         |
| `DD` | Internal RAM pointer |
| `SL` | Long                 |
| `SI` | Integer              |
| `SC` | Character            |
| `SV` | Void                 |
| `SF` | Float                |
| `ST<name>` | Structure of name `<name>`|
| `SX` | SBit                 |
| `SB<offset>$<n>` | Bit field of `n` bits, `offset` bits from the start of the byte |

**Examples**:

```text
{3}DG,STTinyBuffer:S - TinyBuffer*
{2}SI:S - signed int
{2}DF,DG,DG,SL:S -long** foo(...){}
{500}DA25d,DA10d,DG,SI:S - int* x[25][10]
```

#### 2.3.2 Address Space Codes

The address space field in symbol records specifies the memory location or segment where the symbol resides. The possible codes and their meanings are:

| Code | Address Space                |
|------|----------------------------- |
| `A`  | External Stack               |
| `B`  | Internal Stack               |
| `C`  | Code                         |
| `D`  | Code/Static Segment          |
| `E`  | Internal RAM (Lower 128)     |
| `F`  | External RAM                 |
| `G`  | Internal RAM                 |
| `H`  | Bit Addressable              |
| `I`  | SFR Space                    |
| `J`  | SBIT Space                   |
| `R`  | Register Space               |
| `Z`  | Undefined/Function Records   |

These codes are included in the `<AddressSpace>` field of a symbol record, providing clarity on the storage location of each symbol.

### 2.4 Function Records

| Field             | Description                                       |
| ----------------- | ------------------------------------------------- |
| `Scope`           | [Scope](#212-scopes)                              |
| `Name`            | Symbol name                                       |
| `Level`           | Scope level                                       |
| `Block`           | Scope block                                       |
| `TypeChain`       | [Type chain record](#231-type-chain-record)       |
| `Address Space`   | [Address space code](#232-address-space-codes)    |
| `On Stack`        | Indicates if the symbol is on the stack           |
| `Stack`           | Stack offset                                      |
| `Interrupt`       | Indicates if the function is an interrupt handler |
| `Interrupt Num`   | Interrupt number                                  |
| `Register Bank`   | Register bank number                              |

**Format**:

```text
F:<Scope>$<Name>$<Level>$<Block>(<TypeRecord>),<AddressSpace>,<OnStack>,<Stack>,<Interrupt>,<Interrupt Num>,<Register Bank>
```

**Examples**:

```text
F:G$main$0$0({2}DF,SV:S),C,0,0,0,0,0
F:G$SioISR$0$0({2}DF,SV:S),Z,0,0,1,4,0
```

### 2.6 Type Records

| Field          | Description                              |
| -------------- | ---------------------------------------- |
| `Scope`        | [Scope](#212-scopes) of the type         |
| `Name`         | The name of this type                    |
| `TypeMember`   | Detailed type description (see below)    |

**Format**:

```text
T:<Scope>$<Name>[<TypeMember>]
```

#### 2.4.1 Type Member

| Field          | Description                                     |
| -------------- | ----------------------------------------------- |
| `Offset`       | The offset of this type member in decimal       |
| `SymbolRecord` | Complete [symbol records](#23-symbol-record) each decribing one member |

**Format**:

```text
({<Offset>}<SymbolRecord>)
```

**Example**:

C code:

```c
typedef struct TinyBuffer
{
  TinyBuffer* pNext;
  unsigned char length;
};
```

Type record:

```text
T:G$TTinyBuffer[({0}S:S$pNext$0$0({3}DG,STTTinyBuffer:S),Z,0,0)({3}S:S$length$0$0({1}SC:U),Z,0,0)]
```

### 2.5 Link Record

| Field             | Description                                       |
| ----------------- | ------------------------------------------------- |
| `Scope`           | [Scope](#212-scopes)                              |
| `Filename`        | Filename (Assembly files without extension)|
| `Name`            | Symbol name                                       |
| `Level`           | Scope level                                       |
| `Block`           | Scope block                                       |
| `Line`            | Line number. Starts at 1 (not 0).                 |
| `Address`         | The address of the symbol in HEX, relative to the address space code |
| `EndAddress`      | End Address, in HEX                               |

#### 2.5.1 Link Address of Symbol

**Format**:

```text
L:<Scope>$<Name>$<Level>$<Block>:<Address>
```

**Examples**:

```text
L:G$P0$0$0:80 
L:G$ScanCount$0$0:0 
L:Fcmdas$_str_0$0$0:195
```

#### 2.5.2 Linker Symbol End Address Record

**Format**:

```text
L:X<Scope>$<Name>$<Level>$<Block>:<EndAddress>
```

**Examples**:

```text
L:XG$sysClearError$0$0:194 
L:XG$SioISR$0$0:A09
```

**Purpose**:

The Linker Symbol end address record is primarily used to indicate the Ending address of functions. This is because function records do not contain a size value, as symbol records do.

### 2.5.3 Linker ASM Line Record

**Format**:

```text
L:A$<Filename>$<Line>:<EndAddress>
```

**Purpose**:

The linker Asm Line record is used to bind the execution address with a source file and line number.

**Example**:

```text
L:A$TinyBuffer$2320:A13 
L:A$max1270$391:CA4
```

### 2.5.4 Linker C-Line Record

**Format**:

```text
L:C$<Filename>$<Line>$<Level>$<Block>:<EndAddress>
```

**Example**:

```text
L:C$max1270.c$35$1$1:CA9 
L:C$Timer0.c$20$1$1:D9D
```
