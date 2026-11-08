# Body for bnctl exec: prints the DTI object and vtable of each MT Framework class name in ARGS
# (prepend "ARGS = [...]"). The class's getDTI (vtable slot 4) is `lea rax, [rel dti]; ret`; the DTI
# object is found next to the name string's reference in its static constructor. Driven by run_dti.sh.
B = 0x140000000
def vtables(name):
    strings = [s for s in bv.get_strings() if s.value == name]
    if not strings:
        return 'no such string'
    found = set()
    for ref in bv.get_code_refs(strings[0].start):
        f = ref.function
        if f is None:
            continue
        for ins, addr in f.instructions:
            text = ''.join(str(t) for t in ins)
            if abs(addr - ref.address) < 0x20 and text.startswith('lea     rcx, [rel'):
                dti = int(text.split('[rel ')[1].split(']')[0], 16)
                for getter_ref in bv.get_code_refs(dti):
                    getter = getter_ref.function
                    if getter is not None and getter.total_bytes <= 8:
                        for slot in bv.get_data_refs(getter.start):
                            found.add((dti - B, slot - 4 * 8 - B))
    return ', '.join(f'dti {hex(d)} vtable {hex(v)}' for d, v in sorted(found)) or 'no getDTI found'
for name in ARGS:
    print(name, '->', vtables(name))
