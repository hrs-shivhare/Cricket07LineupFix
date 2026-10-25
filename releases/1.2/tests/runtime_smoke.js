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
/* Independent reference model, not a captured native-pool observation.
 * The actual build below still executes all four original native helpers. */
function specialistReference(s,k,openers,pitch,isTest) {
    const blocked=new Set([k.id]);
    if(openers[0]) blocked.add(openers[0].id);
    if(k.role!==8&&openers[1]) blocked.add(openers[1].id);
    const pool=[];
    function take(roles,quota) {
        for(let i=0;i<quota;++i) {
            const p=s.filter(p=>roles.includes(p.role)&&!blocked.has(p.id))
                .reduce((best,p)=>!best||p.accuracy>best.accuracy?p:best,null);
            if(!p) break;
            blocked.add(p.id);pool.push(p.id);
        }
    }
    const dusty=[1,6,7,15].includes(pitch),green=[4,8,9,16].includes(pitch);
    take([10],2);
    take([3],(dusty?0:green?(isTest?2:0):1)+(2-pool.length));
    take([4,5],dusty&&isTest?2:1);
    const target=dusty?(isTest?4:3):green?(isTest?5:3):4;
    take([3,4,5,10],Math.max(0,target-pool.length));
    return pool;
}
function expectedSelection(s,k,openers,pool,pitch,isTest) {
    const fixed=[0,openers[0]?openers[0].id:0];
    if(k.role===8) fixed[0]=k.id;
    else if(openers[1]) fixed[0]=openers[1].id;
    const picked=new Set([k.id,...pool,...fixed.filter(Boolean)]);
    const choose=(test,accuracy)=>s.filter(p=>!picked.has(p.id)&&test(p))
        .reduce((best,p)=>!best || p[accuracy?'accuracy':'skill']>best[accuracy?'accuracy':'skill']?p:best,null);
    const accuracyWinner=isTest?null:choose(p=>[5,9].includes(p.role),true);
    const trio=[],additional=[];
    for(let n=0;n<3&&picked.size<11;++n) {
        const p=choose(p=>[1,2,5,6,7,8,9].includes(p.role),false);
        if(!p) break;
        trio.push(p); picked.add(p.id);
    }
    const credits=trio.filter(p=>[5,9].includes(p.role)).length;
    const accuracyCredit=accuracyWinner!==null&&trio.some(p=>p.id===accuracyWinner.id);
    const quota=isTest?([4,8,9,16].includes(pitch)?0:1):
        ([1,6,7,15,4,8,9,16].includes(pitch)?2:1);
    const remaining=Math.max(0,quota-credits);
    for(let i=0;i<quota&&picked.size<11;++i) {
        const filter=isTest?(p=>[1,9].includes(p.role)):accuracyCredit?(p=>true):
            i<remaining?(p=>[5,9].includes(p.role)):(p=>true);
        let p=choose(filter,!isTest&&!accuracyCredit&&i===0);
        if(isTest&&p&&p.role===1) {
            const tied=s.find(q=>!picked.has(q.id)&&q.role===9&&q.skill===p.skill);
            if(tied) p=tied;
        }
        if(!p) break;
        picked.add(p.id);
        additional.push({id:p.id,role:p.role,pool:isTest?'BAT/ALL':!accuracyCredit&&i<remaining?'ALL/ASP':'any',
            criterion:!isTest&&!accuracyCredit&&i===0?'Accuracy':'Skill'});
    }
    while(picked.size<11) {
        const p=choose(()=>true,false);
        assert(p!==null,'Insufficient remaining players'); picked.add(p.id);
    }
    return {ids:[...picked],battingTrio:trio.map(p=>p.id),battingAllrounders:credits,
        accuracyWinner:accuracyWinner?accuracyWinner.id:null,accuracyCredit,
        requiredAdditionalALL:isTest?null:accuracyCredit?0:remaining,additionalPicks:additional};
}
function run(g,team,side,pitch,overs) {
    const s=squad(team), k=keeperFor(s);
    const openers=s.filter(p=>p.id!==k.id && [2,8].includes(p.role))
        .map((p,i)=>({...p,i})).sort((a,b)=>b.skill-a.skill || a.i-b.i);
    at(0x930ec4).writeS32(pitch);
    at(0x930eb4).writeS32(overs);
    const specialistPool=specialistReference(s,k,openers,pitch,overs===-1);
    build(g.add(0xe0),side,team);
    const expected=expectedSelection(s,k,openers,specialistPool,pitch,overs===-1);
    const xi=Array.from({length:11},(_,i)=>player(g.add(0xfc+side*22+2*i).readU16()));
    const keeper=g.add(0x128+side*2).readU16();
    const bowling=Array.from({length:6},(_,i)=>g.add(0x12c+side*12+2*i).readU16());
    assert(keeper===k.id,'Wrong designated keeper');
    assert(new Set(xi.map(p=>p.id)).size===11,'Duplicate XI');
    assert(xi.every(p=>s.some(q=>q.id===p.id)),'XI outside squad');
    assert(xi.some(p=>p.id===keeper),'Keeper missing');
    assert(xi.every(p=>expected.ids.includes(p.id)),'XI differs from revised selection rules');
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
    const opener=p=>[2,8].includes(p.role)?1:0;
    assert(ordered.every((p,i)=>!i || opener(ordered[i-1])>opener(p) ||
        (opener(ordered[i-1])===opener(p)&&ordered[i-1].skill>=p.skill)),
        'Batting order violates extra opener priority or Skill sort');
    assert(bowling.every(id=>xi.some(p=>p.id===id)),'Bowler outside XI');
    assert(new Set(bowling).size===6,'Duplicate bowling order');
    assert(!bowling.includes(keeper),'Keeper in bowling order');
    return {team,side,pitch,overs,activeSquadCount:s.length,keeper,
        extraOpeners:xi.slice(2).filter(p=>opener(p)).map(p=>p.id),
        specialistReferencePool:specialistPool,
        poolEvidence:'reference model; actual native stages run inside build',
        ...expected,xi,bowling,passed:true};
}
const user32=Process.getModuleByName('user32.dll');
const listeners=['PeekMessageA','PeekMessageW','GetMessageA','GetMessageW'].map(name =>
Interceptor.attach(user32.getExportByName(name), {
    onLeave(retval) {
        if (finished) return;
        const g=getMode();
        if (g.isNull()) return;
        /* This installed-roster replay expects the fully initialized 20-player
         * selection context, matching the user's captured South Africa case. */
        try { squad(0); if(getCount()!==20) return; } catch (_) { return; }
        finished=true;
        const pitch=at(0x930ec4).readS32(),overs=at(0x930eb4).readS32();
        try {
            const results=[];
            const teams=[0,13,1,15,30,2];
            for (const team of teams) for (const test of [false,true])
                for (const p of [0,8]) results.push(run(g,team,team===0?0:1,p,test?-1:50));
            let owkTeam=null;
            for (let t=0;t<67;++t) {
                try { if (keeperFor(squad(t)).role===8) { owkTeam=t; break; } } catch (_) {}
            }
            if (owkTeam!==null && !teams.includes(owkTeam)) results.push(run(g,owkTeam,0,0,50));
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
Interceptor.flush();
send({type:'armed',pid:Process.id,trigger:'next window-message pump return with a valid roster'});
for (const thread of Process.enumerateThreads()) postThread(thread.id,0,0,0);
