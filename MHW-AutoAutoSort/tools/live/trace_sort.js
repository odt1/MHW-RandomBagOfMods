// Count calls to every function that references the pouch offset; report the ones that start firing after a quiet baseline.
const FN = [0x483890,0x487200,0x492910,0x557960,0x5587e0,0x559180,0x63d5c0,0x9f0be0,0x9f1a10,0xacf290,0xafa570,0xbd29c0,0xfa70a0,0x1206fd0,0x1342c90,0x1344350,0x13447f0,0x134ab90,0x134c7e0,0x134fee0,0x1350d30,0x13510b0,0x1355970,0x1356830,0x1356c90,0x1357160,0x1358e60,0x135d610,0x1361d90,0x136bef0,0x1977690,0x1a57930,0x1ace180,0x1ace580,0x1ace970,0x1b59e30,0x1b59ec0,0x1b59fe0,0x1b5a070,0x1b5b470,0x1bb5850,0x1ccd810,0x1d924f0,0x1dec910,0x1e49060,0x1e87f80,0x1ebc0b0,0x1ebdce0,0x1fb32a0,0x270c800,0x2714a70,0x275abd0,0x2769f90,0x2769fb0,0x2771250,0x27f34b0,0x29aa450];
const mhw = Process.getModuleByName('MonsterHunterWorld.exe');
const B = mhw.base;
const counts = {}, callers = {};
const baseline = new Set();
let t = 0;
for (const f of FN) {
  counts[f] = 0; callers[f] = new Set();
  Interceptor.attach(B.add(f), {
    onEnter() {
      counts[f]++;
      const ra = this.returnAddress.sub(B).toInt32();
      if (callers[f].size < 8) callers[f].add(`${this.threadId}@+0x${ra.toString(16)}`);
    }
  });
}
const last = {};
for (const f of FN) last[f] = 0;
setInterval(() => {
  t++;
  const out = [];
  for (const f of FN) {
    const d = counts[f] - last[f];
    last[f] = counts[f];
    if (!d) continue;
    if (t <= 4) { baseline.add(f); continue; }
    if (baseline.has(f)) continue;
    out.push(`+0x${f.toString(16)} x${d} [${[...callers[f]].join(' ')}]`);
    callers[f].clear();
  }
  if (t === 4) console.log(`baseline done, steady: ${[...baseline].map(f => '+0x' + f.toString(16)).join(' ')}`);
  if (out.length) console.log(`t=${t}s ` + out.join(' | '));
}, 1000);
