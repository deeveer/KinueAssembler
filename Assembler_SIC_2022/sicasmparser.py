import sic

def readfile(srcfile):
    try:
        with open(srcfile, "r") as fp:
            return fp.readlines()
    except:
            return None

def decompositLine(line):

    if len(line) > 0:
        if line[0] == '.':
            return (None, None, None)
        if line[0] == '\n':
            return (None, None, None)

    tokens = line.split()
    if len(tokens) == 1:
        if isOpcodeOrDirective(tokens[0]) == False:
            print("Your assembly code has problem.")
            return (None, None, None)
        return (None, tokens[0], None)
    elif len(tokens) == 2:
        if isOpcodeOrDirective(tokens[0]) == True:
            return (None, tokens[0], tokens[1])
        elif isOpcodeOrDirective(tokens[1]) == True:
            return (tokens[0], tokens[1], None)
        else:
            print("Your assembly code has problem.")
            return (None, None, None)
    elif len(tokens) == 3:
        if isOpcodeOrDirective(tokens[1]) == True:
            return (tokens[0], tokens[1], tokens[2])
        else:
            print("Your assembly code has problem.")
            return (None, None, None)
    return (None, None, None)
    
def isOpcodeOrDirective(token):
    if sic.isInstruction(token) == True:
        return True
    if sic.isDirective(token) == True:
        return True
    return False
    
    
