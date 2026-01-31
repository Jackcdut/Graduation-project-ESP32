/**
 * FFT频谱分析功能
 * 提供完整的FFT分析、谐波计算、滤波等功能
 */

// 全局变量
let originalData = [];
let fftResult = null;
let harmonicData = null;
let filteredData = null;
let sampleRate = 1000;
let charts = {};

// 初始化函数，由HTML页面调用
window.initializeFFTPage = function() {
    // 隐藏加载动画
    setTimeout(() => {
        const loader = document.getElementById('pageLoader');
        if (loader) {
            loader.classList.add('loaded');
        }
    }, 1500);

    // 初始化所有功能
    setTimeout(() => {
        try {
            initializeCharts();
            initializeEventListeners();
            initializeParticles();
            showNotification('FFT频谱分析工具已就绪', 'success');
        } catch (error) {
            console.error('FFT页面初始化失败:', error);
            showNotification('页面初始化失败，请刷新重试', 'error');
        }
    }, 800); // 稍微延迟初始化，让加载动画有时间显示
};

// 备用初始化（如果HTML页面没有调用）
document.addEventListener('DOMContentLoaded', function() {
    setTimeout(() => {
        if (typeof Chart !== 'undefined') {
            window.initializeFFTPage();
        } else {
            console.warn('Chart.js未加载，等待HTML页面通知...');
        }
    }, 2000);
});

/**
 * 初始化图表
 */
function initializeCharts() {
    // 注册Chart.js插件
    Chart.register(ChartZoom);

    // 检查ChartAnnotation是否可用
    if (typeof ChartAnnotation !== 'undefined') {
        Chart.register(ChartAnnotation);
    }

    // 时域信号图表
    const timeCtx = document.getElementById('timeChart').getContext('2d');
    charts.timeChart = new Chart(timeCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: '原始信号',
                data: [],
                borderColor: '#3b82f6',
                backgroundColor: 'rgba(59, 130, 246, 0.1)',
                borderWidth: 2,
                pointRadius: 0,
                fill: false,
                tension: 0
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    display: true,
                    position: 'top'
                },
                zoom: {
                    zoom: {
                        wheel: {
                            enabled: true,
                        },
                        pinch: {
                            enabled: true
                        },
                        mode: 'x',
                    },
                    pan: {
                        enabled: true,
                        mode: 'x',
                    }
                }
            },
            scales: {
                x: {
                    title: {
                        display: true,
                        text: '时间 (秒)'
                    },
                    grid: {
                        display: true,
                        color: 'rgba(59, 130, 246, 0.1)'
                    }
                },
                y: {
                    title: {
                        display: true,
                        text: '幅度'
                    },
                    grid: {
                        display: true,
                        color: 'rgba(59, 130, 246, 0.1)'
                    }
                }
            },
            interaction: {
                intersect: false,
                mode: 'index'
            }
        }
    });

    // FFT频谱图表
    const fftCtx = document.getElementById('fftChart').getContext('2d');
    charts.fftChart = new Chart(fftCtx, {
        type: 'bar',
        data: {
            labels: [],
            datasets: [{
                label: 'FFT幅度谱',
                data: [],
                backgroundColor: 'rgba(59, 130, 246, 0.7)',
                borderColor: '#3b82f6',
                borderWidth: 1,
                barPercentage: 1,
                categoryPercentage: 1
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    display: true,
                    position: 'top'
                },
                zoom: {
                    zoom: {
                        wheel: {
                            enabled: true,
                        },
                        pinch: {
                            enabled: true
                        },
                        mode: 'x',
                    },
                    pan: {
                        enabled: true,
                        mode: 'x',
                    }
                }
            },
            scales: {
                x: {
                    title: {
                        display: true,
                        text: '频率 (Hz)'
                    },
                    grid: {
                        display: true,
                        color: 'rgba(59, 130, 246, 0.1)'
                    }
                },
                y: {
                    title: {
                        display: true,
                        text: '幅度'
                    },
                    beginAtZero: true,
                    grid: {
                        display: true,
                        color: 'rgba(59, 130, 246, 0.1)'
                    }
                }
            },
            interaction: {
                intersect: false,
                mode: 'index'
            }
        }
    });

    // 相位谱图表
    const phaseCtx = document.getElementById('phaseChart').getContext('2d');
    charts.phaseChart = new Chart(phaseCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [{
                label: '相位谱',
                data: [],
                borderColor: '#10b981',
                backgroundColor: 'rgba(16, 185, 129, 0.1)',
                borderWidth: 2,
                pointRadius: 0,
                fill: false,
                tension: 0
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    display: true,
                    position: 'top'
                },
                zoom: {
                    zoom: {
                        wheel: {
                            enabled: true,
                        },
                        pinch: {
                            enabled: true
                        },
                        mode: 'x',
                    },
                    pan: {
                        enabled: true,
                        mode: 'x',
                    }
                }
            },
            scales: {
                x: {
                    title: {
                        display: true,
                        text: '频率 (Hz)'
                    },
                    grid: {
                        display: true,
                        color: 'rgba(16, 185, 129, 0.1)'
                    }
                },
                y: {
                    title: {
                        display: true,
                        text: '相位 (弧度)'
                    },
                    grid: {
                        display: true,
                        color: 'rgba(16, 185, 129, 0.1)'
                    }
                }
            },
            interaction: {
                intersect: false,
                mode: 'index'
            }
        }
    });

    // 滤波后信号对比图表
    const filteredCtx = document.getElementById('filteredChart').getContext('2d');
    charts.filteredChart = new Chart(filteredCtx, {
        type: 'line',
        data: {
            labels: [],
            datasets: [
                {
                    label: '原始信号',
                    data: [],
                    borderColor: '#3b82f6',
                    backgroundColor: 'rgba(59, 130, 246, 0.1)',
                    borderWidth: 2,
                    pointRadius: 0,
                    fill: false,
                    tension: 0
                },
                {
                    label: '滤波后信号',
                    data: [],
                    borderColor: '#ef4444',
                    backgroundColor: 'rgba(239, 68, 68, 0.1)',
                    borderWidth: 2,
                    pointRadius: 0,
                    fill: false,
                    tension: 0
                }
            ]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    display: true,
                    position: 'top'
                },
                zoom: {
                    zoom: {
                        wheel: {
                            enabled: true,
                        },
                        pinch: {
                            enabled: true
                        },
                        mode: 'x',
                    },
                    pan: {
                        enabled: true,
                        mode: 'x',
                    }
                }
            },
            scales: {
                x: {
                    title: {
                        display: true,
                        text: '时间 (秒)'
                    },
                    grid: {
                        display: true,
                        color: 'rgba(59, 130, 246, 0.1)'
                    }
                },
                y: {
                    title: {
                        display: true,
                        text: '幅度'
                    },
                    grid: {
                        display: true,
                        color: 'rgba(59, 130, 246, 0.1)'
                    }
                }
            },
            interaction: {
                intersect: false,
                mode: 'index'
            }
        }
    });

    // 谐波分析结果图表
    const harmonicCtx = document.getElementById('harmonicChart').getContext('2d');
    charts.harmonicChart = new Chart(harmonicCtx, {
        type: 'bar',
        data: {
            labels: [],
            datasets: [{
                label: '谐波幅值',
                data: [],
                backgroundColor: [
                    '#3b82f6', '#ef4444', '#10b981', '#f59e0b', '#8b5cf6',
                    '#06b6d4', '#84cc16', '#f97316', '#ec4899', '#6366f1'
                ],
                borderColor: [
                    '#2563eb', '#dc2626', '#059669', '#d97706', '#7c3aed',
                    '#0891b2', '#65a30d', '#ea580c', '#db2777', '#4f46e5'
                ],
                borderWidth: 2
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    display: true,
                    position: 'top'
                }
            },
            scales: {
                x: {
                    title: {
                        display: true,
                        text: '谐波次数'
                    },
                    grid: {
                        display: true,
                        color: 'rgba(59, 130, 246, 0.1)'
                    }
                },
                y: {
                    title: {
                        display: true,
                        text: '幅值'
                    },
                    beginAtZero: true,
                    grid: {
                        display: true,
                        color: 'rgba(59, 130, 246, 0.1)'
                    }
                }
            }
        }
    });
}

/**
 * 初始化事件监听器
 */
function initializeEventListeners() {
    // 文件上传
    const uploadArea = document.getElementById('uploadArea');
    const fileInput = document.getElementById('fileInput');

    if (uploadArea && fileInput) {
        uploadArea.addEventListener('click', () => {
            console.log('Upload area clicked');
            fileInput.click();
        });
        uploadArea.addEventListener('dragover', handleDragOver);
        uploadArea.addEventListener('drop', handleFileDrop);
        uploadArea.addEventListener('dragleave', handleDragLeave);
        fileInput.addEventListener('change', handleFileSelect);
        console.log('File upload event listeners initialized');
    } else {
        console.error('Upload area or file input not found');
    }

    // 数据加载
    const loadDataBtn = document.getElementById('loadDataBtn');
    if (loadDataBtn) {
        loadDataBtn.addEventListener('click', loadData);
        console.log('Load data button event listener initialized');
    }

    // FFT参数变化
    const overlapPercent = document.getElementById('overlapPercent');
    const filterType = document.getElementById('filterType');
    const cutoffFreq1 = document.getElementById('cutoffFreq1');
    const cutoffFreq2 = document.getElementById('cutoffFreq2');

    if (overlapPercent) {
        overlapPercent.addEventListener('input', updateOverlapDisplay);
    }
    if (filterType) {
        filterType.addEventListener('change', updateFilterControls);
    }

    // 添加滤波参数实时更新
    if (cutoffFreq1) {
        cutoffFreq1.addEventListener('input', () => {
            if (filteredData && filteredData.length > 0) {
                applyFilter(); // 实时更新滤波结果
            }
        });
    }
    if (cutoffFreq2) {
        cutoffFreq2.addEventListener('input', () => {
            if (filteredData && filteredData.length > 0) {
                applyFilter(); // 实时更新滤波结果
            }
        });
    }

    // 按钮事件
    const performFFTBtn = document.getElementById('performFFTBtn');
    const analyzeHarmonicsBtn = document.getElementById('analyzeHarmonicsBtn');
    const applyFilterBtn = document.getElementById('applyFilterBtn');

    if (performFFTBtn) {
        performFFTBtn.addEventListener('click', performFFT);
        console.log('Perform FFT button event listener initialized');
    }
    if (analyzeHarmonicsBtn) {
        analyzeHarmonicsBtn.addEventListener('click', analyzeHarmonics);
    }
    if (applyFilterBtn) {
        applyFilterBtn.addEventListener('click', applyFilter);
    }

    // 图表控制按钮
    document.getElementById('zoomResetTime').addEventListener('click', () => resetZoom('timeChart'));
    document.getElementById('zoomResetFreq').addEventListener('click', () => resetZoom('fftChart'));
    document.getElementById('zoomResetPhase').addEventListener('click', () => resetZoom('phaseChart'));
    document.getElementById('zoomResetFiltered').addEventListener('click', () => resetZoom('filteredChart'));

    document.getElementById('toggleTimeGrid').addEventListener('click', () => toggleGrid('timeChart'));
    document.getElementById('toggleFreqGrid').addEventListener('click', () => toggleGrid('fftChart'));
    document.getElementById('togglePhaseGrid').addEventListener('click', () => toggleGrid('phaseChart'));
    document.getElementById('toggleFilteredGrid').addEventListener('click', () => toggleGrid('filteredChart'));

    document.getElementById('markHarmonics').addEventListener('click', markHarmonics);

    // 导出功能
    document.getElementById('exportDataBtn').addEventListener('click', exportData);
    document.getElementById('exportImageBtn').addEventListener('click', exportImages);
    document.getElementById('exportReportBtn').addEventListener('click', generateReport);

    // 添加演示数据生成按钮（如果存在）
    const generateDemoBtn = document.getElementById('generateDemoBtn');
    if (generateDemoBtn) {
        generateDemoBtn.addEventListener('click', generateDemoSignal);
        console.log('Generate demo button event listener initialized');
    }

    // 滑动导航功能
    initializeSlideNavigation();

    // 初始化侧边栏导航功能
    initializeSidebarNavigation();

    // 初始化底部滑动条功能
    initializeHorizontalScrollbar();
}

/**
 * 文件拖拽处理
 */
function handleDragOver(e) {
    e.preventDefault();
    e.currentTarget.classList.add('dragover');
}

function handleDragLeave(e) {
    e.preventDefault();
    e.currentTarget.classList.remove('dragover');
}

function handleFileDrop(e) {
    e.preventDefault();
    e.currentTarget.classList.remove('dragover');
    const files = e.dataTransfer.files;
    if (files.length > 0) {
        processFile(files[0]);
    }
}

function handleFileSelect(e) {
    const files = e.target.files;
    if (files.length > 0) {
        processFile(files[0]);
    }
}

/**
 * 处理文件
 */
function processFile(file) {
    const fileName = file.name;
    const fileExtension = fileName.split('.').pop().toLowerCase();

    if (!['xlsx', 'xls', 'csv', 'txt'].includes(fileExtension)) {
        showNotification('不支持的文件格式', 'error');
        return;
    }

    const reader = new FileReader();

    reader.onload = function(e) {
        try {
            let data = [];

            if (fileExtension === 'csv' || fileExtension === 'txt') {
                // 处理CSV/TXT文件
                const text = e.target.result;
                data = parseTextData(text);
            } else if (fileExtension === 'xlsx' || fileExtension === 'xls') {
                // 处理Excel文件
                const workbook = XLSX.read(e.target.result, { type: 'binary' });
                const firstSheetName = workbook.SheetNames[0];
                const worksheet = workbook.Sheets[firstSheetName];
                const jsonData = XLSX.utils.sheet_to_json(worksheet, { header: 1 });
                data = parseExcelData(jsonData);
            }

            if (data.length > 0) {
                displayDataPreview(data);
                showNotification(`成功读取 ${data.length} 个数据点`, 'success');
            } else {
                showNotification('文件中没有找到有效数据', 'error');
            }
        } catch (error) {
            console.error('文件处理错误:', error);
            showNotification('文件处理失败', 'error');
        }
    };

    if (fileExtension === 'xlsx' || fileExtension === 'xls') {
        reader.readAsBinaryString(file);
    } else {
        reader.readAsText(file);
    }
}

/**
 * 解析文本数据
 */
function parseTextData(text) {
    const lines = text.split('\n');
    const data = [];

    for (let line of lines) {
        line = line.trim();
        if (line === '') continue;

        // 尝试不同的分隔符
        let values = [];
        if (line.includes(',')) {
            values = line.split(',');
        } else if (line.includes('\t')) {
            values = line.split('\t');
        } else if (line.includes(' ')) {
            values = line.split(/\s+/);
        } else {
            values = [line];
        }

        for (let value of values) {
            const num = parseFloat(value.trim());
            if (!isNaN(num)) {
                data.push(num);
            }
        }
    }

    return data;
}

/**
 * 解析Excel数据
 */
function parseExcelData(jsonData) {
    const data = [];

    for (let row of jsonData) {
        for (let cell of row) {
            if (typeof cell === 'number') {
                data.push(cell);
            } else if (typeof cell === 'string') {
                const num = parseFloat(cell);
                if (!isNaN(num)) {
                    data.push(num);
                }
            }
        }
    }

    return data;
}

/**
 * 显示数据预览
 */
function displayDataPreview(data) {
    const preview = document.getElementById('dataPreview');
    const tableBody = document.querySelector('#dataTable tbody');

    // 清空现有内容
    tableBody.innerHTML = '';

    // 显示前50个数据点
    const displayCount = Math.min(data.length, 50);
    for (let i = 0; i < displayCount; i++) {
        const row = document.createElement('tr');
        row.innerHTML = `
            <td>${i + 1}</td>
            <td>${data[i].toFixed(6)}</td>
        `;
        tableBody.appendChild(row);
    }

    if (data.length > 50) {
        const row = document.createElement('tr');
        row.innerHTML = `
            <td colspan="2" style="text-align: center; color: #64748b; font-style: italic;">
                ... 还有 ${data.length - 50} 个数据点
            </td>
        `;
        tableBody.appendChild(row);
    }

    preview.style.display = 'block';

    // 存储数据供后续使用
    window.previewData = data;
}

/**
 * 加载数据
 */
function loadData() {
    let data = [];

    // 检查是否有文件数据
    if (window.previewData && window.previewData.length > 0) {
        data = window.previewData;
    } else {
        // 检查粘贴的数据
        const pasteText = document.getElementById('pasteData').value.trim();
        if (pasteText) {
            data = parseTextData(pasteText);
        }
    }

    if (data.length === 0) {
        showNotification('请先导入数据或粘贴数据', 'error');
        return;
    }

    if (data.length < 32) {
        showNotification('数据点数量太少，至少需要32个数据点', 'error');
        return;
    }

    // 存储原始数据
    originalData = data;
    sampleRate = parseInt(document.getElementById('sampleRate').value);

    // 生成时间轴
    const timeAxis = originalData.map((_, i) => i / sampleRate);

    // 更新时域图表
    charts.timeChart.data.labels = timeAxis;
    charts.timeChart.data.datasets[0].data = originalData.map((value, index) => ({
        x: timeAxis[index],
        y: value
    }));
    charts.timeChart.update();

    showNotification(`成功加载 ${originalData.length} 个数据点`, 'success');
}

/**
 * 更新重叠百分比显示
 */
function updateOverlapDisplay() {
    const value = document.getElementById('overlapPercent').value;
    document.getElementById('overlapValue').textContent = value;
}

/**
 * 更新滤波器控件
 */
function updateFilterControls() {
    const filterType = document.getElementById('filterType').value;
    const cutoffFreq2Group = document.getElementById('cutoffFreq2Group');

    if (filterType === 'bandpass' || filterType === 'bandstop') {
        cutoffFreq2Group.style.display = 'block';
    } else {
        cutoffFreq2Group.style.display = 'none';
    }
}

/**
 * 执行FFT变换
 */
function performFFT() {
    if (originalData.length === 0) {
        showNotification('请先加载数据', 'error');
        return;
    }

    const fftPoints = parseInt(document.getElementById('fftPoints').value);
    const windowFunction = document.getElementById('windowFunction').value;
    sampleRate = parseInt(document.getElementById('sampleRate').value);

    try {
        // 准备数据
        let processedData = [...originalData];

        // 如果数据长度超过FFT点数，截取前面的部分
        if (processedData.length > fftPoints) {
            processedData = processedData.slice(0, fftPoints);
        }

        // 如果数据长度不足FFT点数，用零填充
        while (processedData.length < fftPoints) {
            processedData.push(0);
        }

        // 应用窗函数
        processedData = applyWindow(processedData, windowFunction);

        // 执行FFT
        fftResult = calculateFFT(processedData);

        // 计算频率轴
        const freqAxis = fftResult.magnitude.map((_, i) => i * sampleRate / fftPoints);

        // 只显示正频率部分（奈奎斯特频率以下）
        const halfLength = Math.floor(fftPoints / 2);
        const displayFreq = freqAxis.slice(0, halfLength);
        const displayMagnitude = fftResult.magnitude.slice(0, halfLength);
        const displayPhase = fftResult.phase.slice(0, halfLength);

        // 更新FFT幅度谱图表
        charts.fftChart.data.labels = displayFreq.map(f => f.toFixed(2));
        charts.fftChart.data.datasets[0].data = displayMagnitude.map((mag, index) => ({
            x: displayFreq[index],
            y: mag
        }));
        charts.fftChart.update();

        // 更新相位谱图表
        charts.phaseChart.data.labels = displayFreq.map(f => f.toFixed(2));
        charts.phaseChart.data.datasets[0].data = displayPhase.map((phase, index) => ({
            x: displayFreq[index],
            y: phase
        }));
        charts.phaseChart.update();

        console.log('FFT结果:', {
            fftPoints: fftPoints,
            sampleRate: sampleRate,
            freqResolution: sampleRate / fftPoints,
            maxFreq: displayFreq[displayFreq.length - 1],
            maxMagnitude: Math.max(...displayMagnitude)
        });

        showNotification('FFT变换完成', 'success');

    } catch (error) {
        console.error('FFT计算错误:', error);
        showNotification('FFT计算失败', 'error');
    }
}

/**
 * 应用窗函数
 */
function applyWindow(data, windowType) {
    const N = data.length;
    const windowed = [...data];

    for (let i = 0; i < N; i++) {
        let window = 1;

        switch (windowType) {
            case 'hamming':
                window = 0.54 - 0.46 * Math.cos(2 * Math.PI * i / (N - 1));
                break;
            case 'hanning':
                window = 0.5 * (1 - Math.cos(2 * Math.PI * i / (N - 1)));
                break;
            case 'blackman':
                window = 0.42 - 0.5 * Math.cos(2 * Math.PI * i / (N - 1)) + 0.08 * Math.cos(4 * Math.PI * i / (N - 1));
                break;
            case 'kaiser':
                // 简化的Kaiser窗，beta=5
                const beta = 5;
                const alpha = (N - 1) / 2;
                const x = (i - alpha) / alpha;
                window = besselI0(beta * Math.sqrt(1 - x * x)) / besselI0(beta);
                break;
            case 'bartlett':
                window = 1 - Math.abs((i - (N - 1) / 2) / ((N - 1) / 2));
                break;
            case 'rectangular':
            default:
                window = 1;
                break;
        }

        windowed[i] *= window;
    }

    return windowed;
}

/**
 * 贝塞尔函数I0（用于Kaiser窗）
 */
function besselI0(x) {
    let sum = 1;
    let term = 1;
    let k = 1;

    while (Math.abs(term) > 1e-12) {
        term *= (x / (2 * k)) * (x / (2 * k));
        sum += term;
        k++;
    }

    return sum;
}

/**
 * 计算FFT
 */
function calculateFFT(data) {
    const N = data.length;

    // 确保数据长度是2的幂
    if (!isPowerOfTwo(N)) {
        throw new Error('FFT数据长度必须是2的幂');
    }

    // 创建复数数组
    const real = [...data];
    const imag = new Array(N).fill(0);

    // 执行FFT
    fft(real, imag);

    // 计算幅度和相位
    const magnitude = [];
    const phase = [];

    for (let i = 0; i < N; i++) {
        // 计算幅度，并进行适当的归一化
        magnitude[i] = Math.sqrt(real[i] * real[i] + imag[i] * imag[i]) / N;
        // 对于显示，通常我们需要将幅度乘以2（除了DC分量）
        if (i > 0 && i < N/2) {
            magnitude[i] *= 2;
        }
        phase[i] = Math.atan2(imag[i], real[i]);
    }

    return {
        real: real,
        imag: imag,
        magnitude: magnitude,
        phase: phase
    };
}

/**
 * 检查是否为2的幂
 */
function isPowerOfTwo(n) {
    return n > 0 && (n & (n - 1)) === 0;
}

/**
 * FFT算法实现（Cooley-Tukey算法）
 */
function fft(real, imag) {
    const N = real.length;

    // 位反转重排
    let j = 0;
    for (let i = 1; i < N; i++) {
        let bit = N >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;

        if (i < j) {
            [real[i], real[j]] = [real[j], real[i]];
            [imag[i], imag[j]] = [imag[j], imag[i]];
        }
    }

    // 蝶形运算
    for (let len = 2; len <= N; len *= 2) {
        const halfLen = len / 2;
        const angle = -2 * Math.PI / len;
        const wlen_real = Math.cos(angle);
        const wlen_imag = Math.sin(angle);

        for (let i = 0; i < N; i += len) {
            let w_real = 1;
            let w_imag = 0;

            for (let j = 0; j < halfLen; j++) {
                const u_real = real[i + j];
                const u_imag = imag[i + j];

                const v_real = real[i + j + halfLen] * w_real - imag[i + j + halfLen] * w_imag;
                const v_imag = real[i + j + halfLen] * w_imag + imag[i + j + halfLen] * w_real;

                real[i + j] = u_real + v_real;
                imag[i + j] = u_imag + v_imag;
                real[i + j + halfLen] = u_real - v_real;
                imag[i + j + halfLen] = u_imag - v_imag;

                // 更新旋转因子
                const temp_real = w_real * wlen_real - w_imag * wlen_imag;
                const temp_imag = w_real * wlen_imag + w_imag * wlen_real;
                w_real = temp_real;
                w_imag = temp_imag;
            }
        }
    }
}

/**
 * 谐波分析
 */
function analyzeHarmonics() {
    if (!fftResult) {
        showNotification('请先执行FFT变换', 'error');
        return;
    }

    const fundamentalFreq = parseFloat(document.getElementById('fundamentalFreq').value);
    const harmonicOrder = parseInt(document.getElementById('harmonicOrder').value);
    const fftPoints = parseInt(document.getElementById('fftPoints').value);

    try {
        // 计算谐波频率对应的FFT bin
        const freqResolution = sampleRate / fftPoints;
        const harmonics = [];

        for (let h = 1; h <= harmonicOrder; h++) {
            const harmonicFreq = h * fundamentalFreq;
            const bin = Math.round(harmonicFreq / freqResolution);

            if (bin < fftResult.magnitude.length / 2) {
                harmonics.push({
                    order: h,
                    frequency: harmonicFreq,
                    magnitude: fftResult.magnitude[bin],
                    phase: fftResult.phase[bin]
                });
            }
        }

        if (harmonics.length === 0) {
            showNotification('未找到有效谐波', 'error');
            return;
        }

        // 计算THD和其他失真指标
        const fundamental = harmonics[0].magnitude;
        let harmonicSum = 0;

        for (let i = 1; i < harmonics.length; i++) {
            harmonicSum += harmonics[i].magnitude * harmonics[i].magnitude;
        }

        const thd = Math.sqrt(harmonicSum) / fundamental * 100;

        // 计算噪声功率（简化估算）
        const totalPower = fftResult.magnitude.reduce((sum, mag) => sum + mag * mag, 0);
        const signalPower = harmonics.reduce((sum, h) => sum + h.magnitude * h.magnitude, 0);
        const noisePower = totalPower - signalPower;
        const thdPlusN = Math.sqrt(harmonicSum + noisePower) / fundamental * 100;

        // SINAD计算
        const sinad = 20 * Math.log10(fundamental / Math.sqrt(harmonicSum + noisePower));

        // 存储谐波数据
        harmonicData = {
            harmonics: harmonics,
            thd: thd,
            thdPlusN: thdPlusN,
            sinad: sinad
        };

        // 显示结果
        displayHarmonicResults();
        updateHarmonicChart();

        showNotification('谐波分析完成', 'success');

    } catch (error) {
        console.error('谐波分析错误:', error);
        showNotification('谐波分析失败', 'error');
    }
}

/**
 * 显示谐波分析结果
 */
function displayHarmonicResults() {
    const resultsDiv = document.getElementById('harmonicResults');
    const harmonicList = document.getElementById('harmonicList');

    // 清空现有内容
    harmonicList.innerHTML = '';

    // 显示谐波列表
    harmonicData.harmonics.forEach(harmonic => {
        const item = document.createElement('div');
        item.style.cssText = `
            display: flex;
            justify-content: space-between;
            padding: 6px 10px;
            margin-bottom: 4px;
            background: rgba(255, 255, 255, 0.8);
            border-radius: 6px;
            font-size: 12px;
        `;

        item.innerHTML = `
            <span style="font-weight: 600; color: #1e293b;">
                ${harmonic.order}次谐波 (${harmonic.frequency.toFixed(1)}Hz)
            </span>
            <span style="color: #374151;">
                ${harmonic.magnitude.toFixed(4)}
            </span>
        `;

        harmonicList.appendChild(item);
    });

    // 更新失真指标
    document.getElementById('thdValue').textContent = harmonicData.thd.toFixed(3);
    document.getElementById('thdnValue').textContent = harmonicData.thdPlusN.toFixed(3);
    document.getElementById('sinadValue').textContent = harmonicData.sinad.toFixed(2);

    resultsDiv.style.display = 'block';
}

/**
 * 更新谐波图表
 */
function updateHarmonicChart() {
    const labels = harmonicData.harmonics.map(h => `${h.order}次`);
    const data = harmonicData.harmonics.map(h => h.magnitude);

    charts.harmonicChart.data.labels = labels;
    charts.harmonicChart.data.datasets[0].data = data;
    charts.harmonicChart.update();

    document.getElementById('harmonicContainer').style.display = 'block';
}

/**
 * 应用滤波器
 */
function applyFilter() {
    if (!fftResult) {
        showNotification('请先执行FFT变换', 'error');
        return;
    }

    const filterType = document.getElementById('filterType').value;
    const cutoffFreq1 = parseFloat(document.getElementById('cutoffFreq1').value);
    const cutoffFreq2 = parseFloat(document.getElementById('cutoffFreq2').value);
    const fftPoints = parseInt(document.getElementById('fftPoints').value);

    try {
        // 复制FFT结果
        const filteredReal = [...fftResult.real];
        const filteredImag = [...fftResult.imag];

        // 计算频率分辨率
        const freqResolution = sampleRate / fftPoints;

        // 应用频域滤波
        for (let i = 0; i < fftPoints; i++) {
            const freq = i * freqResolution;
            let shouldFilter = false;

            switch (filterType) {
                case 'lowpass':
                    shouldFilter = freq > cutoffFreq1;
                    break;
                case 'highpass':
                    shouldFilter = freq < cutoffFreq1;
                    break;
                case 'bandpass':
                    shouldFilter = freq < cutoffFreq1 || freq > cutoffFreq2;
                    break;
                case 'bandstop':
                    shouldFilter = freq >= cutoffFreq1 && freq <= cutoffFreq2;
                    break;
            }

            if (shouldFilter) {
                filteredReal[i] = 0;
                filteredImag[i] = 0;
            }
        }

        // 执行逆FFT
        ifft(filteredReal, filteredImag);

        // 提取实部作为滤波后的信号
        filteredData = filteredReal.slice(0, originalData.length);

        // 生成时间轴
        const timeAxis = originalData.map((_, i) => i / sampleRate);

        // 更新滤波对比图表
        charts.filteredChart.data.labels = timeAxis.map(t => t.toFixed(3));
        charts.filteredChart.data.datasets[0].data = originalData.map((value, index) => ({
            x: timeAxis[index],
            y: value
        }));
        charts.filteredChart.data.datasets[1].data = filteredData.map((value, index) => ({
            x: timeAxis[index],
            y: value
        }));
        charts.filteredChart.update('none'); // 使用'none'模式进行快速更新

        document.getElementById('filteredContainer').style.display = 'block';

        // 计算滤波效果统计
        const originalRMS = Math.sqrt(originalData.reduce((sum, val) => sum + val * val, 0) / originalData.length);
        const filteredRMS = Math.sqrt(filteredData.reduce((sum, val) => sum + val * val, 0) / filteredData.length);
        const attenuationDB = 20 * Math.log10(filteredRMS / originalRMS);

        console.log('滤波结果:', {
            filterType: filterType,
            cutoffFreq1: cutoffFreq1,
            cutoffFreq2: cutoffFreq2,
            originalRMS: originalRMS.toFixed(4),
            filteredRMS: filteredRMS.toFixed(4),
            attenuationDB: attenuationDB.toFixed(2) + ' dB'
        });

        showNotification(`${getFilterTypeName(filterType)}应用成功 (衰减: ${attenuationDB.toFixed(1)}dB)`, 'success');

    } catch (error) {
        console.error('滤波错误:', error);
        showNotification('滤波处理失败', 'error');
    }
}

/**
 * 逆FFT算法
 */
function ifft(real, imag) {
    const N = real.length;

    // 共轭
    for (let i = 0; i < N; i++) {
        imag[i] = -imag[i];
    }

    // 执行FFT
    fft(real, imag);

    // 归一化并共轭
    for (let i = 0; i < N; i++) {
        real[i] /= N;
        imag[i] = -imag[i] / N;
    }
}

/**
 * 获取滤波器类型名称
 */
function getFilterTypeName(type) {
    const names = {
        'lowpass': '低通滤波器',
        'highpass': '高通滤波器',
        'bandpass': '带通滤波器',
        'bandstop': '带阻滤波器'
    };
    return names[type] || '滤波器';
}

/**
 * 重置图表缩放
 */
function resetZoom(chartName) {
    if (charts[chartName]) {
        charts[chartName].resetZoom();
    }
}

/**
 * 切换网格显示
 */
function toggleGrid(chartName) {
    if (charts[chartName]) {
        const chart = charts[chartName];
        const xGrid = chart.options.scales.x.grid;
        const yGrid = chart.options.scales.y.grid;

        xGrid.display = !xGrid.display;
        yGrid.display = !yGrid.display;

        chart.update();
    }
}

/**
 * 标记谐波频率
 */
function markHarmonics() {
    if (!harmonicData || !charts.fftChart) {
        showNotification('请先进行谐波分析', 'error');
        return;
    }

    // 检查是否有ChartAnnotation插件
    if (typeof ChartAnnotation === 'undefined') {
        // 如果没有注解插件，在控制台输出谐波信息
        console.log('谐波频率信息:');
        harmonicData.harmonics.forEach(harmonic => {
            console.log(`${harmonic.order}次谐波: ${harmonic.frequency.toFixed(1)}Hz, 幅值: ${harmonic.magnitude.toFixed(4)}`);
        });
        showNotification('谐波信息已输出到控制台', 'info');
        return;
    }

    // 清除现有标记
    if (!charts.fftChart.options.plugins) {
        charts.fftChart.options.plugins = {};
    }
    if (!charts.fftChart.options.plugins.annotation) {
        charts.fftChart.options.plugins.annotation = { annotations: {} };
    } else {
        charts.fftChart.options.plugins.annotation.annotations = {};
    }

    // 添加谐波标记
    harmonicData.harmonics.forEach((harmonic, index) => {
        charts.fftChart.options.plugins.annotation.annotations[`harmonic${index}`] = {
            type: 'line',
            xMin: harmonic.frequency,
            xMax: harmonic.frequency,
            borderColor: `hsl(${index * 40}, 70%, 50%)`,
            borderWidth: 2,
            label: {
                content: `${harmonic.order}次`,
                enabled: true,
                position: 'top'
            }
        };
    });

    charts.fftChart.update();
    showNotification('谐波频率已标记', 'success');
}

/**
 * 导出数据
 */
function exportData() {
    if (originalData.length === 0) {
        showNotification('没有可导出的数据', 'error');
        return;
    }

    let csvContent = 'Time,Original';
    if (filteredData && filteredData.length > 0) {
        csvContent += ',Filtered';
    }
    if (fftResult) {
        csvContent += ',FFT_Magnitude,FFT_Phase';
    }
    csvContent += '\n';

    const timeAxis = originalData.map((_, i) => i / sampleRate);

    for (let i = 0; i < originalData.length; i++) {
        let row = `${timeAxis[i]},${originalData[i]}`;

        if (filteredData && filteredData[i] !== undefined) {
            row += `,${filteredData[i]}`;
        }

        if (fftResult && i < fftResult.magnitude.length) {
            row += `,${fftResult.magnitude[i]},${fftResult.phase[i]}`;
        }

        csvContent += row + '\n';
    }

    downloadFile(csvContent, 'fft_analysis_data.csv', 'text/csv');
    showNotification('数据已导出', 'success');
}

/**
 * 导出图片
 */
function exportImages() {
    const chartNames = ['timeChart', 'fftChart', 'phaseChart'];

    chartNames.forEach(chartName => {
        if (charts[chartName] && charts[chartName].data.datasets[0].data.length > 0) {
            const canvas = charts[chartName].canvas;
            const url = canvas.toDataURL('image/png');
            downloadFile(url, `${chartName}.png`, 'image/png', true);
        }
    });

    showNotification('图片已导出', 'success');
}

/**
 * 生成分析报告
 */
function generateReport() {
    if (originalData.length === 0) {
        showNotification('没有可生成报告的数据', 'error');
        return;
    }

    let report = '# FFT频谱分析报告\n\n';
    report += `生成时间: ${new Date().toLocaleString()}\n\n`;

    // 基本信息
    report += '## 基本信息\n';
    report += `- 数据点数: ${originalData.length}\n`;
    report += `- 采样频率: ${sampleRate} Hz\n`;
    report += `- 信号时长: ${(originalData.length / sampleRate).toFixed(3)} 秒\n\n`;

    // 统计信息
    const mean = originalData.reduce((sum, val) => sum + val, 0) / originalData.length;
    const variance = originalData.reduce((sum, val) => sum + Math.pow(val - mean, 2), 0) / originalData.length;
    const std = Math.sqrt(variance);
    const max = Math.max(...originalData);
    const min = Math.min(...originalData);

    report += '## 统计信息\n';
    report += `- 均值: ${mean.toFixed(6)}\n`;
    report += `- 标准差: ${std.toFixed(6)}\n`;
    report += `- 最大值: ${max.toFixed(6)}\n`;
    report += `- 最小值: ${min.toFixed(6)}\n\n`;

    // 谐波分析结果
    if (harmonicData) {
        report += '## 谐波分析结果\n';
        report += `- THD: ${harmonicData.thd.toFixed(3)}%\n`;
        report += `- THD+N: ${harmonicData.thdPlusN.toFixed(3)}%\n`;
        report += `- SINAD: ${harmonicData.sinad.toFixed(2)} dB\n\n`;

        report += '### 谐波详情\n';
        harmonicData.harmonics.forEach(harmonic => {
            report += `- ${harmonic.order}次谐波 (${harmonic.frequency.toFixed(1)}Hz): ${harmonic.magnitude.toFixed(6)}\n`;
        });
        report += '\n';
    }

    downloadFile(report, 'fft_analysis_report.md', 'text/markdown');
    showNotification('分析报告已生成', 'success');
}

/**
 * 下载文件
 */
function downloadFile(content, filename, mimeType, isDataURL = false) {
    const blob = isDataURL ? null : new Blob([content], { type: mimeType });
    const url = isDataURL ? content : URL.createObjectURL(blob);

    const link = document.createElement('a');
    link.href = url;
    link.download = filename;
    link.style.display = 'none';

    document.body.appendChild(link);
    link.click();
    document.body.removeChild(link);

    if (!isDataURL) {
        URL.revokeObjectURL(url);
    }
}

/**
 * 初始化粒子效果
 */
function initializeParticles() {
    const particlesContainer = document.getElementById('particles');
    const particleCount = 50;

    for (let i = 0; i < particleCount; i++) {
        const particle = document.createElement('div');
        particle.className = 'particle';
        particle.style.left = Math.random() * 100 + '%';
        particle.style.animationDelay = Math.random() * 15 + 's';
        particle.style.animationDuration = (15 + Math.random() * 10) + 's';
        particlesContainer.appendChild(particle);
    }
}

/**
 * 显示通知
 */
function showNotification(message, type = 'info') {
    const container = document.getElementById('notificationContainer');
    const notification = document.createElement('div');

    const colors = {
        success: { bg: '#10b981', border: '#059669' },
        error: { bg: '#ef4444', border: '#dc2626' },
        warning: { bg: '#f59e0b', border: '#d97706' },
        info: { bg: '#3b82f6', border: '#2563eb' }
    };

    const color = colors[type] || colors.info;

    notification.style.cssText = `
        background: ${color.bg};
        color: white;
        padding: 12px 20px;
        border-radius: 8px;
        margin-bottom: 10px;
        box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);
        border-left: 4px solid ${color.border};
        font-size: 14px;
        font-weight: 500;
        opacity: 0;
        transform: translateX(100%);
        transition: all 0.3s ease;
        max-width: 300px;
        word-wrap: break-word;
    `;

    notification.textContent = message;
    container.appendChild(notification);

    // 触发动画
    setTimeout(() => {
        notification.style.opacity = '1';
        notification.style.transform = 'translateX(0)';
    }, 10);

    // 自动移除
    setTimeout(() => {
        notification.style.opacity = '0';
        notification.style.transform = 'translateX(100%)';
        setTimeout(() => {
            if (notification.parentNode) {
                notification.parentNode.removeChild(notification);
            }
        }, 300);
    }, 3000);
}

/**
 * 生成演示信号
 */
function generateDemoSignal() {
    const sampleRate = 1000; // 1kHz采样率
    const duration = 1; // 1秒信号
    const numSamples = sampleRate * duration;

    // 生成包含50Hz基波和150Hz三次谐波的信号
    const data = [];
    for (let i = 0; i < numSamples; i++) {
        const t = i / sampleRate;
        const fundamental = Math.sin(2 * Math.PI * 50 * t); // 50Hz基波
        const harmonic3 = 0.3 * Math.sin(2 * Math.PI * 150 * t); // 150Hz三次谐波
        const harmonic5 = 0.1 * Math.sin(2 * Math.PI * 250 * t); // 250Hz五次谐波
        const noise = 0.05 * (Math.random() - 0.5); // 少量噪声

        data.push(fundamental + harmonic3 + harmonic5 + noise);
    }

    // 显示数据预览
    displayDataPreview(data);

    // 设置采样频率
    document.getElementById('sampleRate').value = sampleRate;

    showNotification('演示信号已生成（50Hz基波 + 谐波）', 'success');
}

/**
 * 初始化滑动导航功能
 */
function initializeSlideNavigation() {
    const slideUp = document.getElementById('slideUp');
    const slideDown = document.getElementById('slideDown');

    // 获取图表区域
    const chartsArea = document.querySelector('.charts-area');

    // 滑动距离
    const slideDistance = 300;

    if (slideUp && chartsArea) {
        slideUp.addEventListener('click', () => {
            chartsArea.scrollBy({ top: -slideDistance, behavior: 'smooth' });
            console.log('图表区域向上滑动');
        });
    }

    if (slideDown && chartsArea) {
        slideDown.addEventListener('click', () => {
            chartsArea.scrollBy({ top: slideDistance, behavior: 'smooth' });
            console.log('图表区域向下滑动');
        });
    }

    // 页面指示器功能
    const pageDots = document.querySelectorAll('.page-dot');
    pageDots.forEach(dot => {
        dot.addEventListener('click', () => {
            const section = dot.dataset.section;
            scrollToSection(section);

            // 更新活动状态
            pageDots.forEach(d => d.classList.remove('active'));
            dot.classList.add('active');
        });
    });

    // 键盘导航（只保留上下键，作用于图表区域）
    document.addEventListener('keydown', (e) => {
        if ((e.ctrlKey || e.metaKey) && chartsArea) {
            switch(e.key) {
                case 'ArrowUp':
                    e.preventDefault();
                    chartsArea.scrollBy({ top: -slideDistance, behavior: 'smooth' });
                    console.log('键盘：图表区域向上滑动');
                    break;
                case 'ArrowDown':
                    e.preventDefault();
                    chartsArea.scrollBy({ top: slideDistance, behavior: 'smooth' });
                    console.log('键盘：图表区域向下滑动');
                    break;
            }
        }
    });

    // 触摸滑动支持（只支持垂直滑动图表区域）
    let touchStartY = 0;

    if (chartsArea) {
        chartsArea.addEventListener('touchstart', (e) => {
            touchStartY = e.touches[0].clientY;
        });

        chartsArea.addEventListener('touchend', (e) => {
            const touchEndY = e.changedTouches[0].clientY;
            const deltaY = touchEndY - touchStartY;

            const minSwipeDistance = 50;

            // 只处理垂直滑动
            if (Math.abs(deltaY) > minSwipeDistance) {
                if (deltaY > 0) {
                    // 向下滑动（手指向下，内容向上）
                    chartsArea.scrollBy({ top: -slideDistance, behavior: 'smooth' });
                    console.log('触摸：图表区域向上滑动');
                } else {
                    // 向上滑动（手指向上，内容向下）
                    chartsArea.scrollBy({ top: slideDistance, behavior: 'smooth' });
                    console.log('触摸：图表区域向下滑动');
                }
            }
        });
    }
}

/**
 * 滚动到指定区域
 */
function scrollToSection(section) {
    const chartsArea = document.querySelector('.charts-area');
    if (!chartsArea) return;

    switch(section) {
        case 'time':
            const timeChart = document.getElementById('timeChart');
            if (timeChart) {
                timeChart.scrollIntoView({ behavior: 'smooth', block: 'center' });
            }
            break;
        case 'freq':
            const fftChart = document.getElementById('fftChart');
            if (fftChart) {
                fftChart.scrollIntoView({ behavior: 'smooth', block: 'center' });
            }
            break;
        case 'phase':
            const phaseChart = document.getElementById('phaseChart');
            if (phaseChart) {
                phaseChart.scrollIntoView({ behavior: 'smooth', block: 'center' });
            }
            break;
        case 'filtered':
            const filteredChart = document.getElementById('filteredChart');
            if (filteredChart) {
                filteredChart.scrollIntoView({ behavior: 'smooth', block: 'center' });
            }
            break;
    }
}

/**
 * 初始化侧边栏导航功能
 */
function initializeSidebarNavigation() {
    console.log('初始化侧边栏导航功能...');

    // 分类展开/收起
    const categoryHeaders = document.querySelectorAll('.category-header');
    console.log(`找到 ${categoryHeaders.length} 个分类标题`);

    categoryHeaders.forEach((header, index) => {
        console.log(`绑定第 ${index + 1} 个分类标题的点击事件:`, header.querySelector('.category-title').textContent);

        header.addEventListener('click', (e) => {
            e.preventDefault();
            console.log('分类标题被点击:', header.querySelector('.category-title').textContent);

            const category = header.closest('.nav-category');
            const isExpanded = category.classList.contains('expanded');

            console.log('当前展开状态:', isExpanded);

            // 切换当前分类
            category.classList.toggle('expanded', !isExpanded);
            header.classList.toggle('expanded', !isExpanded);

            console.log('新的展开状态:', !isExpanded);

            // 可选：收起其他分类（如果需要手风琴效果）
            // document.querySelectorAll('.nav-category').forEach(cat => {
            //     if (cat !== category) {
            //         cat.classList.remove('expanded');
            //         const catHeader = cat.querySelector('.category-header');
            //         if (catHeader) {
            //             catHeader.classList.remove('expanded');
            //         }
            //     }
            // });
        });
    });

    // 处理导航链接点击
    const navLinks = document.querySelectorAll('.nav-link');
    navLinks.forEach(link => {
        link.addEventListener('click', (e) => {
            // 如果链接有data-page属性，说明是页面跳转链接，不阻止默认行为
            if (link.hasAttribute('data-page')) {
                console.log('页面跳转链接被点击:', link.getAttribute('data-page'));
                return; // 让页面跳转逻辑处理
            }

            // 如果是功能链接，阻止默认行为并处理功能
            if (link.hasAttribute('data-function')) {
                e.preventDefault();
                const functionType = link.dataset.function;
                console.log('功能链接被点击:', functionType);
                handleNavFunction(functionType);

                // 更新活动状态
                document.querySelectorAll('.nav-link').forEach(l => l.classList.remove('active'));
                link.classList.add('active');
            }
        });
    });

    console.log('侧边栏导航功能初始化完成');
}

/**
 * 处理导航功能
 */
function handleNavFunction(functionType) {
    switch(functionType) {

        case 'denoise':
            showNotification('信号去噪功能', 'info');
            break;

        default:
            showNotification('功能开发中...', 'info');
            break;
    }
}

/**
 * 初始化底部水平滑动条功能
 */
function initializeHorizontalScrollbar() {
    const horizontalScrollbar = document.querySelector('.horizontal-scrollbar');
    const mainContent = document.querySelector('.main-content');
    const contentLayout = document.querySelector('.content-layout');

    if (!horizontalScrollbar || !mainContent || !contentLayout) {
        console.log('滑动条元素未找到');
        return;
    }

    console.log('初始化底部滑动条功能...');

    // 同步主内容区域的滚动到底部滑动条
    mainContent.addEventListener('scroll', () => {
        horizontalScrollbar.scrollLeft = mainContent.scrollLeft;
    });

    // 同步底部滑动条的滚动到主内容区域
    horizontalScrollbar.addEventListener('scroll', () => {
        mainContent.scrollLeft = horizontalScrollbar.scrollLeft;
    });

    // 监听内容布局的宽度变化，更新滑动条内容宽度
    const updateScrollbarWidth = () => {
        const contentWidth = contentLayout.scrollWidth;
        const scrollbarContent = horizontalScrollbar.querySelector('.horizontal-scrollbar-content');
        if (scrollbarContent) {
            scrollbarContent.style.width = contentWidth + 'px';
        }
    };

    // 初始更新
    updateScrollbarWidth();

    // 监听窗口大小变化
    window.addEventListener('resize', updateScrollbarWidth);

    // 监听内容变化（当图表更新时）
    const observer = new MutationObserver(updateScrollbarWidth);
    observer.observe(contentLayout, {
        childList: true,
        subtree: true,
        attributes: true,
        attributeFilter: ['style']
    });

    console.log('底部滑动条功能初始化完成');
}
