/* Disposable-process integration replay of the bowling-first 3.0 algorithm.
 * Read the live roster, calculate an independent expected trace, and invoke
 * the original wrapper on a game thread. No executable/roster file writes. */
'use strict';
const base=Process.mainModule.base,at=va=>base.add(va-0x400000);
const getMode=new NativeFunction(at(0x40bad0),'pointer',[]);
const getTeam=new NativeFunction(at(0x4e04b0),'pointer',['uint']);
const getPlayer=new NativeFunction(at(0x4d8f10),'pointer',['uint']);
const getCount=new NativeFunction(at(0x4e0550),'int',[]);
const build=new NativeFunction(at(0x4180d0),'void',['pointer','int','uint'],'thiscall');
let finished=false;
function assert(ok,message){if(!ok)throw new Error(message);}
function player(id){
    const p=getPlayer(id);assert(id&&!p.isNull(),'Missing player '+id);
    return {id,name:p.add(4).readCString(),role:p.add(0x15f).readU8()&15,
        skill:(p.add(0x6c).readU32()>>>6)&63,accuracy:(p.add(0x74).readU32()>>>12)&63,
        primary:(p.add(0x84).readU32()>>>10)&15,secondary:(p.add(0x84).readU32()>>>14)&15,
        legacy:p.add(0x8c).readU32()&63};
}
function squad(team){
    const p=getTeam(team),n=getCount(),s=[];
    assert(!p.isNull()&&n>=11&&n<=20,'Roster not ready');
    for(let i=0;i<n;++i){const id=p.add(i*2).readU16();if(id)s.push({...player(id),index:i});}
    return s;
}
function expectedSelection(s,pitch,test){
    const selected=new Set(),trace={},seed=[];
    function best(roles,rating){
        return s.filter(p=>!selected.has(p.id)&&(!roles||roles.includes(p.role)))
            .sort((a,b)=>b[rating]-a[rating]||a.index-b.index)[0]||null;
    }
    function take(name,amount,roles,rating,fallbacks=[],bowling=false){
        trace[name]=[];
        for(let n=0;n<amount&&selected.size<11;++n){
            let p=best(roles,rating);
            for(const pool of fallbacks)if(!p)p=best(pool,rating);
            if(!p)break;
            selected.add(p.id);trace[name].push(p.id);
            if(bowling)seed.push(p.id);
        }
    }
    const dusty=[1,6,7,15].includes(pitch),green=[4,8,9,16].includes(pitch);
    const allrounderQuota=test?0:dusty||green?2:1;
    take('allrounders',allrounderQuota,[5,9],'accuracy',[null]);
    take('openingBowling',2,[10],'accuracy',[],true);
    const openingBowlerVacancies=2-trace.openingBowling.length;
    const additionalBowlingQuota=dusty?0:green?(test?2:0):1;
    take('additionalBowling',additionalBowlingQuota+openingBowlerVacancies,[3,9],'accuracy',[null],true);
    take('spin',dusty&&test?2:1,[4,5],'accuracy',[null],true);
    seed.push(...trace.allrounders);

    const tiers=[p=>[6,7,8].includes(p.role),p=>p.primary===1,p=>p.secondary===1,p=>p.legacy===4,p=>true];
    const keeperPool=tiers.map(f=>s.filter(p=>!selected.has(p.id)&&f(p))).find(pool=>pool.length>0);
    const keeper=keeperPool.sort((a,b)=>b.skill-a.skill||b.index-a.index)[0];
    assert(keeper!==undefined,'No remaining keeper');
    selected.add(keeper.id);trace.keeper=[keeper.id];
    const openers=s.filter(p=>(!selected.has(p.id)||p.id===keeper.id)&&[2,8].includes(p.role))
        .sort((a,b)=>b.skill-a.skill||a.index-b.index).slice(0,2);
    trace.openers=openers.map(p=>p.id);
    for(const p of openers)selected.add(p.id);
    const keeperOpens=openers.some(p=>p.id===keeper.id);
    const missingOpeningSlots=2-openers.length;
    const testFlexPlaces=test&&!green?1:0;
    const battingQuota=3+(keeperOpens?1:0)+missingOpeningSlots+testFlexPlaces;
    take('batting',battingQuota,[1,2,5,6,7,8,9],'skill');
    take('shortage',11-selected.size,null,'skill');
    const opener=p=>[2,8].includes(p.role)?1:0;
    const order=s.filter(p=>selected.has(p.id)).sort((a,b)=>opener(b)-opener(a)||b.skill-a.skill||a.index-b.index);
    const bench=s.filter(p=>!selected.has(p.id)).sort((a,b)=>b.skill-a.skill||a.index-b.index);
    return {keeper:keeper.id,expectedXI:order.map(p=>p.id),expectedBench:bench.map(p=>p.id),
        bowlingSeed:seed,allrounderQuota,openingBowlerVacancies,additionalBowlingQuota,
        keeperOpens,missingOpeningSlots,testFlexPlaces,battingQuota,trace};
}
function run(g,team,side,pitch,overs){
    const s=squad(team),expected=expectedSelection(s,pitch,overs===-1);
    at(0x930ec4).writeS32(pitch);at(0x930eb4).writeS32(overs);
    build(g.add(0xe0),side,team);
    const xi=Array.from({length:11},(_,i)=>player(g.add(0xfc+side*22+2*i).readU16()));
    const keeper=g.add(0x128+side*2).readU16();
    const bowling=Array.from({length:6},(_,i)=>g.add(0x12c+side*12+2*i).readU16());
    const bench=Array.from({length:s.length-11},(_,i)=>g.add(0x168+side*18+2*i).readU16());
    assert(keeper===expected.keeper,'Keeper mismatch');
    assert(xi.every((p,i)=>p.id===expected.expectedXI[i]),'XI/order differs from bowling-first model');
    assert(new Set(xi.map(p=>p.id)).size===11,'Duplicate XI');
    assert(bench.every((id,i)=>id===expected.expectedBench[i]),'Bench mismatch');
    assert(g.add(0x18c+side).readU8()===Math.max(0,bench.length-1),'Reserve metadata mismatch');
    assert(expected.bowlingSeed.every((id,i)=>bowling[i]===id),'Bowling seed changed');
    assert(new Set(bowling).size===6&&bowling.every(id=>xi.some(p=>p.id===id)),'Invalid bowling order');
    assert(!bowling.includes(keeper),'Keeper in bowling order');
    const captain=g.add(0x198+side*2).readU16();
    assert(xi.some(p=>p.id===captain),'Captain outside XI');
    return {team,side,pitch,overs,activeSquadCount:s.length,...expected,xi,bowling,bench,captain,passed:true};
}
const user32=Process.getModuleByName('user32.dll');
const listeners=['PeekMessageA','PeekMessageW','GetMessageA','GetMessageW'].map(name=>
Interceptor.attach(user32.getExportByName(name),{
    onLeave(){
        if(finished)return;
        const g=getMode();if(g.isNull())return;
        const asi=Process.findModuleByName('Cricket07Lineup.asi');
        if(!asi||new NativeFunction(asi.getExportByName('GetLineupStatus'),'int',[])()!==1)return;
        try{squad(0);if(getCount()!==20)return;}catch(_){return;}
        finished=true;
        const oldPitch=at(0x930ec4).readS32(),oldOvers=at(0x930eb4).readS32();
        try{
            const results=[];
            for(const team of [0,13,1,15,30,2])for(const test of [false,true])
                for(const pitch of [0,8,1])results.push(run(g,team,team%2,pitch,test?-1:50));
            send({type:'runtime-lineup-checks',version:'3.0',passed:true,cases:results.length,results});
        }catch(e){send({type:'runtime-lineup-checks',passed:false,error:String(e),stack:e.stack});}
        finally{at(0x930ec4).writeS32(oldPitch);at(0x930eb4).writeS32(oldOvers);}
    }
}));
const post=new NativeFunction(user32.getExportByName('PostThreadMessageA'),'bool',['uint','uint','uint','int']);
Interceptor.flush();
send({type:'armed',pid:Process.id,trigger:'game message return; loaded ASI and 20-player context'});
for(const thread of Process.enumerateThreads())post(thread.id,0,0,0);
