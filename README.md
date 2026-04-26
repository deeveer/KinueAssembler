# SIC/XE Assembler

這是一個使用 C++ 實作的兩階段 (Two-Pass) 組譯器，支援標準 SIC 指令集以及 SIC/XE 擴充架構。它能夠將 `.asm` 原始碼轉換為符合教科書標準格式的 `.obj` 物件檔。

## 🚀 功能特色

- **自動模式偵測**：自動區分 SIC 與 SIC/XE 程式，並產生對應的編碼格式。
- **完整架構支援**：
  - 支援 Format 1, 2, 3, 4 指令格式。
  - 實作 PC 相對定址與 Base 相對定址的自動切換 (PC-relative -> Base-relative fallback)。
  - 支援立即定址 (`#`)、間接定址 (`@`) 與索引定址 (`,X`)。
- **進階指示字 (Directives)**：支援 `START`, `END`, `BYTE`, `WORD`, `RESB`, `RESW`, `EQU`, `ORG`, `BASE`, `NOBASE`, `LTORG`。
- **Literal 處理**：完整實作 Literal Table，支援 `LTORG` 與程式結尾的 Literal Pool 分配。
- **偵錯友善**：自動產生 `intermediate.txt`，列出每行指令的記憶體位址分配情況。

## 📂 專案結構

- `main.cpp`: 程式進入點。
- `assembler.cpp` / `.h`: 組譯器核心邏輯 (Pass 1 & Pass 2)。
- `sicxe_common.cpp` / `.h`: 定義指令表 (OPTAB)、符號表 (SYMTAB) 與基礎資料結構。
- `run.bat`: Windows 專用的自動編譯與執行腳本。
- `run.sh`: macOS/Linux 專用的自動編譯與執行腳本。

## 🛠️ 安裝與環境準備

本專案需要 **C++ 編譯器 (g++)**。請確保您的系統已安裝 `g++` 並已加入環境變數。

### 驗證編譯器
```bash
g++ --version
```
## 🧠 [完整系統建構指令 (AI Prompt Specification)](./SYSTEM_SPEC.md)
## 📖 使用教學

我們提供了便捷的腳本來自動處理編譯與組譯過程：

### Windows (PowerShell/CMD)
```powershell
.\run.bat <source_file.asm> [output_file.obj]
```
*範例：* `.\run.bat Assembler_SIC_2022\textbookexample.asm`

### macOS / Linux
```bash
chmod +x run.sh
./run.sh <source_file.asm> [output_file.obj]
```
*範例：* `./run.sh Assembler_SIC_2022/textbooksicxe.asm`

## 📄 輸出說明

組譯成功後會產生兩個主要檔案：
1.  **`.obj` 檔**：機器碼輸出，包含 Header (H)、Text (T)、Modification (M) 及 End (E) 紀錄。
2.  **`intermediate.txt`**：中間過程紀錄，方便對照位址與符號表。

## ⚠️ 錯誤處理

組譯器會偵測並回報以下常見錯誤：
- 重複定義的標籤 (Duplicate label)
- 未定義的符號 (Undefined symbol)
- 定址位移超出範圍 (Displacement out of range)
- 無法識別的助記碼 (Invalid opcode)
