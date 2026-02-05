#ifndef __WEB_HTML_H__
#define __WEB_HTML_H__

// 主配置页面HTML
static const char WEB_CONFIG_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Super Dial 配置</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 50%, #0f3460 100%);
            min-height: 100vh;
            color: #fff;
            padding: 20px;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
        }
        h1 {
            text-align: center;
            margin-bottom: 30px;
            font-size: 2em;
            background: linear-gradient(90deg, #00d4ff, #7b2cbf);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
        }
        .card {
            background: rgba(255,255,255,0.1);
            backdrop-filter: blur(10px);
            border-radius: 16px;
            padding: 24px;
            margin-bottom: 20px;
            border: 1px solid rgba(255,255,255,0.1);
        }
        .card h2 {
            font-size: 1.2em;
            margin-bottom: 16px;
            color: #00d4ff;
        }
        .bg-grid {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 15px;
            margin-bottom: 20px;
        }
        .bg-slot {
            background: rgba(0,0,0,0.3);
            border-radius: 8px;
            padding: 10px;
            text-align: center;
            border: 2px solid transparent;
            cursor: pointer;
            transition: all 0.2s;
        }
        .bg-slot.active {
            border-color: #00d4ff;
            background: rgba(0,212,255,0.1);
        }
        .bg-slot:hover {
            transform: translateY(-2px);
        }
        .bg-slot input { display: none; }
        .bg-slot-label { display: block; margin-bottom: 5px; color: #aaa; font-size: 0.9em; }
        .bg-btn {
            font-size: 0.8em;
            padding: 5px 10px;
            margin-top: 5px;
            width: 100%;
        }
        .info-row {
            display: flex;
            justify-content: space-between;
            padding: 10px 0;
            border-bottom: 1px solid rgba(255,255,255,0.1);
        }
        .info-row:last-child { border-bottom: none; }
        .info-label { color: #aaa; }
        .info-value { color: #fff; font-weight: 500; }
        .form-group {
            margin-bottom: 16px;
        }
        label {
            display: block;
            margin-bottom: 8px;
            color: #aaa;
            font-size: 0.9em;
        }
        input, select {
            width: 100%;
            padding: 12px;
            border: 1px solid rgba(255,255,255,0.2);
            border-radius: 8px;
            background: rgba(0,0,0,0.3);
            color: #fff;
            font-size: 1em;
        }
        input:focus, select:focus {
            outline: none;
            border-color: #00d4ff;
        }
        .btn {
            padding: 12px 24px;
            border: none;
            border-radius: 8px;
            font-size: 1em;
            cursor: pointer;
            transition: all 0.3s;
        }
        .btn-primary {
            background: linear-gradient(90deg, #00d4ff, #7b2cbf);
            color: #fff;
        }
        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 4px 15px rgba(0,212,255,0.4);
        }
        .btn-secondary {
            background: rgba(255,255,255,0.1);
            color: #fff;
            margin-left: 10px;
        }
        .hid-slot {
            background: rgba(0,0,0,0.2);
            border-radius: 12px;
            padding: 16px;
            margin-bottom: 12px;
        }
        .hid-slot-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 12px;
        }
        .hid-slot-title {
            font-weight: 600;
            color: #00d4ff;
        }
        .key-input-group {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 10px;
        }
        .upload-area {
            border: 2px dashed rgba(255,255,255,0.3);
            border-radius: 12px;
            padding: 40px;
            text-align: center;
            cursor: pointer;
            transition: all 0.3s;
        }
        .upload-area:hover {
            border-color: #00d4ff;
            background: rgba(0,212,255,0.1);
        }
        .upload-area input {
            display: none;
        }
        .status {
            padding: 12px;
            border-radius: 8px;
            margin-top: 16px;
            display: none;
        }
        .status.success {
            display: block;
            background: rgba(0,255,100,0.2);
            color: #0f6;
        }
        .status.error {
            display: block;
            background: rgba(255,0,0,0.2);
            color: #f66;
        }
        .time-display {
            font-size: 2em;
            text-align: center;
            padding: 20px;
            font-family: 'Courier New', monospace;
            color: #00d4ff;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎛️ Super Dial 配置</h1>
        
        <div class="card">
            <h2>📊 设备状态</h2>
            <div class="time-display" id="timeDisplay">--:--:--</div>
            <div class="info-row">
                <span class="info-label">设备ID</span>
                <span class="info-value" id="deviceId">加载中...</span>
            </div>
            <div class="info-row">
                <span class="info-label">固件版本</span>
                <span class="info-value" id="fwVersion">加载中...</span>
            </div>
            <div class="info-row">
                <span class="info-label">存储空间</span>
                <span class="info-value" id="storage">加载中...</span>
            </div>
        </div>

        <div class="card">
            <h2>⌨️ 自定义快捷键</h2>
            <div id="hidSlots"></div>
            <button class="btn btn-primary" onclick="saveHidConfig()">💾 保存配置</button>
            <button class="btn btn-secondary" onclick="loadHidConfig()">🔄 重新加载</button>
            <div class="status" id="hidStatus"></div>
        </div>

        <div class="card">
            <h2>🚀 固件升级</h2>
            <div class="upload-area" onclick="document.getElementById('fwInput').click()">
                <input type="file" id="fwInput" accept=".bin" onchange="uploadFirmware(this)">
                <p>📤 点击上传固件 (.bin)</p>
            </div>
            <div class="status" id="fwStatus"></div>
        </div>

        <div class="card">
            <h2>🖼️ 壁纸管理</h2>
            <p style="color:#aaa;margin-bottom:15px;font-size:0.9em">点击槽位选择当前壁纸，点击上传按钮更新壁纸 (240x240 PNG)</p>
            <div class="bg-grid" id="bgGrid"></div>
            <button class="btn btn-primary" onclick="saveBgConfig()">💾 保存选中壁纸设置</button>
            <div class="status" id="bgStatus"></div>
        </div>

        <div class="card">
            <h2>📶 WiFi配置</h2>
            <div class="form-group">
                <label>WiFi名称 (SSID)</label>
                <input type="text" id="wifiSsid" placeholder="输入WiFi名称">
            </div>
            <div class="form-group">
                <label>WiFi密码</label>
                <input type="password" id="wifiPass" placeholder="输入WiFi密码">
            </div>
            <button class="btn btn-primary" onclick="saveWifiConfig()">📶 保存WiFi配置</button>
            <div class="status" id="wifiStatus"></div>
        </div>
    </div>

    <script>
        const HID_TYPES = ['键盘', '鼠标', 'Surface Dial', '多媒体控制'];
        const MAX_SLOTS = 8;
        const CMD_SLOTS = 8;
        let currentBgIndex = 0;

        function createBgSlot(index) {
            const isActive = index === currentBgIndex;
            return `
                <div class="bg-slot ${isActive ? 'active' : ''}" onclick="selectBg(${index})" id="bgSlot${index}">
                    <span class="bg-slot-label">壁纸 ${index + 1}</span>
                    <button class="btn btn-secondary bg-btn" onclick="event.stopPropagation();document.getElementById('bgInput${index}').click()">📤 上传</button>
                    <input type="file" id="bgInput${index}" accept="image/png" onchange="uploadBg(${index}, this)">
                </div>
            `;
        }
        
        function renderBgSlots() {
            let html = '';
            for(let i=0; i<8; i++) {
                html += createBgSlot(i);
            }
            document.getElementById('bgGrid').innerHTML = html;
        }
        
        function selectBg(index) {
            currentBgIndex = index;
            renderBgSlots(); // 重绘最简单
        }

        function createHidSlot(index, data = {}) {
            return `
                <div class="hid-slot" id="slot${index}">
                    <div class="hid-slot-header">
                        <span class="hid-slot-title">槽位 ${index + 1}</span>
                        <label><input type="checkbox" ${data.enabled ? 'checked' : ''} id="enable${index}"> 启用</label>
                    </div>
                    <div class="form-group">
                        <label>名称</label>
                        <input type="text" id="name${index}" value="${data.name || ''}" placeholder="快捷键名称">
                    </div>
                    <div class="form-group">
                        <label>HID类型</label>
                        <select id="type${index}">
                            ${HID_TYPES.map((t, i) => `<option value="${i}" ${data.hid_type == i ? 'selected' : ''}>${t}</option>`).join('')}
                        </select>
                    </div>
                    <div class="key-input-group">
                        <div class="form-group">
                            <label>顺时针旋转</label>
                            <input type="text" id="cw${index}" value="${data.cw_key || ''}" placeholder="例: CTRL+C">
                        </div>
                        <div class="form-group">
                            <label>逆时针旋转</label>
                            <input type="text" id="ccw${index}" value="${data.ccw_key || ''}" placeholder="例: CTRL+V">
                        </div>
                        <div class="form-group">
                            <label>单击</label>
                            <input type="text" id="click${index}" value="${data.click_key || ''}" placeholder="例: ENTER">
                        </div>
                    </div>
                </div>
            `;
        }

        function loadHidConfig() {
            fetch('/api/config')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('deviceId').textContent = data.device_id || 'N/A';
                    document.getElementById('fwVersion').textContent = data.fw_version || 'N/A';
                    document.getElementById('storage').textContent = data.storage || 'N/A';
                    
                    if(data.bg_index !== undefined) {
                        currentBgIndex = data.bg_index;
                        renderBgSlots();
                    }
                    
                    let slotsHtml = '';
                    for (let i = 0; i < MAX_SLOTS; i++) {
                        slotsHtml += createHidSlot(i, data.hid_slots?.[i] || {});
                    }
                    document.getElementById('hidSlots').innerHTML = slotsHtml;
                })
                .catch(e => console.error('加载配置失败:', e));
        }

        function saveHidConfig() {
            const slots = [];
            for (let i = 0; i < MAX_SLOTS; i++) {
                slots.push({
                    enabled: document.getElementById(`enable${i}`).checked,
                    name: document.getElementById(`name${i}`).value,
                    hid_type: parseInt(document.getElementById(`type${i}`).value),
                    cw_key: document.getElementById(`cw${i}`).value,
                    ccw_key: document.getElementById(`ccw${i}`).value,
                    click_key: document.getElementById(`click${i}`).value
                });
            }
            
            fetch('/api/config', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({hid_slots: slots})
            })
            .then(r => r.json())
            .then(data => showStatus('hidStatus', data.success, data.message))
            .catch(e => showStatus('hidStatus', false, '保存失败: ' + e));
        }

        function saveBgConfig() {
            fetch('/api/config', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({bg_index: currentBgIndex})
            })
            .then(r => r.json())
            .then(data => showStatus('bgStatus', data.success, data.message))
            .catch(e => showStatus('bgStatus', false, '保存失败: ' + e));
        }

        function uploadBg(index, input) {
            if (!input.files[0]) return;
            const file = input.files[0];
            const reader = new FileReader();
            
            showStatus('bgStatus', true, `正在处理槽位 ${index+1} 图片...`);
            
            reader.onload = function(e) {
                const img = new Image();
                img.onload = function() {
                    const canvas = document.createElement('canvas');
                    canvas.width = 240;
                    canvas.height = 240;
                    const ctx = canvas.getContext('2d');
                    ctx.drawImage(img, 0, 0, 240, 240);
                    
                    const imgData = ctx.getImageData(0, 0, 240, 240);
                    const pixels = imgData.data;
                    const buffer = new Uint8Array(240 * 240 * 2);
                    
                    let dataIndex = 0;
                    for (let i = 0; i < pixels.length; i += 4) {
                        const r = pixels[i];
                        const g = pixels[i + 1];
                        const b = pixels[i + 2];
                        const rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
                        buffer[dataIndex++] = rgb565 & 0xFF;
                        buffer[dataIndex++] = (rgb565 >> 8) & 0xFF;
                    }
                    
                    const blob = new Blob([buffer], { type: 'application/octet-stream' });
                    
                    fetch('/api/upload', {
                        method: 'POST',
                        headers: {
                            'Content-Type': 'application/octet-stream',
                            'X-File-Name': `bg_${index}.bin`
                        },
                        body: blob
                    })
                    .then(r => r.json())
                    .then(data => showStatus('bgStatus', data.success, data.message))
                    .catch(e => showStatus('bgStatus', false, '上传失败: ' + e));
                };
                img.src = e.target.result;
            };
            reader.readAsDataURL(file);
        }

        function uploadFirmware(input) {
            if (!input.files[0]) return;
            const file = input.files[0];
            showStatus('fwStatus', true, '正在上传固件 (请勿断电)...');
            
            fetch('/api/update', {
                method: 'POST',
                headers: {'Content-Type': 'application/octet-stream'},
                body: file
            })
            .then(r => r.json())
            .then(data => {
                showStatus('fwStatus', data.success, data.message);
                if(data.success) {
                    setTimeout(() => { location.reload(); }, 5000);
                }
            })
            .catch(e => showStatus('fwStatus', false, '升级失败: ' + e));
        }

        function saveWifiConfig() {
            const ssid = document.getElementById('wifiSsid').value;
            const pass = document.getElementById('wifiPass').value;
            
            fetch('/api/wifi', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({ssid, password: pass})
            })
            .then(r => r.json())
            .then(data => showStatus('wifiStatus', data.success, data.message))
            .catch(e => showStatus('wifiStatus', false, '保存失败: ' + e));
        }

        function showStatus(id, success, message) {
            const el = document.getElementById(id);
            el.className = 'status ' + (success ? 'success' : 'error');
            el.textContent = message;
            setTimeout(() => el.style.display = 'none', 3000);
        }

        function updateTime() {
            fetch('/api/time')
                .then(r => r.json())
                .then(data => {
                    document.getElementById('timeDisplay').textContent = data.time || '--:--:--';
                })
                .catch(() => {});
        }

        // 初始化
        loadHidConfig();
        updateTime();
        setInterval(updateTime, 1000);
    </script>
</body>
</html>
)rawliteral";

#endif // __WEB_HTML_H__
