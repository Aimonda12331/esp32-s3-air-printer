#pragma once

static const char INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html lang="vi">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Air Printer - Quản lý in ấn</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
        body { background: #f0f2f5; padding: 16px; color: #333; display: flex; justify-content: center; align-items: flex-start; min-height: 100vh; }
        .app-container { background: white; width: 100%; max-width: 520px; border-radius: 16px; box-shadow: 0 8px 30px rgba(0,0,0,0.08); overflow: hidden; margin-top: 10px; }
        
        /* Top Navigation Bar */
        .navbar { display: flex; background: #fff; border-bottom: 2px solid #eee; }
        .nav-tab { flex: 1; padding: 14px 10px; text-align: center; font-weight: 600; font-size: 14px; color: #5f6368; cursor: pointer; border-bottom: 3px solid transparent; transition: 0.2s; }
        .nav-tab.active { color: #1a73e8; border-bottom-color: #1a73e8; background: #f8fafd; }
        .nav-tab:hover { background: #f1f3f4; }
        
        .tab-content { display: none; padding: 22px; }
        .tab-content.active { display: block; }
        
        /* Device Info Header */
        .device-badge { display: flex; align-items: center; justify-content: space-between; background: #f1f3f4; padding: 10px 14px; border-radius: 10px; margin-bottom: 18px; font-size: 13px; }
        .status-dot { width: 9px; height: 9px; border-radius: 50%; display: inline-block; margin-right: 6px; }
        .status-online { background: #1e8e3e; box-shadow: 0 0 6px #1e8e3e; }
        .status-ap { background: #f9ab00; box-shadow: 0 0 6px #f9ab00; }

        /* Upload Zone */
        .drop-zone { border: 2px dashed #1a73e8; border-radius: 12px; padding: 30px 16px; text-align: center; background: #f8fbff; cursor: pointer; transition: 0.2s; }
        .drop-zone:hover, .drop-zone.dragover { background: #e8f0fe; border-color: #1557b0; }
        .drop-icon { font-size: 38px; margin-bottom: 10px; }
        .drop-text { font-size: 15px; font-weight: 600; color: #1a73e8; margin-bottom: 4px; }
        .drop-hint { font-size: 12px; color: #70757a; }

        /* Progress Bar */
        .progress-container { display: none; margin-top: 15px; background: #e0e0e0; border-radius: 8px; overflow: hidden; height: 10px; }
        .progress-bar { height: 100%; width: 0%; background: #1a73e8; transition: width 0.2s; }

        /* File List */
        .file-list-title { font-size: 14px; font-weight: 700; color: #444; margin: 24px 0 12px 0; display: flex; justify-content: space-between; align-items: center; }
        .file-item { display: flex; align-items: center; justify-content: space-between; padding: 12px; border: 1px solid #e0e0e0; border-radius: 10px; margin-bottom: 8px; background: #fafafa; }
        .file-info { display: flex; align-items: center; overflow: hidden; }
        .file-icon { font-size: 22px; margin-right: 12px; flex-shrink: 0; }
        .file-name { font-size: 14px; font-weight: 600; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; max-width: 220px; }
        .file-size { font-size: 12px; color: #70757a; margin-top: 2px; }
        .file-actions { display: flex; gap: 6px; flex-shrink: 0; }
        
        .btn-action { padding: 6px 12px; border-radius: 6px; border: none; font-size: 12px; font-weight: 600; cursor: pointer; }
        .btn-print { background: #1e8e3e; color: white; }
        .btn-print:hover { background: #137333; }
        .btn-del { background: #fce8e6; color: #c5221f; }
        .btn-del:hover { background: #fad2cf; }

        /* Form elements for Wi-Fi */
        .form-group { margin-bottom: 14px; }
        label { display: block; font-weight: 600; margin-bottom: 5px; font-size: 13px; color: #444; }
        select, input[type="text"], input[type="password"] { width: 100%; padding: 11px 14px; border: 1.5px solid #dadce0; border-radius: 8px; font-size: 14px; outline: none; }
        select:focus, input:focus { border-color: #1a73e8; }
        .hint { font-size: 12px; color: #666; margin-top: 4px; }
        .preview-domain { font-weight: 600; color: #1e8e3e; }
        
        button.btn-primary { width: 100%; padding: 12px; background: #1a73e8; color: white; border: none; border-radius: 8px; font-size: 15px; font-weight: 600; cursor: pointer; margin-top: 8px; }
        button.btn-primary:hover { background: #1557b0; }
        button.btn-secondary { width: 100%; padding: 10px; background: #f1f3f4; color: #3c4043; border: 1px solid #dadce0; border-radius: 8px; font-size: 14px; font-weight: 600; cursor: pointer; margin-bottom: 12px; }

        .status-box { display: none; margin-top: 16px; padding: 14px; border-radius: 10px; text-align: center; font-size: 14px; }
        .success { background: #e6f4ea; color: #137333; border: 1px solid #a8dab5; }
        .loading { background: #e8f0fe; color: #1a73e8; border: 1px solid #aecbfa; }
        .error { background: #fce8e6; color: #c5221f; border: 1px solid #f5c2c7; }
        .spinner { border: 3px solid rgba(26,115,232,0.2); border-top: 3px solid #1a73e8; border-radius: 50%; width: 18px; height: 18px; animation: spin 0.8s linear infinite; display: inline-block; vertical-align: middle; margin-right: 6px; }
        @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
    </style>
</head>
<body>
    <div class="app-container">
        <!-- Tab Navigation Bar -->
        <div class="navbar">
            <div id="tabBtnPrint" class="nav-tab active" onclick="switchTab('print')">📁 In Ấn & Tải File</div>
            <div id="tabBtnWifi" class="nav-tab" onclick="switchTab('wifi')">⚙️ Cài Đặt Wi-Fi</div>
        </div>

        <!-- TAB 1: UPLOAD & IN ẤN -->
        <div id="tabPrint" class="tab-content active">
            <div class="device-badge">
                <span>Máy in: <b id="lblDevName">air-printer</b></span>
                <span id="lblWifiBadge"><span class="status-dot status-ap"></span>Chế độ AP</span>
            </div>

            <!-- Upload Zone -->
            <div class="drop-zone" id="dropZone" onclick="document.getElementById('fileInput').click()">
                <div class="drop-icon">📤</div>
                <div class="drop-text">Kéo thả file hoặc thư mục vào đây</div>
                <div class="drop-hint">Hỗ trợ file in: PDF, TXT, DOCX, PNG, JPG, G-CODE...</div>
                <input type="file" id="fileInput" multiple style="display:none;" onchange="uploadFiles(this.files)">
            </div>

            <!-- Upload Progress Bar -->
            <div id="progressContainer" class="progress-container">
                <div id="progressBar" class="progress-bar"></div>
            </div>

            <div id="uploadStatus" class="status-box"></div>

            <!-- Danh sách file hàng đợi -->
            <div class="file-list-title">
                <span>Hàng đợi tài liệu in (<span id="fileCount">0</span>)</span>
                <span style="font-size:12px; color:#1a73e8; cursor:pointer;" onclick="loadFileList()">🔄 Làm mới</span>
            </div>

            <div id="fileListContainer">
                <!-- Danh sách file được nạp tự động qua JavaScript -->
            </div>
        </div>

        <!-- TAB 2: CÀI ĐẶT WI-FI & TÊN MÁY IN -->
        <div id="tabWifi" class="tab-content">
            <div class="form-group">
                <label for="dev_name">1. Tên máy in mong muốn (mDNS):</label>
                <input type="text" id="dev_name" placeholder="Ví dụ: mayin-tang1" oninput="updateDomainPreview(this.value)">
                <p class="hint">Địa chỉ truy cập sẽ là: <span id="domain_preview" class="preview-domain">http://air-printer.local</span></p>
            </div>

            <div style="border-top:1px solid #eee; margin:16px 0;"></div>

            <label style="margin-bottom:8px;">2. Chọn mạng Wi-Fi nhà:</label>
            <button class="btn-secondary" onclick="scanWifi()">🔍 Quét danh sách Wi-Fi</button>

            <div class="form-group">
                <select id="ssid_select" onchange="document.getElementById('ssid').value = this.value">
                    <option value="">-- Bấm "Quét Wi-Fi" để tìm mạng --</option>
                </select>
            </div>

            <div class="form-group">
                <label for="ssid">Hoặc nhập thủ công SSID:</label>
                <input type="text" id="ssid" placeholder="Tên Wi-Fi...">
            </div>

            <div class="form-group">
                <label for="password">Mật khẩu Wi-Fi:</label>
                <input type="password" id="password" placeholder="Mật khẩu Wi-Fi...">
            </div>

            <button class="btn-primary" onclick="saveAndConnect()">🚀 LƯU & KẾT NỐI</button>

            <div id="wifiStatusBox" class="status-box"></div>
        </div>
    </div>

    <script>
        function switchTab(tab) {
            document.getElementById('tabPrint').className = 'tab-content' + (tab === 'print' ? ' active' : '');
            document.getElementById('tabWifi').className = 'tab-content' + (tab === 'wifi' ? ' active' : '');
            document.getElementById('tabBtnPrint').className = 'nav-tab' + (tab === 'print' ? ' active' : '');
            document.getElementById('tabBtnWifi').className = 'nav-tab' + (tab === 'wifi' ? ' active' : '');
        }

        function slugify(text) {
            return text.toString().toLowerCase()
                .normalize('NFD').replace(/[\u0300-\u036f]/g, '')
                .replace(/đ/g, 'd')
                .replace(/\s+/g, '-')
                .replace(/[^\w\-]+/g, '')
                .replace(/\-\-+/g, '-')
                .replace(/^-+/, '')
                .replace(/-+$/, '');
        }

        function updateDomainPreview(val) {
            const clean = slugify(val) || 'air-printer';
            document.getElementById('domain_preview').textContent = `http://${clean}.local`;
        }

        // Tự động kiểm tra trạng thái khi tải trang
        window.addEventListener('DOMContentLoaded', async () => {
            setupDropZone();
            loadStatus();
            loadFileList();
        });

        async function loadStatus() {
            try {
                const res = await fetch('/api/status');
                const data = await res.json();
                if (data.device_name) {
                    document.getElementById('dev_name').value = data.device_name;
                    document.getElementById('lblDevName').textContent = data.device_name;
                    updateDomainPreview(data.device_name);
                }
                if (data.status === 'connected') {
                    document.getElementById('lblWifiBadge').innerHTML = 
                        `<span class="status-dot status-online"></span>Đã kết nối (${data.ip})`;
                    // Đã kết nối Wi-Fi -> mở thẳng trang In Ấn & Upload!
                    switchTab('print');
                } else {
                    document.getElementById('lblWifiBadge').innerHTML = 
                        `<span class="status-dot status-ap"></span>Chế độ AP (192.168.4.1)`;
                    // Chưa kết nối Wi-Fi -> mở tab cài đặt Wi-Fi
                    switchTab('wifi');
                }
            } catch (e) {}
        }

        // Kéo thả file
        function setupDropZone() {
            const dz = document.getElementById('dropZone');
            ['dragenter', 'dragover'].forEach(name => {
                dz.addEventListener(name, (e) => { e.preventDefault(); dz.classList.add('dragover'); }, false);
            });
            ['dragleave', 'drop'].forEach(name => {
                dz.addEventListener(name, (e) => { e.preventDefault(); dz.classList.remove('dragover'); }, false);
            });
            dz.addEventListener('drop', (e) => {
                const dt = e.dataTransfer;
                uploadFiles(dt.files);
            }, false);
        }

        // Tải file lên ESP32
        function uploadFiles(files) {
            if (!files || files.length === 0) return;
            const file = files[0]; // Tải từng file

            const pContainer = document.getElementById('progressContainer');
            const pBar = document.getElementById('progressBar');
            const stBox = document.getElementById('uploadStatus');

            pContainer.style.display = 'block';
            pBar.style.width = '0%';
            stBox.style.display = 'block';
            stBox.className = 'status-box loading';
            stBox.innerHTML = `<div class="spinner"></div> Đang tải lên: <b>${file.name}</b>...`;

            const xhr = new XMLHttpRequest();
            xhr.open('POST', '/api/upload?filename=' + encodeURIComponent(file.name), true);

            xhr.upload.onprogress = (e) => {
                if (e.lengthComputable) {
                    const percent = Math.round((e.loaded / e.total) * 100);
                    pBar.style.width = percent + '%';
                }
            };

            xhr.onload = () => {
                pContainer.style.display = 'none';
                if (xhr.status === 200) {
                    stBox.className = 'status-box success';
                    stBox.innerHTML = `✅ Đã tải file <b>${file.name}</b> thành công! Sẵn sàng in.`;
                    loadFileList();
                } else {
                    stBox.className = 'status-box error';
                    stBox.innerHTML = `❌ Tải lên thất bại! Mã lỗi: ${xhr.status}`;
                }
            };

            xhr.onerror = () => {
                pContainer.style.display = 'none';
                stBox.className = 'status-box error';
                stBox.innerHTML = `❌ Lỗi mạng khi tải file.`;
            };

            xhr.send(file);
        }

        // Lấy danh sách file đang có
        async function loadFileList() {
            const container = document.getElementById('fileListContainer');
            try {
                const res = await fetch('/api/files');
                const files = await res.json();
                document.getElementById('fileCount').textContent = files.length;

                if (files.length === 0) {
                    container.innerHTML = `<div style="text-align:center; padding:20px; color:#999; font-size:13px;">Chưa có tài liệu nào trong hàng đợi.</div>`;
                    return;
                }

                container.innerHTML = files.map(f => `
                    <div class="file-item">
                        <div class="file-info">
                            <div class="file-icon">📄</div>
                            <div>
                                <div class="file-name" title="${f.name}">${f.name}</div>
                                <div class="file-size">${f.size} • ${f.time || 'Mới nhận'}</div>
                            </div>
                        </div>
                        <div class="file-actions">
                            <button class="btn-action btn-print" onclick="printFile('${f.name}')">🖨️ In</button>
                            <button class="btn-action btn-del" onclick="deleteFile('${f.name}')">Xóa</button>
                        </div>
                    </div>
                `).join('');
            } catch (e) {
                container.innerHTML = `<div style="text-align:center; color:#c5221f;">Lỗi tải danh sách file</div>`;
            }
        }

        async function printFile(name) {
            const stBox = document.getElementById('uploadStatus');
            stBox.style.display = 'block';
            stBox.className = 'status-box loading';
            stBox.innerHTML = `<div class="spinner"></div> Đang gửi lệnh in file: <b>${name}</b>...`;

            try {
                const res = await fetch('/api/print', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({filename: name})
                });
                const data = await res.json();
                if (data.status === 'printing') {
                    stBox.className = 'status-box success';
                    stBox.innerHTML = `🖨️ <b>Đang in: ${name}</b>. Máy in đang nhận dữ liệu...`;
                }
            } catch (e) {
                stBox.className = 'status-box error';
                stBox.innerHTML = `❌ Lỗi gửi lệnh in!`;
            }
        }

        async function deleteFile(name) {
            if (!confirm(`Bạn có chắc muốn xóa file "${name}"?`)) return;
            try {
                await fetch('/api/files?filename=' + encodeURIComponent(name), { method: 'DELETE' });
                loadFileList();
            } catch (e) {}
        }

        // Quét Wi-Fi
        async function scanWifi() {
            const box = document.getElementById('wifiStatusBox');
            box.style.display = 'block';
            box.className = 'status-box loading';
            box.innerHTML = `<div class="spinner"></div> Đang quét mạng xung quanh...`;
            try {
                const res = await fetch('/api/wifi/scan');
                const list = await res.json();
                const sel = document.getElementById('ssid_select');
                sel.innerHTML = '<option value="">-- Chọn Wi-Fi từ danh sách --</option>';
                list.forEach(item => {
                    const opt = document.createElement('option');
                    opt.value = item.ssid;
                    opt.textContent = `${item.ssid} (${item.rssi} dBm) ${item.locked ? '🔒' : ''}`;
                    sel.appendChild(opt);
                });
                box.className = 'status-box success';
                box.innerHTML = `Đã tìm thấy <b>${list.length}</b> mạng Wi-Fi.`;
            } catch (err) {
                box.className = 'status-box error';
                box.innerHTML = `Quét thất bại. Hãy bấm thử lại!`;
            }
        }

        // Lưu & Kết nối Wi-Fi -> Chuyển sang trang Upload
        async function saveAndConnect() {
            const rawName = document.getElementById('dev_name').value.trim();
            const devName = slugify(rawName) || 'air-printer';
            const ssid = document.getElementById('ssid').value.trim();
            const password = document.getElementById('password').value;

            if (!ssid) {
                alert('Vui lòng chọn hoặc nhập tên Wi-Fi (SSID)!');
                return;
            }

            const box = document.getElementById('wifiStatusBox');
            box.style.display = 'block';
            box.className = 'status-box loading';
            box.innerHTML = `<div class="spinner"></div> Đang kết nối tới Wi-Fi... Xin chờ 10–15 giây.`;

            try {
                const res = await fetch('/api/config', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({
                        device_name: devName,
                        ssid: ssid,
                        password: password
                    })
                });
                const data = await res.json();

                if (data.status === 'connected') {
                    box.className = 'status-box success';
                    box.innerHTML = `<b>✅ KẾT NỐI THÀNH CÔNG!</b><br>
                                    IP mới: <b>http://${data.ip}</b><br>
                                    Tên miền: <b>http://${data.hostname}.local</b><br><br>
                                    <i>Đang chuyển đến trang Tải file trong 2 giây...</i>`;

                    // TỰ ĐỘNG CHUYỂN TRANG UPLOAD SAU KHI VÀO ĐƯỢC WIFI:
                    setTimeout(() => {
                        document.getElementById('lblWifiBadge').innerHTML = 
                            `<span class="status-dot status-online"></span>Đã kết nối (${data.ip})`;
                        document.getElementById('lblDevName').textContent = data.hostname;
                        switchTab('print');
                        loadFileList();
                    }, 2500);

                } else {
                    box.className = 'status-box error';
                    box.innerHTML = `❌ Kết nối thất bại! Vui lòng kiểm tra lại mật khẩu Wi-Fi.`;
                }
            } catch (err) {
                box.className = 'status-box error';
                box.innerHTML = `Lỗi gửi cấu hình. Vui lòng thử lại!`;
            }
        }
    </script>
</body>
</html>
)rawliteral";
