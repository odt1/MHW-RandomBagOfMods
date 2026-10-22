const base = Process.getModuleByName('MonsterHunterWorld.exe').base;
const p = base.add(0x5011f58).readPointer();
const before = [0x12c, 0x130, 0x134].map(o => p.add(o).readFloat());
p.add(0x12c).writeFloat(0.03); p.add(0x130).writeFloat(0.03); p.add(0x134).writeFloat(0.05);
console.log('before ' + before + ' after ' + [0x12c, 0x130, 0x134].map(o => p.add(o).readFloat()));
