const base = Process.getModuleByName('MonsterHunterWorld.exe').base;
const fn = base.add(0x18d7720);
const want = [0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20];
const got = Array.from(new Uint8Array(fn.readByteArray(10)));
if (got.join() !== want.join()) throw new Error('prologue mismatch ' + got);
const cfgPath = 'C:/Users/ODT/AppData/Local/Temp/claude/C--Projects-MHW-Mods/c2ccb8b2-f2a5-48c6-958f-36be5ea2bd70/scratchpad/dz.json';
let cfg = null, cfgText = '';
function load() {
  try { const t = File.readAllText(cfgPath); if (t !== cfgText) { cfg = JSON.parse(t); cfgText = t; console.log('config ' + JSON.stringify(cfg)); } }
  catch (e) { console.log('config error ' + e.message); }
}
load();
setInterval(load, 1000);
const comp = (v, g) => v === 0 ? 0 : Math.sign(v) * (g + (1 - g) * Math.abs(v));
function remap(p, off, c) {
  const x = p.add(off).readFloat(), y = p.add(off + 4).readFloat();
  const m = Math.hypot(x, y);
  let ox = 0, oy = 0;
  if (m > c.dz) {
    const n = Math.min(1, (Math.min(m, 1) - c.dz) / (1 - c.dz));
    const ux = x / m, uy = y / m;
    if (c.shape === 'radial') { const s = c.g + (1 - c.g) * n; ox = ux * s; oy = uy * s; }
    else if (c.shape === 'axial') { ox = comp(ux * n, c.g); oy = comp(uy * n, c.g); }
    else { ox = ux * n; oy = uy * n; }
  }
  p.add(off).writeFloat(ox); p.add(off + 4).writeFloat(oy);
  return [x, y, ox, oy];
}
let last = null, frames = 0;
Interceptor.attach(fn, {
  onEnter(a) { this.p = a[0]; },
  onLeave() {
    frames++;
    if (!cfg || !cfg.on) return;
    const p = this.p;
    const l = remap(p, 0x18, cfg.L), r = remap(p, 0x20, cfg.R);
    p.add(0x28).writeFloat(Math.hypot(l[2], l[3]));
    p.add(0x2c).writeFloat(Math.hypot(r[2], r[3]));
    last = { l, r };
  }
});
setInterval(() => {
  if (last) console.log('frames ' + frames + ' L ' + last.l.map(v => v.toFixed(3)).join(' ') + ' | R ' + last.r.map(v => v.toFixed(3)).join(' '));
}, 2000);
console.log('hooked +0x18d7720');
