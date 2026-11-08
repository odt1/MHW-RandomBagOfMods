# Body for bnctl exec: prints every instruction in .text whose memory operand uses one of the
# displacements in ARGS (prepend "ARGS = [...]"), with the containing function. Finds readers and
# writers of a field when the object's pointer comes from many places. Driven by run_disp.sh.
import struct
B = 0x140000000
text = [s for s in bv.sections.values() if s.name == '.text'][0]
data = bv.read(text.start, text.end - text.start)
for displacement in ARGS:
    pattern = struct.pack('<I', displacement)
    i = data.find(pattern)
    while i != -1:
        hit = text.start + i
        for back in range(3, 9):
            start = hit - back
            line = bv.get_disassembly(start)
            if line and (hex(displacement) + ']') in line and bv.get_instruction_length(start) == back + 4:
                functions = bv.get_functions_containing(start)
                owner = hex(functions[0].start - B) if functions else '?'
                print(hex(displacement), hex(start - B), owner, line)
                break
        i = data.find(pattern, i + 1)
