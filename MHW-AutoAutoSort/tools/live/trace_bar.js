// Log item bar writers, pouch sort calls, and writes into pouch slot objects, each with a fuzzy backtrace.
const FN = [0x5928d0,0xae7d60,0x13447f0,0x134ae90,0x134fa30,0x134fee0,0x13504c0,0x1350d30,0x1351920,0x1351990,0x135bdc0,0x135be80,0x135d110,0x1361d90,0x1367840,0x1367890,0x1368690,0x13687f0,0x13688b0,0x1368900,0x13689b0,0x1368b30,0x136bef0,0x1b6ce80,0x135d610,0x134ab90];
const SLOT_SETTERS = [0x11cea20]; // 0x1187e50 (set id) is 4 bytes, too short to hook
const mhw = Process.getModuleByName('MonsterHunterWorld.exe');
const B = mhw.base;
const singleton = B.add(0x5013950).readPointer();
const user = singleton.add(0xA8).readPointer().add(singleton.add(0xA0).readU32() * 0x26CC00);
const P0 = user.add(0x38080), P1 = user.add(0x38550);
const rva = p => { const d = p.sub(B); return d.compare(0) >= 0 && d.compare(mhw.size) < 0 ? '+0x' + d.toString(16) : p.toString(); };
let t = 0; const baseline = new Set(); const shown = {};
for (const f of FN) {
  shown[f] = 0;
  Interceptor.attach(B.add(f), {
    onEnter(args) {
      if (t <= 4) { baseline.add(f); return; }
      if (baseline.has(f) || shown[f] >= 6) return;
      shown[f]++;
      const bt = Thread.backtrace(this.context, Backtracer.FUZZY).slice(0, 8).map(rva).join(' ');
      console.log(`t=${t} +0x${f.toString(16)} tid=${this.threadId} a2=${args[1]} a3=${args[2]} bt: ${bt}`);
    }
  });
}
const seenCaller = {};
for (const f of SLOT_SETTERS) {
  Interceptor.attach(B.add(f), {
    onEnter(args) {
      const p = args[0];
      if (p.compare(P0) < 0 || p.compare(P1) >= 0) return;
      const key = f + ':' + this.returnAddress;
      seenCaller[key] = (seenCaller[key] || 0) + 1;
      if (seenCaller[key] > 2) return;
      const idx = p.sub(P0).toInt32() / 16;
      const bt = Thread.backtrace(this.context, Backtracer.FUZZY).slice(0, 8).map(rva).join(' ');
      console.log(`t=${t} slot+0x${f.toString(16)} idx=${idx} val=${args[1].toInt32()} tid=${this.threadId} bt: ${bt}`);
    }
  });
}
setInterval(() => { t++; if (t === 5) console.log(`baseline: ${[...baseline].map(f => '+0x' + f.toString(16)).join(' ')}`); }, 1000);
