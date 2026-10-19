# GMD text parser: python -I gmd.py <file.gmd> <regex> prints index, key, text of matches.
# Item names: item_eng.gmd entry 2*id.
import struct, sys, re
def load(path):
    d = open(path, 'rb').read()
    magic, ver, lang, _, kc, sc, kbs, sbs, nl = struct.unpack_from('<4sIIQIIIII', d, 0)
    off = 0x28 + nl + 1
    infos = [struct.unpack_from('<IIIIQQ', d, off + i * 0x20) for i in range(kc)]
    off += kc * 0x20
    if kc: off += 0x800
    keys = d[off:off + kbs].split(b'\0'); off += kbs
    strs = d[off:off + sbs].split(b'\0')
    out = {}
    for k, inf in enumerate(infos):
        out[inf[0]] = keys[k].decode('utf-8', 'replace')
    return [(i, out.get(i, ''), s.decode('utf-8', 'replace')) for i, s in enumerate(strs[:sc])]
if __name__ == '__main__':
    pat = re.compile(sys.argv[2], re.I)
    for i, k, s in load(sys.argv[1]):
        if pat.search(k) or pat.search(s):
            print(i, k, repr(s[:150]))
