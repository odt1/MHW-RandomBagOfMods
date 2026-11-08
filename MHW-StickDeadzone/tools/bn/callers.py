# Body for bnctl exec: prints code refs (callers) of each function RVA in ARGS (prepend "ARGS = [...]").
B = 0x140000000
for r in ARGS:
    f = bv.get_function_at(B + r)
    refs = bv.get_code_refs(B + r)
    print(f'{hex(r)} {f.name if f else "?"}:')
    for x in refs:
        fn = x.function
        print(f'   {hex(x.address - B)} in {fn.name if fn else "?"} {hex(fn.start - B) if fn else ""}')
