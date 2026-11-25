#pragma once

#include <Arduino.h>

const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Диагностика датчиков ESP</title>
<style>
:root{
    color-scheme:dark;
    --bg:#0f172a;
    --card:#172033;
    --secondary:#1e293b;
    --text:#e5e7eb;
    --muted:#94a3b8;
    --blue:#38bdf8;
    --green:#4ade80;
    --red:#fb7185;
    --border:#334155;
}
*{box-sizing:border-box}
body{
    margin:0;
    padding:18px;
    background:var(--bg);
    color:var(--text);
    font-family:Arial,sans-serif;
}
.container{max-width:1000px;margin:0 auto}
h1{margin:0 0 8px;font-size:27px}
h2{margin:0 0 15px;font-size:20px}
.subtitle{margin-bottom:20px;color:var(--muted)}
.card{
    margin-bottom:18px;
    padding:18px;
    background:var(--card);
    border:1px solid var(--border);
    border-radius:14px;
}
.row{
    display:flex;
    flex-wrap:wrap;
    align-items:end;
    gap:12px;
}
.field{display:flex;flex-direction:column;gap:7px}
label{color:var(--muted);font-size:14px}
select,input[type=text]{
    min-width:155px;
    padding:10px;
    border:1px solid var(--border);
    border-radius:8px;
    background:var(--secondary);
    color:var(--text);
    font-size:16px;
}
input[type=file]{
    max-width:100%;
    margin:10px 0;
    color:var(--muted);
}
button{
    padding:11px 17px;
    border:0;
    border-radius:8px;
    background:var(--blue);
    color:#082f49;
    font-size:15px;
    font-weight:bold;
    cursor:pointer;
}
button.secondary{background:#64748b;color:#fff}
button.danger{background:var(--red);color:#4c0519}
button:disabled{opacity:.55;cursor:wait}
.status{
    margin-top:14px;
    line-height:1.5;
    color:var(--muted);
    overflow-wrap:anywhere;
}
.success{color:var(--green)}
.error{color:var(--red)}
.info{line-height:1.8;color:var(--muted)}
.small{
    margin-top:14px;
    color:var(--muted);
    font-size:14px;
    line-height:1.6;
}
code{color:var(--blue)}
.table-wrap{overflow-x:auto}
table{
    width:100%;
    margin-top:14px;
    border-collapse:collapse;
}
th,td{
    padding:10px;
    border-bottom:1px solid var(--border);
    text-align:left;
    white-space:nowrap;
}
th{color:var(--muted)}
.values{
    display:grid;
    grid-template-columns:repeat(auto-fit,minmax(170px,1fr));
    gap:12px;
    margin-top:16px;
}
.value{
    padding:15px;
    border-radius:11px;
    background:var(--secondary);
}
.value-label{color:var(--muted);font-size:14px}
.value-data{
    margin-top:8px;
    font-size:25px;
    font-weight:bold;
}
progress{width:100%;height:20px;margin-top:12px}
.warning{
    padding:12px;
    border:1px solid #7f1d1d;
    border-radius:8px;
    color:#fecdd3;
    line-height:1.5;
}
</style>
</head>
<body>
<div class="container">

<h1>Диагностика датчиков</h1>
<div class="subtitle">
    I2C · 1-Wire · DHT · EEPROM · OTA
</div>

<section class="card">
    <h2>Плата и точка доступа</h2>
    <div class="info">
        Платформа: <code id="platform">—</code><br>
        Текущий SSID: <code id="currentSsid">—</code><br>
        Пароль: отсутствует<br>
        Адрес: <code id="apIp">http://192.168.4.1</code><br>
        Точка доступа: <span id="apState">—</span><br>
        EEPROM: <span id="eepromState">—</span>
    </div>
</section>

<section class="card">
    <h2>Имя точки доступа</h2>

    <div class="row">
        <div class="field">
            <label for="ssidInput">Новый SSID</label>
            <input
                id="ssidInput"
                type="text"
                maxlength="32"
                autocomplete="off">
        </div>

        <button id="ssidButton" onclick="saveSsid()">
            Сохранить SSID
        </button>
    </div>

    <div id="ssidStatus" class="status">
        Новое имя применяется после перезагрузки.
        Длина — от 1 до 32 байт.
    </div>
</section>

<section class="card">
    <h2>Настройка GPIO</h2>

    <div class="row">
        <div class="field">
            <label for="sdaPin">I2C SDA</label>
            <select id="sdaPin"></select>
        </div>

        <div class="field">
            <label for="sclPin">I2C SCL</label>
            <select id="sclPin"></select>
        </div>

        <div class="field">
            <label for="oneWirePin">DS18B20 / 1-Wire</label>
            <select id="oneWirePin"></select>
        </div>

        <div class="field">
            <label for="dhtPin">DHT</label>
            <select id="dhtPin"></select>
        </div>

        <div class="field">
            <label for="dhtType">Тип DHT</label>
            <select id="dhtType">
                <option value="11">DHT11</option>
                <option value="22">DHT22</option>
            </select>
        </div>
    </div>

    <div class="row" style="margin-top:14px">
        <button id="pinsButton" onclick="savePins()">
            Сохранить настройки
        </button>

        <button
            id="rebootButton"
            class="danger"
            onclick="rebootDevice()">
            Перезагрузить плату
        </button>
    </div>

    <div id="pinsStatus" class="status">
        Настройки сохраняются в EEPROM и восстанавливаются после перезагрузки.
    </div>

    <div class="small">
        Все датчики должны питаться от напряжения, допустимого для их модулей.
        Для сигнальных линий ESP используется 3,3 В.<br>
        DS18B20: между DATA и 3,3 В нужен резистор около 4,7 кОм.<br>
        I2C: на SDA и SCL нужны подтяжки к 3,3 В, обычно 4,7 кОм.
    </div>
</section>

<section class="card">
    <h2>I2C-сканер</h2>

    <button id="scanButton" onclick="runScan()">
        Сканировать I2C
    </button>

    <div id="scanStatus" class="status">
        Сканирование ещё не выполнялось.
    </div>

    <div class="table-wrap">
        <table>
            <thead>
            <tr>
                <th>HEX</th>
                <th>DEC</th>
                <th>Комментарий</th>
            </tr>
            </thead>
            <tbody id="scanResults">
            <tr><td colspan="3">Нет результатов</td></tr>
            </tbody>
        </table>
    </div>
</section>

<section class="card">
    <h2>Автоматическая диагностика I2C-датчиков</h2>

    <button id="i2cSensorButton" onclick="runI2cSensors()">
        Определить и прочитать датчики
    </button>

    <div id="i2cSensorStatus" class="status">
        Поддерживаются BME280, BMP280, SHT30/SHT31 и SHT40/SHT4x.
    </div>

    <div class="table-wrap">
        <table>
            <thead>
            <tr>
                <th>Датчик</th>
                <th>Адрес</th>
                <th>Температура</th>
                <th>Давление</th>
                <th>Влажность</th>
            </tr>
            </thead>
            <tbody id="i2cSensorResults">
            <tr><td colspan="5">Нет результатов</td></tr>
            </tbody>
        </table>
    </div>
</section>

<section class="card">
    <h2>DS18B20 / 1-Wire</h2>

    <button id="dsButton" onclick="runDs18b20()">
        Найти и прочитать DS18B20
    </button>

    <div id="dsStatus" class="status">
        Диагностика ещё не выполнялась.
    </div>

    <div class="table-wrap">
        <table>
            <thead>
            <tr>
                <th>ROM-адрес</th>
                <th>Температура</th>
            </tr>
            </thead>
            <tbody id="dsResults">
            <tr><td colspan="2">Нет результатов</td></tr>
            </tbody>
        </table>
    </div>
</section>

<section class="card">
    <h2>DHT11 / DHT22</h2>

    <button id="dhtButton" onclick="runDht()">
        Прочитать DHT
    </button>

    <div id="dhtStatus" class="status">
        Чтение занимает около 2 секунд.
    </div>

    <div class="values">
        <div class="value">
            <div class="value-label">Температура</div>
            <div id="dhtTemperature" class="value-data">—</div>
        </div>

        <div class="value">
            <div class="value-label">Влажность</div>
            <div id="dhtHumidity" class="value-data">—</div>
        </div>
    </div>
</section>

<section class="card">
    <h2>Обновление прошивки по .bin</h2>

    <div class="warning">
        Загружайте только firmware.bin для этой модели платы.
        Не отключайте питание во время записи.
        После успешной загрузки плата автоматически перезагрузится.
    </div>

    <input id="firmwareFile" type="file" accept=".bin">

    <div class="row">
        <button
            id="firmwareButton"
            class="danger"
            onclick="uploadFirmware()">
            Загрузить и прошить
        </button>
    </div>

    <progress id="firmwareProgress" value="0" max="100"></progress>

    <div id="firmwareStatus" class="status">
        Файл ещё не выбран.
    </div>
</section>

</div>

<script>
const byId = id => document.getElementById(id);

function setStatus(id,text,type=''){
    const element=byId(id);
    element.textContent=text;
    element.className='status'+(type?' '+type:'');
}

async function requestJson(url,options={}){
    const response=await fetch(url,{
        cache:'no-store',
        ...options
    });

    let data;

    try{
        data=await response.json();
    }catch(error){
        throw new Error(
            'Некорректный ответ платы, HTTP '+response.status
        );
    }

    if(!response.ok){
        throw new Error(data.error||('HTTP '+response.status));
    }

    return data;
}

function fillSelect(id,pins,selected){
    const select=byId(id);
    select.innerHTML='';

    for(const pin of pins){
        const option=document.createElement('option');
        option.value=pin;
        option.textContent='GPIO'+pin;
        select.appendChild(option);
    }

    select.value=selected;
}

function clearTable(id,columns,text){
    byId(id).innerHTML=
        '<tr><td colspan="'+columns+'">'+text+'</td></tr>';
}

function appendCell(row,text,code=false){
    const cell=document.createElement('td');

    if(code){
        const element=document.createElement('code');
        element.textContent=text;
        cell.appendChild(element);
    }else{
        cell.textContent=text;
    }

    row.appendChild(cell);
}

function formatNumber(value,unit){
    return value===null||value===undefined?'—':value+unit;
}

async function loadState(){
    try{
        const data=await requestJson('/api/state');
        const pins=data.allowedPins;

        byId('platform').textContent=data.platform;
        byId('currentSsid').textContent=data.ssid;
        byId('ssidInput').value=data.ssid;
        byId('apIp').textContent='http://'+data.apIp;

        byId('apState').textContent=
            data.apStarted?'запущена':'ошибка запуска';

        byId('apState').className=
            data.apStarted?'success':'error';

        byId('eepromState').textContent=
            data.eepromReady?'доступна':'ошибка инициализации';

        byId('eepromState').className=
            data.eepromReady?'success':'error';

        fillSelect('sdaPin',pins,data.sda);
        fillSelect('sclPin',pins,data.scl);
        fillSelect('oneWirePin',pins,data.oneWirePin);
        fillSelect('dhtPin',pins,data.dhtPin);

        byId('dhtType').value=String(data.dhtType);
    }catch(error){
        setStatus('pinsStatus',error.message,'error');
    }
}

async function saveSsid(){
    const button=byId('ssidButton');
    const ssid=byId('ssidInput').value.trim();

    if(!ssid){
        setStatus('ssidStatus','SSID не может быть пустым.','error');
        return;
    }

    button.disabled=true;
    setStatus('ssidStatus','Сохранение...');

    try{
        const body=new URLSearchParams();
        body.set('ssid',ssid);

        const data=await requestJson('/api/ssid',{
            method:'POST',
            body:body
        });

        byId('currentSsid').textContent=data.ssid;

        setStatus(
            'ssidStatus',
            'SSID сохранён. Перезагрузите плату. '+
            'После перезагрузки подключайтесь к сети "'+data.ssid+'".',
            'success'
        );
    }catch(error){
        setStatus('ssidStatus',error.message,'error');
    }finally{
        button.disabled=false;
    }
}

async function savePins(){
    const button=byId('pinsButton');
    const sda=byId('sdaPin').value;
    const scl=byId('sclPin').value;

    if(sda===scl){
        setStatus(
            'pinsStatus',
            'SDA и SCL должны использовать разные GPIO.',
            'error'
        );
        return;
    }

    button.disabled=true;
    setStatus('pinsStatus','Сохранение в EEPROM...');

    try{
        const body=new URLSearchParams();

        body.set('sda',sda);
        body.set('scl',scl);
        body.set('oneWirePin',byId('oneWirePin').value);
        body.set('dhtPin',byId('dhtPin').value);
        body.set('dhtType',byId('dhtType').value);

        await requestJson('/api/config',{
            method:'POST',
            body:body
        });

        clearTable(
            'scanResults',
            3,
            'Настройки изменены, повторите сканирование'
        );

        clearTable(
            'i2cSensorResults',
            5,
            'Настройки изменены, повторите диагностику'
        );

        setStatus(
            'pinsStatus',
            'Настройки сохранены. Теперь можно перезагрузить плату.',
            'success'
        );
    }catch(error){
        setStatus('pinsStatus',error.message,'error');
    }finally{
        button.disabled=false;
    }
}

async function rebootDevice(){
    if(!confirm('Перезагрузить плату?')){
        return;
    }

    try{
        await requestJson('/api/reboot',{method:'POST'});
    }catch(error){
        // Соединение может разорваться сразу после команды.
    }

    setStatus(
        'pinsStatus',
        'Плата перезагружается. Подождите несколько секунд '+
        'и снова подключитесь к точке доступа.',
        'success'
    );
}

async function runScan(){
    const button=byId('scanButton');

    button.disabled=true;
    setStatus('scanStatus','Сканирование...');

    try{
        const data=await requestJson('/api/scan',{method:'POST'});
        const results=byId('scanResults');

        results.innerHTML='';

        if(data.devices.length===0){
            clearTable('scanResults',3,'Устройства не обнаружены');
        }else{
            for(const device of data.devices){
                const row=document.createElement('tr');

                appendCell(row,device.address,true);
                appendCell(row,String(device.decimal));
                appendCell(row,device.note||'—');

                results.appendChild(row);
            }
        }

        let message=
            'Найдено устройств: '+data.count+
            '. SDA=GPIO'+data.sda+
            ', SCL=GPIO'+data.scl+
            '. Время: '+data.durationMs+' мс.';

        if(data.busErrors>0){
            message+=' Ошибок шины: '+data.busErrors+'.';
        }

        setStatus(
            'scanStatus',
            message,
            data.count>0?'success':''
        );
    }catch(error){
        setStatus('scanStatus',error.message,'error');
    }finally{
        button.disabled=false;
    }
}

async function runI2cSensors(){
    const button=byId('i2cSensorButton');

    button.disabled=true;
    setStatus('i2cSensorStatus','Определение датчиков...');

    try{
        const data=await requestJson('/api/i2c-sensors',{
            method:'POST'
        });

        const results=byId('i2cSensorResults');
        results.innerHTML='';

        if(data.sensors.length===0){
            clearTable(
                'i2cSensorResults',
                5,
                'Поддерживаемые датчики не найдены'
            );
        }else{
            for(const sensor of data.sensors){
                const row=document.createElement('tr');

                appendCell(row,sensor.name);
                appendCell(row,sensor.address,true);

                appendCell(
                    row,
                    formatNumber(sensor.temperature,' °C')
                );

                appendCell(
                    row,
                    formatNumber(sensor.pressure,' hPa')
                );

                appendCell(
                    row,
                    formatNumber(sensor.humidity,' %')
                );

                results.appendChild(row);
            }
        }

        setStatus(
            'i2cSensorStatus',
            data.message,
            data.count>0?'success':'error'
        );
    }catch(error){
        setStatus('i2cSensorStatus',error.message,'error');
    }finally{
        button.disabled=false;
    }
}

async function runDs18b20(){
    const button=byId('dsButton');

    button.disabled=true;
    setStatus('dsStatus','Поиск и измерение...');

    try{
        const data=await requestJson('/api/ds18b20',{
            method:'POST'
        });

        const results=byId('dsResults');
        results.innerHTML='';

        if(data.devices.length===0){
            clearTable('dsResults',2,'Датчики не найдены');
        }else{
            for(const device of data.devices){
                const row=document.createElement('tr');

                appendCell(row,device.rom,true);
                appendCell(
                    row,
                    formatNumber(device.temperature,' °C')
                );

                results.appendChild(row);
            }
        }

        setStatus(
            'dsStatus',
            data.message+' GPIO'+data.pin+'.',
            data.count>0?'success':'error'
        );
    }catch(error){
        setStatus('dsStatus',error.message,'error');
    }finally{
        button.disabled=false;
    }
}

async function runDht(){
    const button=byId('dhtButton');

    button.disabled=true;
    byId('dhtTemperature').textContent='—';
    byId('dhtHumidity').textContent='—';

    setStatus('dhtStatus','Ожидание и чтение датчика...');

    try{
        const data=await requestJson('/api/dht',{method:'POST'});

        byId('dhtTemperature').textContent=
            formatNumber(data.temperature,' °C');

        byId('dhtHumidity').textContent=
            formatNumber(data.humidity,' %');

        setStatus(
            'dhtStatus',
            data.message+' Тип: DHT'+data.dhtType+
            ', GPIO'+data.pin+'.',
            data.ok?'success':'error'
        );
    }catch(error){
        setStatus('dhtStatus',error.message,'error');
    }finally{
        button.disabled=false;
    }
}

function uploadFirmware(){
    const input=byId('firmwareFile');
    const button=byId('firmwareButton');
    const progress=byId('firmwareProgress');

    if(!input.files.length){
        setStatus('firmwareStatus','Выберите .bin-файл.','error');
        return;
    }

    const file=input.files[0];

    if(!file.name.toLowerCase().endsWith('.bin')){
        setStatus(
            'firmwareStatus',
            'Разрешены только файлы .bin.',
            'error'
        );
        return;
    }

    if(!confirm(
        'Начать прошивку?\n\n'+
        'Не отключайте питание и не закрывайте страницу.'
    )){
        return;
    }

    button.disabled=true;
    progress.value=0;

    setStatus('firmwareStatus','Загрузка файла...');

    const form=new FormData();
    form.append('firmware',file,file.name);

    const xhr=new XMLHttpRequest();

    xhr.open('POST','/update',true);

    xhr.upload.onprogress=function(event){
        if(event.lengthComputable){
            progress.value=Math.round(
                event.loaded*100/event.total
            );

            setStatus(
                'firmwareStatus',
                'Загружено '+progress.value+'%. '+
                'Не отключайте питание.'
            );
        }
    };

    xhr.onload=function(){
        button.disabled=false;

        let data={};

        try{
            data=JSON.parse(xhr.responseText);
        }catch(error){
            setStatus(
                'firmwareStatus',
                'Неизвестный ответ платы, HTTP '+xhr.status+'.',
                'error'
            );
            return;
        }

        if(xhr.status===200&&data.ok){
            progress.value=100;

            setStatus(
                'firmwareStatus',
                'Прошивка записана. Плата перезагружается. '+
                'Через несколько секунд снова подключитесь к точке доступа.',
                'success'
            );
        }else{
            setStatus(
                'firmwareStatus',
                data.error||('Ошибка HTTP '+xhr.status),
                'error'
            );
        }
    };

    xhr.onerror=function(){
        button.disabled=false;

        setStatus(
            'firmwareStatus',
            'Соединение разорвано. Если загрузка достигла 100%, '+
            'подождите перезагрузки платы. Иначе повторите попытку.',
            'error'
        );
    };

    xhr.send(form);
}

loadState();
</script>
</body>
</html>
)=====";