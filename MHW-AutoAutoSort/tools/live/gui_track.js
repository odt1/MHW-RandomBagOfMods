// Track uGUIBase instances (ctor +0x1fb3d10, base dtor +0x1fb4210); log create/destroy and changes of flags at +0x2298.
const mhw = Process.getModuleByName('MonsterHunterWorld.exe');
const B = mhw.base;
const live = new Map();   // ptr string -> {p, name, flags}
let t0 = Date.now();
const ts = () => ((Date.now() - t0) / 1000).toFixed(1);
function dtiName(p) {
  try {
    const vt = p.readPointer();
    const f = vt.add(4 * 8).readPointer();
    // lea rax, [rip+disp32]; ret
    if (f.readU8() === 0x48 && f.add(1).readU8() === 0x8d && f.add(2).readU8() === 0x05) {
      const dti = f.add(7).add(f.add(3).readS32());
      return dti.add(8).readPointer().readCString();
    }
    return 'vt+0x' + vt.sub(B).toString(16);
  } catch (e) { return '?'; }
}
Interceptor.attach(B.add(0x1fb3d10), {
  onEnter(args) { this.p = args[0]; },
  onLeave() { live.set(this.p.toString(), { p: this.p, name: null, flags: null, born: ts() }); }
});
Interceptor.attach(B.add(0x1fb4210), {
  onEnter(args) {
    const k = args[0].toString();
    const e = live.get(k);
    if (e) { console.log(`${ts()} destroy ${e.name || dtiName(e.p)} ${k}`); live.delete(k); }
  }
});
// also adopt instances that existed before attach when they reach these per-instance methods
for (const f of [0x1fb4540, 0x1e77fb0, 0x10c2dc0]) {
  Interceptor.attach(B.add(f), { onEnter(args) {
    const k = args[0].toString();
    if (!live.has(k)) { live.set(k, { p: args[0], name: null, flags: null, born: 'pre' }); console.log(`${ts()} adopt via +0x${f.toString(16)} ${k}`); }
  } });
}
let first = true;
setInterval(() => {
  for (const [k, e] of live) {
    if (!e.name) { e.name = dtiName(e.p); if (!first) console.log(`${ts()} create ${e.name} ${k}`); }
    let f; try { f = e.p.add(0x2298).readU32(); } catch (x) { continue; }
    const open = (f & 0x80000011) !== 0;
    if (e.flags === null) { e.flags = f; e.open = open; continue; }
    if (open !== e.open) console.log(`${ts()} ${open ? 'OPEN ' : 'CLOSE'} ${e.name} flags 0x${e.flags.toString(16)} -> 0x${f.toString(16)}`);
    e.flags = f; e.open = open;
  }
  if (first) { console.log(`${ts()} tracking ${live.size} GUIs created since attach`); first = false; }
}, 250);
console.log('attached');
