const base = Process.getModuleByName('MonsterHunterWorld.exe').base;
const name = o => { try { const vt = o.readPointer(); if (vt.compare(base) < 0 || vt.compare(base.add(0x6000000)) > 0) return null; const g = vt.add(32).readPointer(); if (g.readU16() !== 0x8d48 || g.add(2).readU8() !== 0x05) return null; const dti = g.add(7).add(g.add(3).readS32()); return dti.add(8).readPointer().readCString(); } catch (e) { return null; } };
const out = [];
for (let off = 0x5010000; off < 0x5240000; off += 8) {
  let p; try { p = base.add(off).readPointer(); } catch (e) { continue; }
  if (p.isNull() || p.compare(base.add(0x6000000)) < 0 && p.compare(base) >= 0) continue;
  const n = name(p);
  if (n && /Camera|Pad|Input|sMh|Controller/.test(n)) out.push('+0x' + off.toString(16) + ' ' + n + ' ' + p);
}
console.log(out.join('\n'));
