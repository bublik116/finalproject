/*
  Мок-API для фронтенда ESP32 Fridge.
  Позже будет заменено на реальные REST/WebSocket вызовы к устройству.
*/

(function(){
  const LS_KEY = 'esp32_fridge_base_url';
  let baseUrl = localStorage.getItem(LS_KEY) || '';

  function setBaseUrl(url){
    baseUrl = (url || '').trim();
    localStorage.setItem(LS_KEY, baseUrl);
  }
  function getBaseUrl(){ return baseUrl; }

  // Эмуляция текущего состояния
  let mockState = {
    t_room: 4.2,
    t_evap: -8.7,
    door: false,
    mode: 'IDLE',
    relays: { compressor: false, fan: false, defrost: false, light: false },
    cfg: { setpoint: -3.0, hysteresis: 1.0, ton_min_s: 120, toff_min_s: 180, start_delay_s: 60 },
  };

  function randomStep(x, s){
    return x + (Math.random()*2-1) * s;
  }

  // Мок: периодически обновляем состояние как будто приходит с устройства
  setInterval(() => {
    mockState.t_room = parseFloat(randomStep(mockState.t_room, 0.05).toFixed(1));
    mockState.t_evap = parseFloat(randomStep(mockState.t_evap, 0.1).toFixed(1));
    if (mockState.t_room > mockState.cfg.setpoint + mockState.cfg.hysteresis/2) {
      mockState.relays.compressor = true;
      mockState.relays.fan = true;
      mockState.mode = 'COOL';
    } else if (mockState.t_room < mockState.cfg.setpoint - mockState.cfg.hysteresis/2) {
      mockState.relays.compressor = false;
      mockState.relays.fan = false;
      mockState.mode = 'IDLE';
    }
  }, 1500);

  async function apiGetState(){
    // TODO: заменить на fetch(`${baseUrl}/api/state`)
    await new Promise(r => setTimeout(r, 150));
    return JSON.parse(JSON.stringify(mockState));
  }

  async function apiToggleLight(){
    mockState.relays.light = !mockState.relays.light;
    return { ok: true, light: mockState.relays.light };
  }

  async function apiSaveConfig(cfg){
    mockState.cfg = Object.assign({}, mockState.cfg, cfg);
    return { ok: true };
  }

  // Моки для вкладки Controls
  async function apiDefrostStart(){ mockState.relays.defrost = true; return { ok:true }; }
  async function apiDefrostStop(){ mockState.relays.defrost = false; return { ok:true }; }
  async function apiDoorOpen(){ mockState.door = true; return { ok:true }; }
  async function apiDoorClose(){ mockState.door = false; return { ok:true }; }
  async function apiBeep(){ return { ok:true }; }

  // Моки воздействия на температуры (имитация)
  async function apiTempNudge(which, delta){
    if (which === 'room') mockState.t_room = parseFloat((mockState.t_room + delta).toFixed(1));
    if (which === 'evap') mockState.t_evap = parseFloat((mockState.t_evap + delta).toFixed(1));
    return { ok:true };
  }

  window.FridgeAPI = {
    setBaseUrl, getBaseUrl, apiGetState, apiToggleLight, apiSaveConfig,
    apiDefrostStart, apiDefrostStop, apiDoorOpen, apiDoorClose, apiBeep, apiTempNudge
  };
})();
