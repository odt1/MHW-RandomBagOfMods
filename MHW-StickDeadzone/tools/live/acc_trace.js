const base = Process.getModuleByName('MonsterHunterWorld.exe').base;
const acc = { Lmag: 0x1b15c90, LX: 0x1b15da0, LY: 0x1b15e70, Rmag: 0x1b15fd0, RX: 0x1b160e0, RY: 0x1b161b0, Thr: 0x19a0800 };
const hits = {};
const ls = [];
for (const [n, r] of Object.entries(acc)) {
  ls.push(Interceptor.attach(base.add(r), {
    onEnter(a) {
      const k = n + ' ctx' + a[1].toInt32() + ' from +0x' + this.returnAddress.sub(base).toString(16);
      hits[k] = (hits[k] || 0) + 1;
    }
  }));
}
setTimeout(() => {
  ls.forEach(l => l.detach());
  Interceptor.flush();
  const keys = Object.keys(hits).sort();
  for (const k of keys) console.log(hits[k] + '\t' + k);
}, 4000);
