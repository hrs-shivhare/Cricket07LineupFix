/* Controlled integration replay in a freshly launched, disposable game process.
 * Invoke the original lineup wrapper on a thread already executing the game.
 * Only transient match settings/lineup state change; no roster or save writes.
 */
'use strict';
const base = Process.mainModule.base;
const at = va => base.add(va - 0x400000);
const getMode = new NativeFunction(at(0x40bad0), 'pointer', []);
const getTeam = new NativeFunction(at(0x4e04b0), 'pointer', ['uint']);
const getPlayer = new NativeFunction(at(0x4d8f10), 'pointer', ['uint']);
const getCount = new NativeFunction(at(0x4e0550), 'int', []);
const build = new NativeFunction(at(0x4180d0), 'void', ['pointer', 'int', 'uint'], 'thiscall');
let finished = false;
function assert(ok, message) { if (!ok) throw new Error(message); }
function player(id) {
    const p = getPlayer(id);
    assert(!p.isNull(), 'Missing player ' + id);
    return {id, name:p.add(4).readCString(),
        role:p.add(0x15f).readU8() & 15,
        skill:(p.add(0x6c).readU32() >>> 6) & 63,
        accuracy:(p.add(0x74).readU32() >>> 12) & 63,
        primary:(p.add(0x84).readU32() >>> 10) & 15,
        secondary:(p.add(0x84).readU32() >>> 14) & 15,
        legacy:p.add(0x8c).readU32() & 63};
}
function squad(team) {
    const p = getTeam(team), n = getCount(), result = [];
    assert(!p.isNull() && n >= 11 && n <= 20, 'Roster not ready');
    for (let i=0; i<n; ++i) {
        const id=p.add(2*i).readU16();
        if (id) result.push(player(id));
    }
    return result;
}
function keeperFor(s) {
    const tests = [p=>[6,7,8].includes(p.role),p=>p.primary===1,
        p=>p.secondary===1,p=>p.legacy===4,p=>true];
    for (const test of tests) {
        let best=null;
        for (const p of s) if (test(p) && (!best || p.skill>=best.skill)) best=p;
        if (best) return best;
    }
}
function run(g,team,side,pitch,overs) {
    const s=squad(team), k=keeperFor(s);
    const openers=s.filter(p=>p.id!==k.id && [2,8].includes(p.role))
        .map((p,i)=>({...p,i})).sort((a,b)=>b.skill-a.skill || a.i-b.i);
    at(0x930ec4).writeS32(pitch);
    at(0x930eb4).writeS32(overs);
    build(g.add(0xe0),side,team);
    const xi=Array.from({length:11},(_,i)=>player(g.add(0xfc+side*22+2*i).readU16()));
    const keeper=g.add(0x128+side*2).readU16();
    const bowling=Array.from({length:6},(_,i)=>g.add(0x12c+side*12+2*i).readU16());
    assert(keeper===k.id,'Wrong designated keeper');
    assert(new Set(xi.map(p=>p.id)).size===11,'Duplicate XI');
    assert(xi.every(p=>s.some(q=>q.id===p.id)),'XI outside squad');
    assert(xi.some(p=>p.id===keeper),'Keeper missing');
    const fixed=new Set();
    if (k.role===8) {
        assert(xi[0].id===k.id,'OWK not No.1'); fixed.add(0);
    } else if (openers[1]) {
        assert(xi[0].id===openers[1].id,'Second opener not No.1'); fixed.add(0);
    }
    if (openers[0]) {
        assert(xi[1].id===openers[0].id,'Best other opener not No.2'); fixed.add(1);
    }
    const ordered=xi.filter((p,i)=>!fixed.has(i));
    assert(ordered.every((p,i)=>!i || ordered[i-1].skill>=p.skill),'Batting order not sorted');
    assert(bowling.every(id=>xi.some(p=>p.id===id)),'Bowler outside XI');
    assert(new Set(bowling).size===6,'Duplicate bowling order');
    assert(!bowling.includes(keeper),'Keeper in bowling order');
    return {team,side,pitch,overs,keeper,xi,bowling,passed:true};
}
const user32=Process.getModuleByName('user32.dll');
const listeners=['PeekMessageA','PeekMessageW','GetMessageA','GetMessageW'].map(name =>
Interceptor.attach(user32.getExportByName(name), {
    onLeave(retval) {
        if (finished) return;
        const g=getMode();
        if (g.isNull()) return;
        try { squad(0); } catch (_) { return; }
        finished=true;
        const pitch=at(0x930ec4).readS32(),overs=at(0x930eb4).readS32();
        try {
            const results=[];
            for (const team of [0,13]) for (const test of [false,true])
                for (const p of [0,8]) results.push(run(g,team,team===0?0:1,p,test?-1:50));
            let owkTeam=null;
            for (let t=0;t<67;++t) {
                try { if (keeperFor(squad(t)).role===8) { owkTeam=t; break; } } catch (_) {}
            }
            if (owkTeam!==null) results.push(run(g,owkTeam,0,0,50));
            send({type:'runtime-lineup-checks',passed:true,cases:results.length,owkTeam,results});
        } catch(e) { send({type:'runtime-lineup-checks',passed:false,error:String(e),stack:e.stack}); }
        finally {
            at(0x930ec4).writeS32(pitch);
            at(0x930eb4).writeS32(overs);
        }
    }
}));
const postThread=new NativeFunction(user32.getExportByName('PostThreadMessageA'),
    'bool',['uint','uint','uint','int']);
for (const thread of Process.enumerateThreads()) postThread(thread.id,0,0,0);
send({type:'armed',pid:Process.id,trigger:'next window-message pump return with a valid roster'});
