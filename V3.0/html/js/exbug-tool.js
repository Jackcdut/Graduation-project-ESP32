/**
 * ExDebug Tool - 简洁版控制台脚本
 */

let realtimeChart = null;
let startTime = Date.now();

// 页面初始化
document.addEventListener('DOMContentLoaded', function() {
    initChart();
    initChartTabs();
    loadDeviceInfo();
    updateTime();
    setInterval(updateTime, 1000);
    setInterval(updateSystemStats, 3000);
    
    addLog('系统启动', 'info');
    setTimeout(() => addLog('硬件自检完成', 'success'), 500);
    setTimeout(() => addLog('WiFi连接成功', 'success'), 1000);
});

// 时间更新
function updateTime() {
    const now = new Date();
    const timeEl = document.getElementById('currentTime');
    const dateEl = document.getElementById('currentDate');
    
    if (timeEl) timeEl.textContent = now.toLocaleTimeString('zh-CN', { hour12: false });
    if (dateEl) {
        dateEl.textContent = now.toLocaleDateString('zh-CN', { 
            year: 'numeric', month: 'long', day: 'numeric' 
        });
    }
}

// 系统状态更新
function updateSystemStats() {
    const temp = 38 + Math.random() * 8;
    const memory = 40 + Math.random() * 20;
    
    const tempEl = document.getElementById('chipTemp');
    const memEl = document.getElementById('memoryUsage');
    
    if (tempEl) tempEl.textContent = Math.round(temp) + '°C';
    if (memEl) memEl.textContent = Math.round(memory) + '%';
}

// 设备信息加载
function loadDeviceInfo() {
    const deviceId = localStorage.getItem('onenet_device_id');
    const authInfo = localStorage.getItem('onenet_auth_info');
    
    const deviceIdEl = document.getElementById('deviceIdDisplay');
    const deviceNameEl = document.getElementById('deviceName');
    const cloudStatusEl = document.getElementById('cloudStatus');
    
    if (deviceIdEl) deviceIdEl.textContent = deviceId || '--';
    if (deviceNameEl) deviceNameEl.textContent = authInfo || '--';
    
    if (deviceId && cloudStatusEl) {
        cloudStatusEl.textContent = '已连接';
        cloudStatusEl.classList.add('good');
    }
}

// 图表初始化
function initChart() {
    const ctx = document.getElementById('realtimeChart');
    if (!ctx) return;
    
    const labels = Array.from({length: 60}, (_, i) => i + 's');
    const voltageData = Array.from({length: 60}, () => Math.random() * 2 + 4);
    const currentData = Array.from({length: 60}, () => Math.random() * 50 + 80);
    const powerData = voltageData.map((v, i) => (v * currentData[i] / 1000).toFixed(2));
    
    realtimeChart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: labels,
            datasets: [{
                label: '电压',
                data: voltageData,
                borderColor: '#2563eb',
                backgroundColor: 'rgba(37, 99, 235, 0.1)',
                fill: true,
                tension: 0.4,
                borderWidth: 2,
                pointRadius: 0
            }, {
                label: '电流',
                data: currentData,
                borderColor: '#10b981',
                backgroundColor: 'rgba(16, 185, 129, 0.1)',
                fill: true,
                tension: 0.4,
                borderWidth: 2,
                pointRadius: 0,
                yAxisID: 'y1'
            }, {
                label: '功率',
                data: powerData,
                borderColor: '#f59e0b',
                backgroundColor: 'transparent',
                fill: false,
                tension: 0.4,
                borderWidth: 2,
                pointRadius: 0,
                yAxisID: 'y2'
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: { legend: { display: false } },
            scales: {
                x: {
                    grid: { display: false },
                    ticks: { maxTicksLimit: 10, color: '#94a3b8', font: { size: 10 } }
                },
                y: {
                    beginAtZero: false,
                    position: 'left',
                    grid: { color: '#f1f5f9' },
                    ticks: { color: '#2563eb', font: { size: 10 } }
                },
                y1: {
                    beginAtZero: false,
                    position: 'right',
                    grid: { display: false },
                    ticks: { color: '#10b981', font: { size: 10 } }
                },
                y2: { display: false }
            },
            interaction: { intersect: false, mode: 'index' }
        }
    });
    
    setInterval(updateChartData, 2000);
}

function updateChartData() {
    if (!realtimeChart) return;
    
    const v = Math.random() * 2 + 4;
    const c = Math.random() * 50 + 80;
    const p = (v * c / 1000).toFixed(2);
    
    realtimeChart.data.datasets[0].data.shift();
    realtimeChart.data.datasets[0].data.push(v);
    realtimeChart.data.datasets[1].data.shift();
    realtimeChart.data.datasets[1].data.push(c);
    realtimeChart.data.datasets[2].data.shift();
    realtimeChart.data.datasets[2].data.push(p);
    
    realtimeChart.update('none');
}

function initChartTabs() {
    const tabs = document.querySelectorAll('.tab-btn');
    tabs.forEach(tab => {
        tab.addEventListener('click', function() {
            tabs.forEach(t => t.classList.remove('active'));
            this.classList.add('active');
            showNotification('已切换到 ' + this.textContent + ' 视图', 'info');
        });
    });
}

// 模块操作
function openModule(module) {
    const names = {
        'oscilloscope': '数字示波器',
        'generator': 'DDS信号发生器',
        'multimeter': '数字万用表',
        'power': '数控直流电源',
        'serial': '无线串口透传'
    };
    showNotification('正在打开 ' + names[module], 'info');
    addLog('打开 ' + names[module], 'info');
}

// 刷新数据
function refreshAllData() {
    showNotification('正在刷新数据...', 'info');
    loadDeviceInfo();
    updateSystemStats();
    setTimeout(() => showNotification('数据刷新完成', 'success'), 800);
}

// 云端同步
function syncToCloud() {
    const deviceId = localStorage.getItem('onenet_device_id');
    if (!deviceId) {
        showNotification('请先登录设备', 'warning');
        return;
    }
    
    showNotification('正在同步到云端...', 'info');
    addLog('开始云端同步', 'info');
    
    setTimeout(() => {
        const lastSyncEl = document.getElementById('lastSync');
        if (lastSyncEl) lastSyncEl.textContent = new Date().toLocaleTimeString('zh-CN');
        showNotification('同步成功', 'success');
        addLog('云端同步完成', 'success');
    }, 1500);
}

// 活动日志
function addLog(text, type = 'info') {
    const list = document.getElementById('activityList');
    if (!list) return;
    
    const time = new Date().toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' });
    const item = document.createElement('div');
    item.className = 'activity-item';
    item.innerHTML = `
        <div class="activity-dot ${type}"></div>
        <span class="activity-text">${text}</span>
        <span class="activity-time">${time}</span>
    `;
    
    list.insertBefore(item, list.firstChild);
    while (list.children.length > 8) list.removeChild(list.lastChild);
}

// 通知
function showNotification(message, type = 'info') {
    document.querySelectorAll('.notification').forEach(n => n.remove());
    
    const notification = document.createElement('div');
    notification.className = `notification ${type}`;
    notification.innerHTML = `<i class="fas fa-${type === 'success' ? 'check-circle' : type === 'error' ? 'times-circle' : 'info-circle'}"></i><span>${message}</span>`;
    document.body.appendChild(notification);
    
    setTimeout(() => {
        notification.style.animation = 'slideOut 0.3s ease';
        setTimeout(() => notification.remove(), 300);
    }, 3000);
}
