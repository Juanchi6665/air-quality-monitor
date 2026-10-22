#pragma once
#include <Arduino.h>

// Pagina web completa (HTML + CSS + JS) guardada en la flash del programa.
// No usa librerias externas: el telefono no tiene Internet cuando esta
// conectado al punto de acceso, asi que los graficos se dibujan a mano en <canvas>.
static const char WEB_PAGE[] PROGMEM = R"rawliteral(<!doctype html>
<html lang="es"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Monitor de aire</title>
<style>
:root{--bg:#0f1419;--card:#1a222c;--line:#2a3542;--fg:#e6edf3;--mut:#8b98a5;--acc:#3fb6a8;--warn:#e3b341;--bad:#f0605d}
*{box-sizing:border-box}body{margin:0;font-family:system-ui,sans-serif;background:var(--bg);color:var(--fg)}
header{padding:14px 16px;border-bottom:1px solid var(--line);display:flex;justify-content:space-between;align-items:center;gap:8px;flex-wrap:wrap}
h1{font-size:18px;margin:0}h2{font-size:15px;margin:0 0 10px;color:var(--mut);font-weight:600}
main{padding:12px 16px;max-width:900px;margin:auto}
section{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:14px;margin-bottom:14px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(130px,1fr));gap:10px}
.tile{background:var(--bg);border-radius:10px;padding:10px;cursor:pointer;border:1px solid transparent}
.tile.sel{border-color:var(--acc)}.tile .l{font-size:12px;color:var(--mut)}.tile .v{font-size:22px;font-weight:600;margin-top:4px}
.tile .u{font-size:13px;color:var(--mut);margin-left:3px}
.row{display:flex;gap:8px;flex-wrap:wrap;align-items:center;margin-bottom:10px}
select,input,button{font:inherit;background:var(--bg);color:var(--fg);border:1px solid var(--line);border-radius:8px;padding:8px 10px}
button{background:var(--acc);color:#05201d;border:0;font-weight:600}button.sec{background:var(--line);color:var(--fg)}
canvas{width:100%;height:240px;display:block}
.pill{font-size:12px;padding:3px 8px;border-radius:99px;background:var(--line)}.ok{color:var(--acc)}.bad{color:var(--bad)}
#tip{font-size:13px;color:var(--mut);min-height:18px}
</style></head><body>
<header><h1>Monitor de calidad del aire</h1>
<div class="row" style="margin:0"><span class="pill">Habitacion: <b id="room">-</b></span><span class="pill" id="status">-</span></div></header>
<main>
<section><h2>En vivo <span id="clock" style="float:right;font-weight:400"></span></h2><div class="grid" id="tiles"></div></section>
<section><h2>Ultima hora: <span id="liveName"></span></h2><canvas id="liveChart"></canvas><div id="tip"></div></section>
<section><h2>Historial</h2>
<div class="row"><select id="hRoom"></select><select id="hDay"></select><select id="hField"></select>
<a id="dl" href="#"><button class="sec">Descargar CSV</button></a></div>
<canvas id="histChart"></canvas><div id="hInfo" style="font-size:13px;color:var(--mut)"></div></section>
<section><h2>Habitaciones</h2>
<div class="row"><select id="rSel"></select><button onclick="selectRoom()">Medir aqui</button></div>
<div class="row"><input id="rName" placeholder="Nueva habitacion" maxlength="24"><button onclick="createRoom()">Crear y medir</button></div>
<div class="row"><button class="sec" onclick="syncTime()">Sincronizar hora con el telefono</button></div></section>
</main>
<script>
const $=id=>document.getElementById(id);let F=[],liveKey='co2',recent=null;
const api=(u,o)=>fetch(u,o).then(r=>{if(!r.ok)return r.json().then(e=>{throw e.error||r.status});return r.status==204?null:r.json()});
const post=(u,d)=>api(u,{method:'POST',body:new URLSearchParams(d)});
const fmt=(v,d)=>v==null?'--':Number(v).toFixed(d);
const field=k=>F.find(f=>f.key==k);
const ts=e=>new Date(e*1000).toISOString().slice(11,16); // el RTC guarda hora local

function chart(cv,t,y,f){
  const dpr=devicePixelRatio||1,W=cv.clientWidth,H=cv.clientHeight;cv.width=W*dpr;cv.height=H*dpr;
  const c=cv.getContext('2d');c.scale(dpr,dpr);c.clearRect(0,0,W,H);
  const css=getComputedStyle(document.body),mut=css.getPropertyValue('--mut'),ln=css.getPropertyValue('--line');
  const pts=t.map((x,i)=>[x,y[i]]).filter(p=>p[1]!=null);
  c.font='11px system-ui';c.fillStyle=mut;
  if(pts.length<2){c.fillText('Sin datos suficientes',10,20);return}
  let x0=pts[0][0],x1=pts[pts.length-1][0],lo=Infinity,hi=-Infinity;
  pts.forEach(p=>{lo=Math.min(lo,p[1]);hi=Math.max(hi,p[1])});
  if(hi-lo<1e-6){lo-=1;hi+=1}const pad=(hi-lo)*.1;lo-=pad;hi+=pad;
  const L=44,R=8,T=8,B=22,X=v=>L+(v-x0)/(x1-x0||1)*(W-L-R),Y=v=>T+(hi-v)/(hi-lo)*(H-T-B);
  c.strokeStyle=ln;c.lineWidth=1;
  for(let i=0;i<=4;i++){const v=lo+(hi-lo)*i/4,yy=Y(v);c.beginPath();c.moveTo(L,yy);c.lineTo(W-R,yy);c.stroke();c.fillText(v.toFixed(f.dec),2,yy+4)}
  for(let i=0;i<=4;i++){const v=x0+(x1-x0)*i/4;c.fillText(ts(v),Math.min(X(v)-14,W-34),H-6)}
  c.strokeStyle=css.getPropertyValue('--acc');c.lineWidth=2;c.lineJoin='round';c.beginPath();
  pts.forEach((p,i)=>{const xx=X(p[0]),yy=Y(p[1]);(i&&p[0]-pts[i-1][0]<600)?c.lineTo(xx,yy):c.moveTo(xx,yy)});c.stroke();
  cv.onclick=cv.ontouchstart=ev=>{const r=cv.getBoundingClientRect(),cx=(ev.touches?ev.touches[0]:ev).clientX-r.left;
    let best=pts[0];pts.forEach(p=>{if(Math.abs(X(p[0])-cx)<Math.abs(X(best[0])-cx))best=p});
    (cv.id=='liveChart'?$('tip'):$('hInfo')).textContent=ts(best[0])+'  →  '+fmt(best[1],f.dec)+' '+f.unit};
}

function renderTiles(d){
  const big=[{key:'temp',label:'Temperatura',unit:'°C',dec:1,v:d.temp},{key:'hum',label:'Humedad',unit:'%',dec:1,v:d.hum}];
  const all=big.concat(F.map(f=>Object.assign({v:d.values[f.key]},f)));
  $('tiles').innerHTML=all.map(f=>`<div class="tile ${f.key==liveKey?'sel':''}" onclick="pickLive('${f.key}')"><div class="l">${f.label}</div><div class="v">${fmt(f.v,f.dec)}<span class="u">${f.unit}</span></div></div>`).join('');
}
function pickLive(k){if(!field(k))return;liveKey=k;drawLive();api('/api/live').then(renderTiles)}
function drawLive(){if(!recent)return;const f=field(liveKey);$('liveName').textContent=f.label+' ('+f.unit+')';chart($('liveChart'),recent.t,recent[liveKey],f)}

async function refreshLive(){
  try{const d=await api('/api/live');renderTiles(d);$('room').textContent=d.room;
    const bad=Object.entries(d.sensors).filter(e=>!e[1]).map(e=>e[0]);
    $('status').innerHTML=(d.sd?'<span class="ok">SD OK</span>':'<span class="bad">SIN SD</span>')+(bad.length?' · <span class="bad">'+bad.join(', ')+'</span>':'');
    $('clock').textContent=new Date(d.epoch*1000).toISOString().replace('T',' ').slice(0,19);
  }catch(e){$('status').innerHTML='<span class="bad">Sin conexion</span>'}
}
async function refreshRecent(){try{recent=await api('/api/recent');drawLive()}catch(e){}}

async function loadRooms(){
  const r=await api('/api/rooms');const opts=r.rooms.map(x=>`<option ${x==r.current?'selected':''}>${x}</option>`).join('');
  $('rSel').innerHTML=opts;$('hRoom').innerHTML=opts;$('room').textContent=r.current;loadDays();
}
async function loadDays(){
  const days=await api('/api/days?room='+encodeURIComponent($('hRoom').value));
  $('hDay').innerHTML=days.reverse().map(d=>`<option>${d}</option>`).join('');loadHistory();
}
async function loadHistory(){
  const room=$('hRoom').value,day=$('hDay').value,f=field($('hField').value);
  const q='room='+encodeURIComponent(room)+'&day='+encodeURIComponent(day);$('dl').href='/api/history?download=1&'+q;
  if(!day){chart($('histChart'),[],[],f);$('hInfo').textContent='';return}
  const txt=await fetch('/api/history?'+q).then(r=>r.text());const rows=txt.trim().split('\n');
  const col=rows[0].split(';').indexOf(f.key),t=[],y=[];
  rows.slice(1).forEach(r=>{const c=r.split(';');t.push(Date.parse(c[0].replace(' ','T')+'Z')/1000);y.push(c[col]===''?null:+c[col])});
  chart($('histChart'),t,y,f);const v=y.filter(x=>x!=null);
  $('hInfo').textContent=v.length?`${v.length} registros · min ${fmt(Math.min(...v),f.dec)} · max ${fmt(Math.max(...v),f.dec)} · prom ${fmt(v.reduce((a,b)=>a+b,0)/v.length,f.dec)} ${f.unit}`:'Sin datos';
}

async function createRoom(){const n=$('rName').value.trim();if(!n)return;
  try{await post('/api/rooms',{name:n});$('rName').value='';await loadRooms()}catch(e){alert(e)}}
async function selectRoom(){try{await post('/api/rooms/select',{id:$('rSel').value});await loadRooms()}catch(e){alert(e)}}
async function syncTime(){ // el RTC guarda hora local: se envia la hora del telefono con su zona horaria
  const n=new Date(),epoch=Math.floor(n.getTime()/1000)-n.getTimezoneOffset()*60;
  try{await post('/api/time',{epoch});alert('Hora actualizada')}catch(e){alert(e)}}

(async()=>{
  F=await api('/api/fields');$('hField').innerHTML=F.map(f=>`<option value="${f.key}">${f.label}</option>`).join('');
  $('hField').value='co2';$('hRoom').onchange=loadDays;$('hDay').onchange=loadHistory;$('hField').onchange=loadHistory;
  onresize=()=>{drawLive();loadHistory()};
  await refreshLive();await refreshRecent();await loadRooms();
  setInterval(refreshLive,2000);setInterval(refreshRecent,10000);
})();
</script></body></html>)rawliteral";
