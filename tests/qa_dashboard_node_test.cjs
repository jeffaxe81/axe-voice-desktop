"use strict";
const fs=require("node:fs"),vm=require("node:vm"),assert=require("node:assert/strict");
const content=fs.readFileSync("docs/qa/AXE-Voice-Consolidador-Windows-v1.0.html","utf8");
const m=content.match(/<script>([\s\S]*?)<\/script>/);
assert.ok(m,"embedded offline script missing");
const script=m[1],split=script.indexOf('$("files").addEventListener');
assert.ok(split>0,"testable model boundary missing");
const context={module:{exports:{}}};
vm.createContext(context);
vm.runInContext(script.slice(0,split)+"\nmodule.exports={CASES,normalized,summarize,reports,excelCell};",context,{timeout:3000});
const {CASES,normalized,summarize,reports,excelCell}=context.module.exports;
assert.equal(CASES.length,44);
assert.equal(CASES.filter(x=>x.required).length,39);
function fixture(){
  const results={};for(const t of CASES)results[t.id]={status:"pass"};
  return {format:"axe_voice_windows_acceptance",schema:1,
    meta:{tester:"QA",date:"2026-11-08",machine:"PC-LAB-01",os:"Windows 11 x64",
      edition:"Win32 Baresip candidato",version:"0.83.0",commit:"01234567",
      reviewer:"Supervisor",reviewDate:"2026-11-08",decision:"Testes locais aprovados"},
    results,tests:CASES.map(x=>({id:x.id,title:x.title,required:x.required})),
    localAcceptanceVerified:false};
}
const complete=normalized(fixture(),"pc-lab.json");
assert.equal(complete.accepted,true,"recompute from content, not user supplied flag");
assert.equal(complete.done,39);
assert.equal(complete.issues.length,0);
const absent=fixture();delete absent.results["B03"];
assert.equal(normalized(absent,"missing.json").missing,1);
assert.equal(normalized(absent,"missing.json").accepted,false);
const requiredNA=fixture();requiredNA.results["A01"].status="na";
assert.equal(normalized(requiredNA,"na.json").missing,1);
const optionalNA=fixture();optionalNA.results["A04"].status="na";
assert.equal(normalized(optionalNA,"optional.json").accepted,true);
const bad=fixture();bad.results["D03"].status="fail";bad.localAcceptanceVerified=true;
const broken=normalized(bad,"failed.json");
assert.equal(broken.accepted,false);
assert.equal(broken.fail,1);
const blocked=fixture();blocked.results["E01"].status="block";
const blockedReport=normalized(blocked,"blocked.json");
assert.equal(blockedReport.block,1);
const missingReview=fixture();missingReview.meta.reviewer="";
assert.equal(normalized(missingReview,"unsigned.json").accepted,false);
for(const item of [
  {format:"bad",schema:1,meta:{},results:{}},
  {format:"axe_voice_windows_acceptance",schema:2,meta:{},results:{}},
  {format:"axe_voice_windows_acceptance",schema:1,meta:{},results:[]}
])assert.throws(()=>normalized(item,"invalid.json"));
const invalidStatus=fixture();invalidStatus.results.A01.status="success";
assert.throws(()=>normalized(invalidStatus,"wrong.json"));
const stale=fixture();stale.tests.pop();assert.throws(()=>normalized(stale,"stale.json"));
reports.push(complete,broken,blockedReport,normalized(absent,"missing.json"));
const summary=summarize();
assert.equal(summary.reports,4);
assert.equal(summary.locallyAccepted,1);
assert.equal(summary.fail,1);
assert.equal(summary.block,1);
assert.equal(summary.missing,1);
assert.equal(summary.requiredPossible,156);
assert.equal(summary.requiredExecuted,155);
assert.equal(summary.productionApproved,false);
assert.equal(summary.liveTelephony,"deferred_not_homologated");
assert.ok(summary.issues.some(x=>x.id==="D03"&&x.fail===1));
assert.ok(summary.issues.some(x=>x.id==="E01"&&x.block===1));
assert.ok(summary.issues.some(x=>x.id==="B03"&&x.missing===1));
assert.ok(excelCell("=1+1").includes("'=1+1"));
assert.ok(excelCell("@IMPORT").includes("'@IMPORT"));
assert.ok(excelCell("+SUM(A1)").includes("'+SUM"));
console.log("QA Windows consolidated dashboard: 44 cases and 18+ offline security/acceptance assertions PASS");
