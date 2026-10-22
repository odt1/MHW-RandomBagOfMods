# Body for bnctl exec: prints HLIL of the functions containing each RVA in ARGS (prepend "ARGS = [...]").
# ML=<lines> limits output per function. Driven by run_hlil.sh.
import sys
B = 0x140000000
def show(rva, maxlines=int(__import__("os").environ.get("ML","400"))):
    f = bv.get_functions_containing(B + rva)[0]
    print(f'==== {f.name} rva {hex(f.start - B)} size {f.total_bytes}')
    lines = [f'{hex(l.address - B) if l.address else ""}  {l}' for l in f.hlil.root.lines] if f.hlil else []
    for l in lines[:maxlines]: print(l)
    if len(lines) > maxlines: print(f'... {len(lines)} lines')
for r in ARGS:
    show(r)
