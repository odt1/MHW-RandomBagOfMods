const base = Process.getModuleByName('MonsterHunterWorld.exe').base;
const c = { copy: 0, dz: 0, cam: null };
const a = Interceptor.attach(base.add(0x12a9b70), { onEnter() { c.copy++; } });
const b = Interceptor.attach(base.add(0x1f9ea40), { onEnter(x) { c.dz++; c.cam = x[0]; } });
setTimeout(() => { a.detach(); b.detach();
  const p = base.add(0x5011f58).readPointer();
  console.log(JSON.stringify({ copyCalls: c.copy, deadzoneCalls: c.dz, cam: c.cam && c.cam.toString(),
    camDz: c.cam && [c.cam.add(0x1578).readFloat(), c.cam.add(0x157c).readFloat()], camIn: c.cam && [c.cam.add(0xa90).readFloat(), c.cam.add(0xa94).readFloat()],
    param: [0x12c, 0x130, 0x134].map(o => p.add(o).readFloat()) }));
}, 2000);
