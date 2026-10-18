// Log every uGUIBase open (+0x1fb79b0) and close (+0x1fb55e0) with the GUI's DTI name.
const mhw = Process.getModuleByName('MonsterHunterWorld.exe');
const B = mhw.base;
const t0 = Date.now();
const ts = () => ((Date.now() - t0) / 1000).toFixed(1);
const names = new Map();
function dtiName(p) {
  const k = p.toString();
  if (names.has(k)) return names.get(k);
  let n = '?';
  try {
    const f = p.readPointer().add(4 * 8).readPointer();
    if (f.readU8() === 0x48 && f.add(1).readU8() === 0x8d && f.add(2).readU8() === 0x05) {
      n = f.add(7).add(f.add(3).readS32()).add(8).readPointer().readCString();
    } else n = 'vt+0x' + p.readPointer().sub(B).toString(16);
  } catch (e) {}
  names.set(k, n);
  return n;
}
const counts = {};
function log(kind, p) {
  const n = dtiName(p);
  const key = kind + n;
  counts[key] = (counts[key] || 0) + 1;
  if (counts[key] > 20) return;
  console.log(`${ts()} ${kind} ${n} ${p} flags=0x${p.add(0x2298).readU32().toString(16)}`);
}
Interceptor.attach(B.add(0x1fb79b0), { onEnter(args) { log('OPEN ', args[0]); } });
Interceptor.attach(B.add(0x1fb55e0), { onEnter(args) { log('CLOSE', args[0]); } });
console.log('attached');
