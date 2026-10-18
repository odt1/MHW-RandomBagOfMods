// Read-only: pouch lists, item bar and ammo bar of the selected character.
// frida -q -t 1 -n MonsterHunterWorld.exe -l read_pouch.js
const mhw = Process.getModuleByName('MonsterHunterWorld.exe');
const singleton = mhw.base.add(0x5013950).readPointer();
const user = singleton.add(0xA8).readPointer().add(singleton.add(0xA0).readU32() * 0x26CC00);
const LISTS = [['items', 0x38080, 24], ['ammo', 0x38200, 16], ['list2', 0x38380, 24], ['tools', 0x38500, 5]];
for (const [name, off, n] of LISTS) {
  const s = [];
  for (let i = 0; i < n; i++) s.push(`${user.add(off + i * 16 + 8).readU32()}x${user.add(off + i * 16 + 12).readU32()}`);
  console.log(`${name.padEnd(5)} ${s.join(' ')}`);
}
const bar = off => { const a = []; for (let i = 0; i < 40; i++) a.push(user.add(off + i * 4).readU32()); return a.join(','); };
console.log(`itembar ${bar(0xEDF38)}`);
console.log(`ammobar ${bar(0xEDFD8)}`);
