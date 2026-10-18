// Superseded probe kept for reference: lists runs of 16-byte item slot objects (vtable +0x2F206B0) in the user block.
const mhw = Process.getModuleByName('MonsterHunterWorld.exe');
const singleton = mhw.base.add(0x5013950).readPointer();
const slot = singleton.add(0xA0).readU32();
const user = singleton.add(0xA8).readPointer().add(slot * 0x26CC00);
const vt = mhw.base.add(0x2F206B0);
const buf = new Uint32Array(user.readByteArray(0x26CC00));
const lo = vt.and(0xffffffff).toUInt32(), hi = vt.shr(32).toUInt32();
let runs = [], start = -1, prev = -100;
for (let i = 0; i + 1 < buf.length; i += 2) {
  if (buf[i] === lo && buf[i+1] === hi) {
    const off = i*4;
    if (off - prev !== 16) { if (start >= 0) runs.push([start, prev]); start = off; }
    prev = off;
  }
}
if (start >= 0) runs.push([start, prev]);
for (const [a, b] of runs) {
  const n = (b - a) / 16 + 1;
  const ids = [];
  for (let k = 0; k < n; k++) { const id = buf[(a + k*16 + 8)/4], c = buf[(a + k*16 + 12)/4]; if (id) ids.push(`${k}:${id}x${c}`); }
  console.log(`run +0x${a.toString(16)} n=${n} used=${ids.length} ${ids.slice(0, 40).join(' ')}`);
}
