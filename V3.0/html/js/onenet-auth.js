/**
 * OneNet设备认证系统
 * 通过OneNet平台验证设备ID和产品ID进行登录
 * 
 * ============ OneNet官方查询设备ID API ============
 * 
 * 接口地址: http://ota.heclouds.com/ota/devInfo
 * 请求方式: GET
 * 
 * HTTP头部参数:
 * | 名称          | 格式   | 必须 | 说明                    |
 * |---------------|--------|------|-------------------------|
 * | Content-Type  | string | 是   | 必须为application/json  |
 * | Authorization | string | 是   | 安全鉴权信息            |
 * 
 * 请求Body参数:
 * | 名称     | 格式   | 必须 | 说明                    |
 * |----------|--------|------|-------------------------|
 * | pid      | long   | 是   | 产品ID                  |
 * | authInfo | string | 是   | 设备唯一标识            |
 * 
 * authInfo说明（根据产品协议不同）:
 * - NB协议: IMEI号
 * - MQTTS(新版MQTT): 设备名称
 * - MQTT/EDP/TCP透传: 鉴权信息
 * - HTTP协议: 设备编号
 * 
 * 返回参数:
 * | 名称   | 格式   | 说明                              |
 * |--------|--------|-----------------------------------|
 * | errno  | int    | 错误码，0表示成功                 |
 * | error  | string | 错误描述，"succ"表示成功          |
 * | data   | json   | 设备信息 { dev_id: "设备ID" }     |
 * 
 * ================================================
 */

class OneNetAuthSystem {
    constructor() {
        this.isLoading = false;
        this.productId = 'FCwDzD6VU0';  // 固定产品ID
        // Cloudflare Worker 代理地址
        this.apiUrl = 'https://onenet-verify.2085761619.workers.dev';
        
        this.init();
    }

    init() {
        this.bindEventListeners();
        this.loadSavedCredentials();
        this.hideLoader();
        console.log('✓ OneNet认证系统初始化完成');
    }

    bindEventListeners() {
        const loginForm = document.getElementById('loginForm');
        if (loginForm) {
            loginForm.addEventListener('submit', (e) => this.handleLogin(e));
        }

        const authInfoInput = document.getElementById('authInfo');
        if (authInfoInput) {
            authInfoInput.addEventListener('input', () => this.clearFieldError('authInfoGroup'));
            authInfoInput.addEventListener('blur', () => this.validateAuthInfo());
        }
    }

    loadSavedCredentials() {
        const savedAuthInfo = localStorage.getItem('onenet_auth_info');
        const rememberChecked = localStorage.getItem('onenet_remember') === 'true';
        
        if (rememberChecked && savedAuthInfo) {
            const authInfoInput = document.getElementById('authInfo');
            const rememberInput = document.getElementById('remember');
            
            if (authInfoInput) authInfoInput.value = savedAuthInfo;
            if (rememberInput) rememberInput.checked = true;
        }
    }

    async handleLogin(event) {
        event.preventDefault();
        
        if (this.isLoading) return;

        const authInfo = document.getElementById('authInfo')?.value?.trim() || '';
        const remember = document.getElementById('remember')?.checked || false;

        // 验证输入
        if (!this.validateAuthInfo()) {
            return;
        }

        try {
            this.setLoadingState(true);
            this.showStatus('正在连接OneNet平台...', 'connecting');

            // 验证设备
            const result = await this.verifyDevice(this.productId, authInfo);

            if (result.success) {
                this.showStatus('设备验证成功！', 'success');
                this.saveDeviceInfo(authInfo, result.deviceId, remember);
                this.showNotification('登录成功，正在跳转...', 'success');
                
                setTimeout(() => {
                    window.location.href = 'data.html';
                }, 1500);
            } else {
                this.showStatus(result.message || '设备验证失败', 'error');
                this.showNotification(result.message || '验证失败，请检查设备标识', 'error');
                this.setLoadingState(false);
            }

        } catch (error) {
            console.error('登录错误:', error);
            this.showStatus('网络错误，请重试', 'error');
            this.showNotification('网络连接失败', 'error');
            this.setLoadingState(false);
        }
    }

    async verifyDevice(productId, authInfo) {
        // 验证输入
        if (!authInfo || authInfo.length < 1) {
            return { success: false, message: '设备ID不能为空' };
        }

        if (productId !== 'FCwDzD6VU0') {
            return { success: false, message: '产品ID无效' };
        }

        if (authInfo.length < 3) {
            return { success: false, message: '设备ID至少需要3个字符' };
        }

        try {
            // 通过阿里云函数计算代理调用 OneNet API
            console.log('通过云函数验证设备...');
            const result = await this.callOneNetAPI(productId, authInfo);
            
            if (result.errno === 0 && result.data && result.data.dev_id) {
                return {
                    success: true,
                    deviceId: result.data.dev_id,
                    message: '设备验证成功'
                };
            } else {
                let errorMsg = '设备验证失败';
                if (result.errno === 1) {
                    errorMsg = '设备不存在，请检查设备ID';
                } else if (result.error && result.error !== 'succ') {
                    errorMsg = result.error;
                }
                return { success: false, message: errorMsg };
            }
        } catch (error) {
            console.error('验证失败:', error);
            return { 
                success: false, 
                message: '网络请求失败，请检查网络连接' 
            };
        }
    }

    async callOneNetAPI(productId, authInfo) {
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), 10000);

        try {
            const response = await fetch(this.apiUrl, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    pid: productId,
                    authInfo: authInfo
                }),
                signal: controller.signal
            });

            clearTimeout(timeoutId);

            if (!response.ok) {
                throw new Error(`HTTP ${response.status}`);
            }

            return await response.json();
        } catch (error) {
            clearTimeout(timeoutId);
            if (error.name === 'AbortError') {
                throw new Error('请求超时');
            }
            throw error;
        }
    }

    saveDeviceInfo(authInfo, deviceId, remember) {
        localStorage.setItem('onenet_device_id', deviceId);
        localStorage.setItem('onenet_product_id', this.productId);
        localStorage.setItem('onenet_auth_info', authInfo);
        localStorage.setItem('onenet_login_time', new Date().toISOString());
        localStorage.setItem('onenet_remember', remember.toString());
        
        sessionStorage.setItem('device_authenticated', 'true');
        sessionStorage.setItem('current_device', JSON.stringify({
            deviceId: deviceId,
            productId: this.productId,
            authInfo: authInfo,
            loginTime: new Date().toISOString()
        }));
    }

    validateAuthInfo() {
        const authInfo = document.getElementById('authInfo')?.value?.trim();
        if (!authInfo || authInfo.length < 1) {
            this.showFieldError('authInfoGroup', '请输入设备唯一标识');
            return false;
        }
        this.clearFieldError('authInfoGroup');
        return true;
    }

    showFieldError(groupId, message) {
        const group = document.getElementById(groupId);
        if (group) {
            group.classList.add('error');
            const errorEl = group.querySelector('.input-error');
            if (errorEl) errorEl.textContent = message;
        }
    }

    clearFieldError(groupId) {
        const group = document.getElementById(groupId);
        if (group) group.classList.remove('error');
    }

    showStatus(message, type) {
        const statusEl = document.getElementById('statusMessage');
        const textEl = document.getElementById('statusText');
        
        if (statusEl && textEl) {
            statusEl.className = 'status-message show ' + type;
            textEl.textContent = message;
            
            const icon = statusEl.querySelector('i');
            if (icon) {
                if (type === 'connecting') {
                    icon.className = 'fas fa-spinner fa-spin';
                } else if (type === 'success') {
                    icon.className = 'fas fa-check-circle';
                } else if (type === 'error') {
                    icon.className = 'fas fa-exclamation-circle';
                }
            }
        }
    }

    showNotification(message, type = 'info') {
        const notification = document.createElement('div');
        notification.className = `notification ${type}`;
        notification.textContent = message;
        document.body.appendChild(notification);

        setTimeout(() => {
            notification.style.animation = 'slideIn 0.3s ease reverse';
            setTimeout(() => notification.remove(), 300);
        }, 3000);
    }

    setLoadingState(loading) {
        this.isLoading = loading;
        const submitBtn = document.getElementById('loginBtn');
        
        if (submitBtn) {
            if (loading) {
                submitBtn.classList.add('loading');
                submitBtn.disabled = true;
            } else {
                submitBtn.classList.remove('loading');
                submitBtn.disabled = false;
            }
        }
    }

    hideLoader() {
        setTimeout(() => {
            // 尝试通过ID查找
            let loader = document.getElementById('pageLoader');
            // 如果没有ID，尝试通过class查找
            if (!loader) {
                loader = document.querySelector('.page-loader');
            }
            if (loader) {
                loader.classList.add('loaded');
            }
        }, 500);
    }

    // 静态方法
    static isAuthenticated() {
        return sessionStorage.getItem('device_authenticated') === 'true';
    }

    static getCurrentDevice() {
        const deviceStr = sessionStorage.getItem('current_device');
        return deviceStr ? JSON.parse(deviceStr) : null;
    }

    static logout() {
        sessionStorage.removeItem('device_authenticated');
        sessionStorage.removeItem('current_device');
        window.location.href = 'index.html';
    }
}

// 初始化
document.addEventListener('DOMContentLoaded', () => {
    window.oneNetAuth = new OneNetAuthSystem();
});
