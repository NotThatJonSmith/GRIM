
import sys, re

dump_file = open(sys.argv[1])
addr2func = {}
encoded_instructions = {}
source_instructions = {}

trace = sys.stdin
if len(sys.argv) == 3:
    trace = open(sys.argv[2])

for line in dump_file:

    m = re.match(r"(\S+)\s+\<(.*)\>", line)

    if m:
        addr = int(m.group(1), 16)
        func_name = m.group(2)
        addr2func[addr] = func_name

    m = re.match(r"\s*([A-Fa-f0-9]+):\s+([A-Fa-f0-9]+)(.*)", line)

    if m:
        addr = int(m.group(1), 16)
        encoded_instructions[addr] = m.group(2).strip()
        source_instructions[addr] = m.group(3).strip()

last_func = None
for line in trace:

    line = line.strip()
    m = re.match(r"([0-9a-f]+):.*", line)
    if not m:
        print(line)
        continue

    addr = int(m.group(1), 16)

    if addr in addr2func.keys():
        # Hey, we're at a function's normal entry
        fname = addr2func[addr]
        print("Calling function "+fname)
        last_func = fname
    else:
        # Find the function we're in
        func_name = None
        min_idx = None
        for func_addr in addr2func.keys():
            idx = addr - func_addr
            if idx < 0 or (min_idx != None and idx >= min_idx):
                continue
            min_idx = idx
            func_name = addr2func[func_addr]
        if func_name != last_func:
            print("Returning to function {}".format(func_name))
            last_func = func_name

    print(line,end="")
    if addr in encoded_instructions.keys():
        print("\t\tfrom source: "+
              encoded_instructions[addr]+
            "\t"+source_instructions[addr],end="")
    else:
        print("\t\t from unknown source disasm",end="")

    print()
    
