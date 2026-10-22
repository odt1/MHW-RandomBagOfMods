const base = Process.getModuleByName('MonsterHunterWorld.exe').base;
for (const g of [0x5011f70, 0x5011f78, 0x5011f80]) {
  const p = base.add(g).readPointer();
  let path = ''; try { path = p.add(0xc).readCString(); } catch (e) {}
  const fl = [];
  for (let o = 0x60; o < 0x300; o += 4) { const f = p.add(o).readFloat(); const i = p.add(o).readU32(); fl.push('+' + o.toString(16) + '=' + (Math.abs(f) > 1e-4 && Math.abs(f) < 1e6 ? +f.toFixed(4) : (i === 0 ? '0' : 'i' + i.toString(16)))); }
  console.log('== +0x' + g.toString(16) + ' ' + p + ' path=' + path + '\n' + fl.join(' '));
}
