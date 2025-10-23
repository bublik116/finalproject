/*
  ESP32 Fridge — Frontend (mock API)
*/

(() => {
  const $ = (id) => document.getElementById(id);

  // Tabs
  const tabOverview = $('tabOverview');
  const tabHistory = $('tabHistory');
  const tabControls = $('tabControls');
  const views = {
    overview: document.querySelector('.view.overview')?.parentElement, // grid holds multiple overview sections
    history: document.querySelector('.view.history'),
    controls: document.querySelector('.view.controls'),
  };
  function showTab(name){
    // multiple overview sections: show all with .view.overview
    document.querySelectorAll('.view.overview').forEach(el => el.classList.remove('hidden'));
    views.history?.classList.add('hidden');
    views.controls?.classList.add('hidden');
    tabOverview.classList.remove('active');
    tabHistory.classList.remove('active');
    tabControls.classList.remove('active');
    if (name==='overview'){
      tabOverview.classList.add('active');
    } else if (name==='history'){
      document.querySelectorAll('.view.overview').forEach(el => el.classList.add('hidden'));
      views.history?.classList.remove('hidden');
      tabHistory.classList.add('active');
    } else if (name==='controls'){
      document.querySelectorAll('.view.overview').forEach(el => el.classList.add('hidden'));
      views.controls?.classList.remove('hidden');
      tabControls.classList.add('active');
    }
  }
  tabOverview?.addEventListener('click', ()=> showTab('overview'));
  tabHistory?.addEventListener('click', ()=> showTab('history'));
  tabControls?.addEventListener('click', ()=> showTab('controls'));

  const tRoomEl = $('tRoom');
  const tEvapEl = $('tEvap');
  const relayCompEl = $('relayComp');
  const relayFanEl = $('relayFan');
  const relayDefEl = $('relayDef');
  const relayLightEl = $('relayLight');
  const doorEl = $('door');
  const modeEl = $('mode');
  const baseUrlEl = $('baseUrl');
  const connStatusEl = $('connStatus');
  const saveUrlBtn = $('saveUrlBtn');

  const spInput = $('spInput');
  const hystInput = $('hystInput');
  const tonInput = $('tonInput');
  const toffInput = $('toffInput');
  const startDelayInput = $('startDelayInput');
  const saveCfgBtn = $('saveCfgBtn');
  const saveStatus = $('saveStatus');

  const logEl = $('log');
  const clearLogBtn = $('clearLogBtn');
  const toggleLightBtn = $('toggleLightBtn');

  // History graph
  const histCanvas = $('histCanvas');
  const histCtx = histCanvas ? histCanvas.getContext('2d') : null;
  const autoScrollEl = $('autoScroll');
  const histLenEl = $('histLen');
  let histRoom = [];
  let histEvap = [];

  // Controls tab buttons
  const defrostStartBtn = $('defrostStartBtn');
  const defrostStopBtn = $('defrostStopBtn');
  const doorOpenBtn = $('doorOpenBtn');
  const doorCloseBtn = $('doorCloseBtn');
  const beepBtn = $('beepBtn');

  // Temp nudge (mock)
  const coolRoomBtn = $('coolRoomBtn');
  const heatRoomBtn = $('heatRoomBtn');
  const coolEvapBtn = $('coolEvapBtn');
  const heatEvapBtn = $('heatEvapBtn');

  function badge(el, on) {
    el.textContent = on ? 'вкл' : 'выкл';
    el.classList.remove('grey','green','red');
    el.classList.add(on ? 'green' : 'grey');
  }
  function badgeDoor(el, open) {
    el.textContent = open ? 'открыта' : 'закрыта';
    el.classList.remove('grey','green','red');
    el.classList.add(open ? 'red' : 'green');
  }

  function setConn(ok) {
    connStatusEl.textContent = ok ? 'онлайн' : 'оффлайн';
    connStatusEl.classList.remove('grey','green');
    connStatusEl.classList.add(ok ? 'green' : 'grey');
  }

  function addLog(msg) {
    const ts = new Date().toLocaleTimeString();
    logEl.textContent += `[${ts}] ${msg}\n`;
    logEl.scrollTop = logEl.scrollHeight;
  }

  // Привязки UI
  baseUrlEl.value = FridgeAPI.getBaseUrl() || '';
  saveUrlBtn.addEventListener('click', () => {
    FridgeAPI.setBaseUrl(baseUrlEl.value);
    addLog(`Базовый URL сохранён: ${baseUrlEl.value || '(пусто)'}`);
  });
  clearLogBtn.addEventListener('click', () => { logEl.textContent=''; });
  toggleLightBtn.addEventListener('click', async () => {
    const r = await FridgeAPI.apiToggleLight();
    addLog(`Свет: ${r.light ? 'вкл' : 'выкл'}`);
    refreshOnce();
  });

  // Controls tab actions (mock)
  defrostStartBtn?.addEventListener('click', async ()=>{ await FridgeAPI.apiDefrostStart(); addLog('Оттайка: старт'); refreshOnce(); });
  defrostStopBtn?.addEventListener('click', async ()=>{ await FridgeAPI.apiDefrostStop(); addLog('Оттайка: стоп'); refreshOnce(); });
  doorOpenBtn?.addEventListener('click', async ()=>{ await FridgeAPI.apiDoorOpen(); addLog('Дверь: открыта (мок)'); refreshOnce(); });
  doorCloseBtn?.addEventListener('click', async ()=>{ await FridgeAPI.apiDoorClose(); addLog('Дверь: закрыта (мок)'); refreshOnce(); });
  beepBtn?.addEventListener('click', async ()=>{ await FridgeAPI.apiBeep(); addLog('Зуммер: бип (мок)'); });

  // Temp nudges
  coolRoomBtn?.addEventListener('click', async ()=>{ await FridgeAPI.apiTempNudge('room', -0.5); addLog('Камера: -0.5°C (мок)'); refreshOnce(); });
  heatRoomBtn?.addEventListener('click', async ()=>{ await FridgeAPI.apiTempNudge('room', +0.5); addLog('Камера: +0.5°C (мок)'); refreshOnce(); });
  coolEvapBtn?.addEventListener('click', async ()=>{ await FridgeAPI.apiTempNudge('evap', -0.5); addLog('Испаритель: -0.5°C (мок)'); refreshOnce(); });
  heatEvapBtn?.addEventListener('click', async ()=>{ await FridgeAPI.apiTempNudge('evap', +0.5); addLog('Испаритель: +0.5°C (мок)'); refreshOnce(); });

  // История температур
  function pushHistory(room, evap){
    const maxLen = Math.max(100, Math.min(5000, parseInt(histLenEl?.value, 10) || 1000));
    histRoom.push(room); histEvap.push(evap);
    while (histRoom.length > maxLen) histRoom.shift();
    while (histEvap.length > maxLen) histEvap.shift();
  }
  function drawHistory(){
    if (!histCtx || !histCanvas) return;
    const W = histCanvas.width, H = histCanvas.height;
    histCtx.clearRect(0,0,W,H);
    histCtx.fillStyle = '#0b0f14'; histCtx.fillRect(0,0,W,H);
    // Оси
    histCtx.strokeStyle = '#263241'; histCtx.lineWidth = 1;
    histCtx.beginPath(); histCtx.moveTo(0, H/2); histCtx.lineTo(W, H/2); histCtx.stroke();
    // Диапазон температуры (±40C)
    const ymin = -40, ymax = 40;
    const dataN = Math.max(histRoom.length, histEvap.length, 1);
    const xStep = W / Math.max(1, dataN-1);

    const draw = (arr, color) => {
      histCtx.strokeStyle = color; histCtx.lineWidth = 1.5; histCtx.beginPath();
      for (let i=0;i<arr.length;i++){
        const x = i * xStep;
        const y = H - (arr[i] - ymin) * (H/(ymax-ymin));
        if (i===0) histCtx.moveTo(x,y); else histCtx.lineTo(x,y);
      }
      histCtx.stroke();
    };
    draw(histRoom, '#58a6ff');
    draw(histEvap, '#3fb950');
  }

  // Аварии/события (мок: простые пороги)
  const alarmsEl = $('alarms');
  function updateAlarms(state){
    if (!alarmsEl) return;
    const msgs = [];
    if (state.t_room > 15) msgs.push('ВНИМАНИЕ: t_room > 15°C');
    if (state.t_evap > 0 && state.relays.compressor) msgs.push('АЛАРМ: испаритель греется при включенном компрессоре');
    if (state.door) msgs.push('Дверь открыта');
    alarmsEl.innerHTML = msgs.map(m=>`<div class="alarm">${m}</div>`).join('') || '<span class="muted">Нет событий</span>';
  }

  function setConn(ok) {
    connStatusEl.textContent = ok ? 'онлайн' : 'оффлайн';
    connStatusEl.classList.remove('grey','green');
    connStatusEl.classList.add(ok ? 'green' : 'grey');
  }

  function addLog(msg) {
    const ts = new Date().toLocaleTimeString();
    logEl.textContent += `[${ts}] ${msg}\n`;
    logEl.scrollTop = logEl.scrollHeight;
  }

  // Привязки UI
  baseUrlEl.value = FridgeAPI.getBaseUrl() || '';
  saveUrlBtn.addEventListener('click', () => {
    FridgeAPI.setBaseUrl(baseUrlEl.value);
    addLog(`Базовый URL сохранён: ${baseUrlEl.value || '(пусто)'}`);
  });

  clearLogBtn.addEventListener('click', () => { logEl.textContent=''; });
  toggleLightBtn.addEventListener('click', async () => {
    const r = await FridgeAPI.apiToggleLight();
    addLog(`Свет: ${r.light ? 'вкл' : 'выкл'}`);
    refreshOnce();
  });

  const spInput = $('spInput');
  const hystInput = $('hystInput');
  const tonInput = $('tonInput');
  const toffInput = $('toffInput');
  const startDelayInput = $('startDelayInput');
  const saveCfgBtn = $('saveCfgBtn');
  const saveStatus = $('saveStatus');

  saveCfgBtn.addEventListener('click', async () => {
    const cfg = {
      setpoint: parseFloat(spInput.value),
      hysteresis: parseFloat(hystInput.value),
      ton_min_s: parseInt(tonInput.value, 10),
      toff_min_s: parseInt(toffInput.value, 10),
      start_delay_s: parseInt(startDelayInput.value, 10),
    };
    await FridgeAPI.apiSaveConfig(cfg);
    saveStatus.textContent = 'сохранено';
    setTimeout(()=> saveStatus.textContent='', 1500);
    addLog(`Настройки сохранены: SP=${cfg.setpoint} H=${cfg.hysteresis}`);
  });

  async function refreshOnce(){
    try {
      const st = await FridgeAPI.apiGetState();
      setConn(true);
      tRoomEl.textContent = `${st.t_room.toFixed(1)} °C`;
      tEvapEl.textContent = `${st.t_evap.toFixed(1)} °C`;
      badge(relayCompEl, !!st.relays.compressor);
      badge(relayFanEl, !!st.relays.fan);
      badge(relayDefEl, !!st.relays.defrost);
      badge(relayLightEl, !!st.relays.light);
      badgeDoor(doorEl, !!st.door);
      modeEl.textContent = st.mode || '—';

      // Настройки
      spInput.value = st.cfg?.setpoint ?? spInput.value;
      hystInput.value = st.cfg?.hysteresis ?? hystInput.value;
      tonInput.value = st.cfg?.ton_min_s ?? tonInput.value;
      toffInput.value = st.cfg?.toff_min_s ?? toffInput.value;
      startDelayInput.value = st.cfg?.start_delay_s ?? startDelayInput.value;

      // История
      pushHistory(st.t_room, st.t_evap);
      drawHistory();
      if (autoScrollEl?.checked) {
        const cont = histCanvas?.parentElement; // секция
        cont?.scrollIntoView({ block: 'nearest' });
      }
      // Аварии
      updateAlarms(st);
    } catch (e) {
      setConn(false);
    }
  }

  // Периодический опрос
  setInterval(refreshOnce, 1500);
  // Первый запуск
  refreshOnce();
})();
