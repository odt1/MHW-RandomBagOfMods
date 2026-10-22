const base = Process.getModuleByName('MonsterHunterWorld.exe').base;
const name = o => { try { const vt = o.readPointer(); const g = vt.add(32).readPointer(); if (g.readU16() !== 0x8d48) return 'vt+0x' + vt.sub(base).toString(16) + ' nolea'; const dti = g.add(7).add(g.add(3).readS32()); return dti.add(8).readPointer().readCString() + ' vt+0x' + vt.sub(base).toString(16); } catch (e) { return 'err'; } };
let done = false;
const l = Interceptor.attach(base.add(0x11a9c50), { onEnter(a) { if (done) return; done = true; const o = a[0];
  console.log('ctl ' + o + ' ' + name(o) + ' +0x30..3c ' + [0x30,0x34,0x38,0x3c].map(x => o.add(x).readFloat().toFixed(3)).join(','));
  for (const off of [0x8, 0x10, 0x18, 0x20, 0x28, 0xdb0]) { try { const p = o.add(off).readPointer(); console.log('  +0x' + off.toString(16) + ' ' + p + ' ' + name(p)); } catch (e) {} }
  globalThis.ctl = o; } });
setTimeout(() => { l.detach(); }, 1000);
