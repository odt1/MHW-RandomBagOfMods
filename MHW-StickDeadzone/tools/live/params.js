const base = Process.getModuleByName('MonsterHunterWorld.exe').base;
const p = base.add(0x5011f58).readPointer();
const out = { param: p.toString() };
if (!p.isNull()) {
  for (let o = 0x120; o <= 0x160; o += 4) out['+0x' + o.toString(16)] = p.add(o).readFloat();
  try { const vt = p.readPointer(); out.vt = '+0x' + vt.sub(base).toString(16); } catch (e) {}
}
const sPad = base.add(0x51c4558).readPointer();
out.sPad = sPad.toString();
if (!sPad.isNull()) {
  const pad = sPad.add(0x38 + sPad.add(0x1518).readU32() * 0x2d8);
  out.padType = pad.add(0xb).readU8(); out.stickBtnThreshold = pad.add(0x1a).readS16(); out.trigThreshold = pad.add(0x19).readU8();
}
console.log(JSON.stringify(out, null, 1));
