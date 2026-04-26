import sys

import sic
import sicasmparser

import objfile

# 處理 BYTE C'...' 格式：將字元轉換為對應的 16 進位字串
def processBYTEC(operand):
    constant = ""
    for i in range(2, len(operand)-1):
        tmp = hex(ord(operand[i])) # 取得字元的 ASCII 碼並轉為 16 進位
        tmp = tmp[2:] # 去除 '0x' 前綴
        if len(tmp) == 1:
            tmp = "0" + tmp # 補足兩位數
        tmp = tmp.upper()
        constant += tmp
    return constant

# 生成指令的機器碼：結合 Opcode 與運算元位址
def generateInstruction(opcode, operand, SYMTAB):
    # 將 Opcode 左移 16 位元 (佔據指令的高位位元組)
    instruction = int(sic.OPTAB[opcode] * 65536)
    if operand != None:
        # 處理索引定址 (,X)：將第 15 位元 (X flag) 設為 1
        if operand[len(operand)-2:] == ',X':
            instruction += 32768 # 2^15 = 32768
            operand = operand[:len(operand)-2]
        
        # 從符號表中尋找標籤對應的位址
        if operand in SYMTAB:
            instruction += int(SYMTAB[operand])
        else:
            return "" # 找不到符號則回傳空字串 (錯誤狀況)
    return objfile.hexstrToWord(hex(instruction))


# 程式入口：處理命令行參數並讀取檔案
if len(sys.argv) != 2:
    print("用法: python3 assembler.py <source file>")
    sys.exit()
    
lines = sicasmparser.readfile(sys.argv[1]) # 讀取所有原始碼行

SYMTAB = {} # 初始化符號表

# --- 第一階段 (PASS 1)：建立符號表並計算程式長度 ---
for line in lines:
    t = sicasmparser.decompositLine(line) # 解析行內容 (Label, Mnemonic, Operand)

    if t == (None, None, None):
        continue
    
    # 處理 START：設定起始位址
    if t[1] == "START":
        STARTING = int(t[2], 16)
        LOCCTR = int(STARTING)
    
    # 處理 END：計算長度並結束第一階段
    if t[1] == "END":
        proglen = int(LOCCTR - STARTING)
        break
    
    # 記錄標籤 (Label) 到符號表中
    if t[0] != None:
        if t[0] in SYMTAB:
            print("錯誤：標籤 [%s] 重複定義。" % t[0])
            continue
        SYMTAB[t[0]] = LOCCTR
    
    # 根據指令或指示字更新位址計數器 (LOCCTR)
    if sic.isInstruction(t[1]) == True:
        LOCCTR = LOCCTR + 3 # 標準指令佔 3 bytes
    elif t[1] == "WORD":
        LOCCTR = LOCCTR + 3 # WORD 佔 3 bytes
    elif t[1] == "RESW":
        LOCCTR = LOCCTR + (int(t[2])*3) # 預留 WORD 空間
    elif t[1] == "RESB":
        LOCCTR = LOCCTR + int(t[2]) # 預留 BYTE 空間
    elif t[1] == "BYTE":
        # 根據文字或 16 進位格式計算長度
        if t[2][0] == 'C':
            LOCCTR = LOCCTR + (len(t[2]) - 3)
        if t[2][0] == 'X':
            LOCCTR = LOCCTR + ((len(t[2]) - 3)/2)
        

print(SYMTAB)

# --- 第二階段 (PASS 2)：生成機器碼並寫入物件檔 ---

reserveflag = False # 用於標記是否遇到 RESB/RESW (需切斷 T record)

t = sicasmparser.decompositLine(lines[0])
    
file = objfile.openFile(sys.argv[1]) # 開啟輸出檔案

LOCCTR = 0
if t[1] == "START":
    LOCCTR = int(t[2], 16)
    progname = t[0]
STARTING = LOCCTR

# 寫入物件檔的 Header (H record)
objfile.writeHeader(file, progname, STARTING, proglen)

tline = "" # 當前 Text record 的機器碼內容
tstart = LOCCTR # 當前 Text record 的起始位址

for line in lines:
    t = sicasmparser.decompositLine(line)

    if t == (None, None, None):
        continue

    
    if t[1] == "START":
        continue

    # 處理 END：寫入最後的 Text record 與 End record
    if t[1] == "END":
        if len(tline) > 0:
            objfile.writeText(file, tstart, tline)
            
        PROGLEN = LOCCTR - STARTING

        address = STARTING
        if t[2] != None:
            address = SYMTAB[t[2]] # 取得起始執行位址
            
        objfile.writeEnd(file, address)
        break

                    
    if t[1] in sic.OPTAB:

        instruction = generateInstruction(t[1], t[2], SYMTAB)
        
        if len(instruction) == 0:
            print("Undefined Symbole: %s" % t[2])
            break

        if (LOCCTR + 3 - tstart > 30) or (reserveflag == True):
            objfile.writeText(file, tstart, tline)
            tstart = LOCCTR
            tline = instruction
        else:
            tline += instruction

        reserveflag = False

        LOCCTR += 3
            
    elif t[1] == "WORD":

        constant = objfile.hexstrToWord(hex(int(t[2])))

        if (LOCCTR + 3 - tstart > 30) or (reserveflag == True):
            objfile.writeText(file, tstart, tline)
            tstart = LOCCTR
            tline = constant
        else:
            tline += constant
        
        reserveflag = False

        LOCCTR += 3
            
    elif t[1] == "BYTE":

        if t[2][0] == 'X':
            operandlen = int((len(t[2]) - 3)/2)
            constant = t[2][2:len(t[2])-1]
        elif t[2][0] == 'C':
            operandlen = int(len(t[2]) - 3)
            constant = processBYTEC(t[2])
            
        if (LOCCTR + 3 - tstart > 30) or (reserveflag == True):
            objfile.writeText(file, tstart, tline)
            tstart = LOCCTR
            tline = constant
        else:
            tline += constant

        reserveflag = False

        LOCCTR += operandlen
            
    elif t[1] == "RESB":
        LOCCTR += int(t[2])
        reserveflag = True
    elif t[1] == "RESW":
        LOCCTR += (int(t[2]) * 3)
        reserveflag = True
    else:
        print("Invalid Instruction / Invalid Directive")
        