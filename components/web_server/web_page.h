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

        /* Multi-file Upload Task Queue */
        .upload-tasks-container { display: none; margin-top: 16px; border: 1.5px solid #d2e3fc; border-radius: 12px; padding: 14px; background: #f8fbff; }
        .upload-tasks-summary { display: flex; justify-content: space-between; align-items: center; font-size: 13px; font-weight: 700; color: #1a73e8; margin-bottom: 10px; }
        .upload-task-item { background: #ffffff; border: 1px solid #dadce0; border-radius: 8px; padding: 10px 12px; margin-bottom: 8px; box-shadow: 0 1px 3px rgba(0,0,0,0.03); }
        .upload-task-item:last-child { margin-bottom: 0; }
        .upload-task-header { display: flex; justify-content: space-between; align-items: center; font-size: 13px; font-weight: 600; margin-bottom: 6px; }
        .upload-task-name { white-space: nowrap; overflow: hidden; text-overflow: ellipsis; max-width: 270px; color: #202124; }
        .upload-task-meta { font-size: 11px; color: #5f6368; flex-shrink: 0; }
        .upload-task-bar-wrap { background: #e8eaed; border-radius: 6px; height: 8px; overflow: hidden; width: 100%; }
        .upload-task-bar { height: 100%; width: 0%; background: #1a73e8; border-radius: 6px; transition: width 0.15s ease; }
        .upload-task-bar.bar-success { background: #1e8e3e; }
        .upload-task-bar.bar-error { background: #d93025; }
        .upload-task-footer { display: flex; justify-content: space-between; font-size: 11px; font-weight: 500; margin-top: 5px; }
        .task-status-wait { color: #80868b; }
        .task-status-running { color: #1a73e8; font-weight: 600; }
        .task-status-success { color: #1e8e3e; font-weight: 600; }
        .task-status-error { color: #d93025; font-weight: 600; }

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
                <div class="drop-hint">Hỗ trợ chọn cùng lúc nhiều file hoặc nguyên cả thư mục</div>
                <div style="display:flex; gap:10px; justify-content:center; margin-top:12px;">
                    <button type="button" class="btn-action" style="background:#e8f0fe; color:#1a73e8; border:1px solid #aecbfa;" onclick="event.stopPropagation(); document.getElementById('fileInput').click()">📄 Chọn nhiều file</button>
                    <button type="button" class="btn-action" style="background:#e8f0fe; color:#1a73e8; border:1px solid #aecbfa;" onclick="event.stopPropagation(); document.getElementById('folderInput').click()">📁 Chọn cả thư mục</button>
                </div>
                <input type="file" id="fileInput" multiple style="display:none;" onchange="uploadFiles(this.files); this.value='';">
                <input type="file" id="folderInput" webkitdirectory directory multiple style="display:none;" onchange="uploadFiles(this.files); this.value='';">
            </div>

            <!-- Multi-file Upload Task Queue Container -->
            <div id="uploadTasksContainer" class="upload-tasks-container">
                <div class="upload-tasks-summary">
                    <span id="uploadSummaryTitle">Tiến trình tải lên</span>
                    <span id="uploadSummaryCount" style="color:#1a73e8; font-size:12px;">0%</span>
                </div>
                <div id="uploadTasksList"></div>
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

        function formatBytes(bytes) {
            if (!bytes || bytes <= 0) return '0 B';
            const k = 1024;
            const sizes = ['B', 'KB', 'MB', 'GB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return parseFloat((bytes / Math.pow(k, i)).toFixed(1)) + ' ' + sizes[i];
        }

        function uploadSingleFile(file, displayName, onProgress) {
            return new Promise((resolve) => {
                const xhr = new XMLHttpRequest();
                const encodedName = encodeURIComponent(displayName);
                xhr.open('POST', '/api/upload?filename=' + encodedName, true);
                
                // Gửi kèm tiêu đề X-Filename để ESP32 đọc chính xác tên dài
                xhr.setRequestHeader('X-Filename', encodedName);
                xhr.timeout = 180000; // 3 phút cho các file lớn > 25MB

                xhr.upload.onprogress = (e) => {
                    if (e.lengthComputable && onProgress) {
                        onProgress(e.loaded, e.total);
                    }
                };

                xhr.onload = () => {
                    resolve(xhr.status === 200);
                };

                xhr.onerror = () => {
                    resolve(false);
                };

                xhr.ontimeout = () => {
                    resolve(false);
                };

                xhr.send(file);
            });
        }

        // Tải danh sách nhiều file / thư mục lên ESP32 tuần tự
        async function uploadFiles(files) {
            if (!files || files.length === 0) return;

            // Chuyển ngay FileList sang mảng tĩnh JS độc lập
            // Vì nếu input.value = '' được gọi, FileList gốc của trình duyệt sẽ bị reset về rỗng
            const fileList = Array.from(files);
            const total = fileList.length;

            const tasksContainer = document.getElementById('uploadTasksContainer');
            const tasksList = document.getElementById('uploadTasksList');
            const summaryTitle = document.getElementById('uploadSummaryTitle');
            const summaryCount = document.getElementById('uploadSummaryCount');
            const stBox = document.getElementById('uploadStatus');

            tasksContainer.style.display = 'block';
            stBox.style.display = 'none';

            summaryTitle.textContent = `Tiến trình tải lên (${total} tệp)`;
            summaryCount.textContent = `0/${total} hoàn tất`;

            // 1. Dựng sẵn khung tiến trình cho TỪNG tệp ngay từ đầu
            tasksList.innerHTML = fileList.map((file, idx) => {
                const displayName = file.webkitRelativePath || file.name;
                const sizeStr = formatBytes(file.size);
                const safeName = displayName.replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
                return `
                    <div class="upload-task-item" id="task-item-${idx}">
                        <div class="upload-task-header">
                            <span class="upload-task-name" title="${safeName}">📄 ${safeName}</span>
                            <span class="upload-task-meta">${sizeStr}</span>
                        </div>
                        <div class="upload-task-bar-wrap">
                            <div class="upload-task-bar" id="pbar-${idx}"></div>
                        </div>
                        <div class="upload-task-footer">
                            <span id="pstatus-${idx}" class="task-status-wait">⏳ Đang chờ...</span>
                            <span id="ppercent-${idx}" style="color:#5f6368;">0%</span>
                        </div>
                    </div>
                `;
            }).join('');

            let completedCount = 0;

            // 2. Chạy tải lần lượt từng file từ mảng fileList tĩnh
            for (let i = 0; i < total; i++) {
                const file = fileList[i];
                const displayName = file.webkitRelativePath || file.name;
                const pbar = document.getElementById(`pbar-${i}`);
                const pstatus = document.getElementById(`pstatus-${i}`);
                const ppercent = document.getElementById(`ppercent-${i}`);

                if (pstatus) {
                    pstatus.className = 'task-status-running';
                    pstatus.innerHTML = `<span class="spinner" style="width:11px;height:11px;border-width:2px;margin-right:4px;"></span> Đang tải...`;
                }

                try {
                    const success = await uploadSingleFile(file, displayName, (loaded, totalBytes) => {
                        const percent = Math.round((loaded / totalBytes) * 100);
                        if (pbar) pbar.style.width = percent + '%';
                        if (ppercent) ppercent.textContent = `${percent}% (${formatBytes(loaded)} / ${formatBytes(totalBytes)})`;
                    });

                    if (success) {
                        completedCount++;
                        if (pbar) {
                            pbar.style.width = '100%';
                            pbar.classList.add('bar-success');
                        }
                        if (pstatus) {
                            pstatus.className = 'task-status-success';
                            pstatus.textContent = '✅ Đã tải xong';
                        }
                        if (ppercent) ppercent.textContent = '100%';
                        // Cập nhật ngay hàng đợi bên dưới khi mỗi file xong
                        loadFileList();
                    } else {
                        if (pbar) pbar.classList.add('bar-error');
                        if (pstatus) {
                            pstatus.className = 'task-status-error';
                            pstatus.textContent = '❌ Tải thất bại';
                        }
                    }
                } catch (err) {
                    if (pbar) pbar.classList.add('bar-error');
                    if (pstatus) {
                        pstatus.className = 'task-status-error';
                        pstatus.textContent = '❌ Lỗi kết nối';
                    }
                }

                summaryCount.textContent = `${completedCount}/${total} hoàn tất`;

                // Nghỉ 500ms giữa 2 tệp để ESP32 giải phóng socket và ổn định bộ đệm mạng
                if (i < total - 1) {
                    await new Promise(r => setTimeout(r, 500));
                }
            }

            // 3. Thông báo tổng kết
            stBox.style.display = 'block';
            if (completedCount === total) {
                stBox.className = 'status-box success';
                stBox.innerHTML = `✅ Đã tải xong toàn bộ <b>${completedCount}</b> tài liệu!`;
            } else {
                stBox.className = 'status-box error';
                stBox.innerHTML = `⚠️ Đã tải ${completedCount}/${total} tài liệu. Có tệp gặp sự cố kết nối!`;
            }

            loadFileList();
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

                container.innerHTML = files.map(f => {
                    const encodedName = encodeURIComponent(f.name);
                    const safeDisplayName = f.name.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
                    return `
                    <div class="file-item">
                        <div class="file-info">
                            <div class="file-icon">📄</div>
                            <div>
                                <div class="file-name" title="${safeDisplayName}">${safeDisplayName}</div>
                                <div class="file-size">${f.size} • ${f.time || 'Mới nhận'}</div>
                            </div>
                        </div>
                        <div class="file-actions">
                            <button class="btn-action btn-print" onclick="printFile(decodeURIComponent('${encodedName}'))">🖨️ In</button>
                            <button class="btn-action btn-del" onclick="deleteFile(decodeURIComponent('${encodedName}'))">Xóa</button>
                        </div>
                    </div>
                `;
                }).join('');
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
