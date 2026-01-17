// 增强版认证系统
class AuthSystem {
    constructor() {
        this.isLoading = false;
        this.failedAttempts = 0;
        this.maxFailedAttempts = 5;
        this.lockoutDuration = 30 * 60 * 1000; // 30分钟
        
        this.init();
    }

    async init() {
        // 等待uniCloud初始化
        await this.waitForUniCloud();
        
        // 绑定事件监听器
        this.bindEventListeners();
        
        // 初始化页面状态
        this.initializePageState();
        
        // 检查用户登录状态
        await this.checkAuthStatus();
        
        console.log('✓ 认证系统初始化完成');
    }

    async waitForUniCloud() {
        let attempts = 0;
        const maxAttempts = 50;
        
        while (!window.uniCloudService?.isInitialized && attempts < maxAttempts) {
            await new Promise(resolve => setTimeout(resolve, 100));
            attempts++;
        }
        
        if (!window.uniCloudService?.isInitialized) {
            throw new Error('UniCloud 服务初始化超时');
        }
    }

    bindEventListeners() {
        // 登录表单
        const loginForm = document.getElementById('loginForm');
        if (loginForm) {
            loginForm.addEventListener('submit', (e) => this.handleLogin(e));
        }

        // 注册表单
        const registerForm = document.getElementById('registerForm');
        if (registerForm) {
            registerForm.addEventListener('submit', (e) => this.handleRegister(e));
        }

        // 密码重置表单
        const resetForm = document.getElementById('resetForm');
        if (resetForm) {
            resetForm.addEventListener('submit', (e) => this.handlePasswordReset(e));
        }

        // 实时验证
        this.bindValidationListeners();
        
        // 发送验证码按钮
        this.bindVerificationButton();
        
        // 服务条款复选框
        this.bindAgreementCheckbox();

    }

    bindValidationListeners() {
        // 用户名验证
        const usernameInput = document.getElementById('username');
        if (usernameInput) {
            usernameInput.addEventListener('blur', () => this.validateUsername(usernameInput.value));
            usernameInput.addEventListener('input', () => this.clearFieldError(usernameInput));
        }

        // 邮箱验证
        const emailInput = document.getElementById('email');
        if (emailInput) {
            emailInput.addEventListener('blur', () => this.validateEmail(emailInput.value));
            emailInput.addEventListener('input', () => this.clearFieldError(emailInput));
        }

        // 密码验证
        const passwordInput = document.getElementById('password');
        if (passwordInput) {
            passwordInput.addEventListener('input', () => this.validatePassword(passwordInput.value));
        }

        // 确认密码验证
        const confirmPasswordInput = document.getElementById('confirm_password');
        if (confirmPasswordInput && passwordInput) {
            confirmPasswordInput.addEventListener('blur', () => {
                this.validatePasswordMatch(passwordInput.value, confirmPasswordInput.value);
            });
        }
    }
    
    bindVerificationButton() {
        const sendVerificationBtn = document.getElementById('sendVerificationBtn');
        if (sendVerificationBtn) {
            sendVerificationBtn.addEventListener('click', async () => {
                const emailInput = document.getElementById('email');
                const email = emailInput?.value?.trim();
                
                if (!email) {
                    this.showNotification('请先输入邮箱地址', 'error');
                    return;
                }
                
                if (!this.validateEmail(email)) {
                    this.showNotification('请输入有效的邮箱地址', 'error');
                    return;
                }
                
                const success = await this.sendEmailVerification(email, 'register');
                if (success) {
                    this.startVerificationButtonCooldown(sendVerificationBtn);
                    this.showNotification('验证码发送成功，请查收邮箱', 'success');
                }
            });
        }
    }

    bindAgreementCheckbox() {
        // 检查当前页面是否需要协议复选框功能
        const isRegisterPage = document.location.pathname.includes('register') || 
                               document.querySelector('#registerForm') !== null ||
                               document.getElementById('agreement') !== null;
        
        if (!isRegisterPage) {
            console.log('[Agreement] 🚫 当前页面无需协议复选框功能，跳过初始化');
            return;
        }
        
        // 使用多种方式确保DOM完全加载，但限制重试次数
        let retryCount = 0;
        const maxRetries = 10; // 最多重试10次
        
        const initAgreementHandler = () => {
            const agreementCheckbox = document.getElementById('agreement');
            const handwritingContainer = document.querySelector('.handwriting-container');
            
            if (agreementCheckbox && handwritingContainer) {
                console.log('[Agreement] ✓ 找到必要元素，开始初始化');
                
                // 确保初始状态正确
                handwritingContainer.classList.remove('show');
                
                // 清除可能存在的旧事件监听器
                const newCheckbox = agreementCheckbox.cloneNode(true);
                agreementCheckbox.parentNode.replaceChild(newCheckbox, agreementCheckbox);
                
                // 添加新的事件监听器
                newCheckbox.addEventListener('change', function() {
                    console.log('[Agreement] 复选框状态改变:', this.checked);
                    
                    if (this.checked) {
                        console.log('[Agreement] 显示手写动画');
                        handwritingContainer.classList.add('show');
                        
                        // 重置并重新应用动画
                        const paths = handwritingContainer.querySelectorAll('path');
                        console.log('[Agreement] 找到路径数量:', paths.length);
                        
                        paths.forEach((path, index) => {
                            // 完全重置路径样式
                            path.style.strokeDasharray = '1000';
                            path.style.strokeDashoffset = '1000';
                            path.style.animation = 'none';
                            
                            // 强制重排
                            path.offsetHeight;
                            
                            // 延迟重新应用动画
                            setTimeout(() => {
                                path.style.animation = '';
                            }, 10 + index * 5);
                        });
                    } else {
                        console.log('[Agreement] 隐藏手写动画');
                        handwritingContainer.classList.remove('show');
                        
                        // 重置所有路径
                        const paths = handwritingContainer.querySelectorAll('path');
                        paths.forEach(path => {
                            path.style.strokeDasharray = '1000';
                            path.style.strokeDashoffset = '1000';
                            path.style.animation = 'none';
                        });
                    }
                });
                
                console.log('[Agreement] ✓ 事件监听器绑定成功');
            } else {
                retryCount++;
                if (retryCount < maxRetries) {
                    console.log(`[Agreement] ⚠ 元素未找到，重试 ${retryCount}/${maxRetries}...`);
                    setTimeout(initAgreementHandler, 200);
                } else {
                    console.warn('[Agreement] ❌ 达到最大重试次数，停止尝试。可能当前页面不包含协议复选框。');
                }
            }
        };
        
        // 立即尝试初始化
        if (document.readyState === 'loading') {
            document.addEventListener('DOMContentLoaded', initAgreementHandler);
        } else {
            initAgreementHandler();
        }
        
        // 额外的保险措施
        setTimeout(initAgreementHandler, 300);
    }

    startVerificationButtonCooldown(button, seconds = 60) {
        button.disabled = true;
        button.classList.add('loading');
        
        let countdown = seconds;
        const originalText = button.textContent;
        
        const updateText = () => {
            button.textContent = `重新发送 (${countdown}s)`;
            
            if (countdown > 0) {
                countdown--;
                setTimeout(updateText, 1000);
            } else {
                button.textContent = originalText;
                button.disabled = false;
                button.classList.remove('loading');
            }
        };
        
        updateText();
    }

    initializePageState() {
        // 检查账户锁定状态
        this.checkAccountLockout();
        
        // 初始化页面加载动画
        setTimeout(() => {
            const loader = document.querySelector('.page-loader');
            if (loader) {
                loader.classList.add('loaded');
            }
        }, 1000);
    }

    async checkAuthStatus() {
        try {
            const currentUser = await window.uniCloudService.getCurrentUser();
            if (currentUser?.result?.success) {
                // 用户已登录，可以选择跳转到主页面
                console.log('用户已登录:', currentUser.result.user);
                
                // 如果在登录页面，可以提示用户已登录
                if (window.location.pathname.includes('index.html') || window.location.pathname === '/') {
                    this.showNotification('您已登录系统', 'info');
                }
            }
        } catch (error) {
            console.log('用户未登录或token无效');
        }
    }

    // 登录处理
    async handleLogin(event) {
        event.preventDefault();
        
        if (this.isLoading) return;
        
        // 获取表单数据
        const usernameInput = document.getElementById('username');
        const passwordInput = document.getElementById('password');
        const rememberInput = document.getElementById('remember');
        
        const credentials = {
            username: usernameInput?.value?.trim() || '',
            password: passwordInput?.value?.trim() || '',
            remember: rememberInput?.checked || false
        };

        // 前端验证
        if (!this.validateLoginForm(credentials)) {
            return;
        }

        // 检查账户锁定
        if (this.isAccountLocked()) {
            this.showNotification('账户已被临时锁定，请稍后再试', 'error');
            return;
        }

        try {
            this.setLoadingState(true);
            this.showNotification('正在验证登录信息...', 'info');

            const result = await window.uniCloudService.login(credentials);

            if (result.result?.success) {
                // 登录成功
                this.failedAttempts = 0;
                localStorage.removeItem('account_lockout');
                
                // 保存用户信息和token
                const { user, token } = result.result;
                localStorage.setItem('unicloud_token', token);
                localStorage.setItem('currentUser', JSON.stringify(user));
                localStorage.setItem('loginTime', new Date().toISOString());

                if (credentials.remember) {
                    localStorage.setItem('remember_user', credentials.username);
                }

                this.showNotification('登录成功！正在跳转...', 'success');
                
                // 跳转到主页面
                setTimeout(() => {
                    window.location.href = 'data.html';
                }, 1500);

            } else {
                // 登录失败
                this.handleLoginFailure(result.result?.message || '登录失败');
            }

        } catch (error) {
            console.error('登录错误:', error);
            this.handleLoginFailure('网络错误，请检查连接后重试');
        } finally {
            this.setLoadingState(false);
        }
    }

    // 注册处理
    async handleRegister(event) {
        event.preventDefault();
        
        if (this.isLoading) return;

        // 获取表单数据
        const usernameInput = document.getElementById('username');
        const emailInput = document.getElementById('email');
        const passwordInput = document.getElementById('password');
        const confirmPasswordInput = document.getElementById('confirm_password');
        const agreementInput = document.getElementById('agreement');
        
        const userData = {
            username: usernameInput?.value?.trim() || '',
            email: emailInput?.value?.trim() || '',
            password: passwordInput?.value?.trim() || '',
            confirmPassword: confirmPasswordInput?.value?.trim() || '',
            agreement: agreementInput?.checked || false
        };

        // 前端验证
        if (!this.validateRegisterForm(userData)) {
            return;
        }

        try {
            this.setLoadingState(true);
            this.showNotification('正在创建账户...', 'info');

            // 先检查用户名和邮箱是否已存在
            const [usernameCheck, emailCheck] = await Promise.all([
                window.uniCloudService.checkUsernameExists(userData.username),
                window.uniCloudService.checkEmailExists(userData.email)
            ]);

            if (usernameCheck.result?.exists) {
                this.showFieldError('username', '用户名已被使用');
                return;
            }

            if (emailCheck.result?.exists) {
                this.showFieldError('email', '邮箱已被注册');
                return;
            }

            // 发送邮箱验证码
            await this.sendEmailVerification(userData.email, 'register');

            // 显示邮箱验证界面
            this.showEmailVerificationModal(userData);

        } catch (error) {
            console.error('注册错误:', error);
            this.showNotification('注册失败：' + (error.message || '未知错误'), 'error');
        } finally {
            this.setLoadingState(false);
        }
    }

    // 密码重置处理
    async handlePasswordReset(event) {
        event.preventDefault();
        
        if (this.isLoading) return;

        const email = document.getElementById('reset_email')?.value?.trim();
        
        if (!this.validateEmail(email)) {
            this.showFieldError('reset_email', '请输入有效的邮箱地址');
            return;
        }

        try {
            this.setLoadingState(true);
            this.showNotification('正在发送重置邮件...', 'info');

            const result = await window.uniCloudService.resetPassword(email);

            if (result.result?.success) {
                this.showPasswordResetSuccess();
                this.showNotification('密码重置邮件已发送', 'success');
            } else {
                this.showNotification(result.result?.message || '发送失败', 'error');
            }

        } catch (error) {
            console.error('密码重置错误:', error);
            this.showNotification('发送失败：' + (error.message || '未知错误'), 'error');
        } finally {
            this.setLoadingState(false);
        }
    }

    // 发送邮箱验证码
    async sendEmailVerification(email, type = 'register') {
        try {
            const result = await window.uniCloudService.sendVerificationCode(email, type);
            if (result.result?.success) {
                this.showNotification(`验证码已发送到 ${email}`, 'success');
                return true;
            } else {
                throw new Error(result.result?.message || '发送失败');
            }
        } catch (error) {
            this.showNotification('验证码发送失败：' + error.message, 'error');
            return false;
        }
    }

    // 显示邮箱验证模态框
    showEmailVerificationModal(userData) {
        const modal = this.createVerificationModal(userData);
        document.body.appendChild(modal);
        
        // 添加动画效果
        setTimeout(() => modal.classList.add('show'), 10);
    }

    // 创建验证码模态框
    createVerificationModal(userData) {
        const modal = document.createElement('div');
        modal.className = 'verification-modal';
        modal.innerHTML = `
            <div class="verification-content">
                <div class="verification-header">
                    <h3>邮箱验证</h3>
                    <button class="close-btn" onclick="this.closest('.verification-modal').remove()">×</button>
                </div>
                <div class="verification-body">
                    <p>我们已向 <strong>${userData.email}</strong> 发送了验证码</p>
                    <div class="verification-input-group">
                        <input type="text" id="verification-code" placeholder="请输入6位验证码" maxlength="6">
                        <button class="resend-btn" onclick="authSystem.resendVerificationCode('${userData.email}')">
                            重新发送 <span class="countdown"></span>
                        </button>
                    </div>
                    <div class="verification-actions">
                        <button class="btn btn-primary" onclick="authSystem.verifyEmailCode('${userData.email}', ${JSON.stringify(userData).replace(/"/g, '&quot;')})">
                            验证并注册
                        </button>
                    </div>
                </div>
            </div>
        `;

        // 开始倒计时
        this.startResendCountdown(modal.querySelector('.countdown'));
        
        return modal;
    }

    // 验证邮箱验证码并完成注册
    async verifyEmailCode(email, userData) {
        const code = document.getElementById('verification-code')?.value?.trim();
        
        if (!code || code.length !== 6) {
            this.showNotification('请输入6位验证码', 'error');
            return;
        }

        try {
            this.setLoadingState(true);
            
            // 验证邮箱验证码
            const verifyResult = await window.uniCloudService.verifyEmail(email, code);
            
            if (!verifyResult.result?.success) {
                this.showNotification(verifyResult.result?.message || '验证码错误', 'error');
                return;
            }

            // 验证成功，进行注册
            const registerResult = await window.uniCloudService.register({
                username: userData.username,
                email: userData.email,
                password: userData.password
            });

            if (registerResult.result?.success) {
                // 注册成功
                document.querySelector('.verification-modal')?.remove();
                this.showNotification('注册成功！请登录您的账户', 'success');
                
                // 跳转到登录页面或自动登录
                setTimeout(() => {
                    window.location.href = 'index.html';
                }, 2000);

            } else {
                this.showNotification(registerResult.result?.message || '注册失败', 'error');
            }

        } catch (error) {
            console.error('验证注册错误:', error);
            this.showNotification('验证失败：' + (error.message || '未知错误'), 'error');
        } finally {
            this.setLoadingState(false);
        }
    }

    // 重新发送验证码
    async resendVerificationCode(email) {
        const success = await this.sendEmailVerification(email, 'register');
        if (success) {
            const countdownElement = document.querySelector('.countdown');
            this.startResendCountdown(countdownElement);
        }
    }

    // 倒计时功能
    startResendCountdown(element, seconds = 60) {
        if (!element) return;
        
        const resendBtn = element.closest('.resend-btn');
        resendBtn.disabled = true;
        
        const countdown = () => {
            element.textContent = `(${seconds}s)`;
            if (seconds > 0) {
                seconds--;
                setTimeout(countdown, 1000);
            } else {
                element.textContent = '';
                resendBtn.disabled = false;
            }
        };
        
        countdown();
    }

    // 表单验证方法
    validateLoginForm(credentials) {
        let isValid = true;

        if (!credentials.username) {
            this.showFieldError('username', '请输入用户名');
            isValid = false;
        }

        if (!credentials.password) {
            this.showFieldError('password', '请输入密码');
            isValid = false;
        }

        return isValid;
    }

    validateRegisterForm(userData) {
        let isValid = true;

        if (!userData.username || userData.username.length < 3) {
            this.showFieldError('username', '用户名至少需要3个字符');
            isValid = false;
        }

        if (!this.validateEmail(userData.email)) {
            this.showFieldError('email', '请输入有效的邮箱地址');
            isValid = false;
        }



        if (!this.validatePassword(userData.password)) {
            isValid = false;
        }

        if (userData.password !== userData.confirmPassword) {
            this.showFieldError('confirm_password', '两次输入的密码不一致');
            isValid = false;
        }

        if (!userData.agreement) {
            this.showNotification('请同意服务条款', 'error');
            isValid = false;
        }

        return isValid;
    }

    // 各种验证方法
    validateEmail(email) {
        const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
        return emailRegex.test(email);
    }

    validatePassword(password) {
        if (!password || password.length < 6) {
            this.showFieldError('password', '密码至少需要6个字符');
            return false;
        }
        
        // 可以添加更复杂的密码强度检查
        return true;
    }

    validateUsername(username) {
        if (!username || username.length < 3) {
            this.showFieldError('username', '用户名至少需要3个字符');
            return false;
        }
        return true;
    }

    validatePasswordMatch(password, confirmPassword) {
        if (password !== confirmPassword) {
            this.showFieldError('confirm_password', '两次输入的密码不一致');
            return false;
        }
        return true;
    }

    // UI 相关方法
    showFieldError(fieldId, message) {
        const field = document.getElementById(fieldId);
        if (!field) return;

        const inputGroup = field.closest('.input-group');
        if (inputGroup) {
            inputGroup.classList.add('error');
            
            let errorElement = inputGroup.querySelector('.error-message');
            if (!errorElement) {
                errorElement = document.createElement('span');
                errorElement.className = 'error-message';
                inputGroup.appendChild(errorElement);
            }
            
            errorElement.textContent = message;
        }
    }

    clearFieldError(field) {
        const inputGroup = field.closest('.input-group');
        if (inputGroup) {
            inputGroup.classList.remove('error');
            const errorElement = inputGroup.querySelector('.error-message');
            if (errorElement) {
                errorElement.remove();
            }
        }
    }

    showNotification(message, type = 'info') {
        // 创建通知元素
        const notification = document.createElement('div');
        notification.className = `notification notification-${type}`;
        notification.innerHTML = `
            <div class="notification-content">
                <span class="notification-message">${message}</span>
                <button class="notification-close" onclick="this.parentElement.parentElement.remove()">×</button>
            </div>
        `;

        // 添加到页面
        document.body.appendChild(notification);

        // 自动移除
        setTimeout(() => {
            if (notification.parentElement) {
                notification.classList.add('fade-out');
                setTimeout(() => notification.remove(), 300);
            }
        }, 4000);
    }

    setLoadingState(loading) {
        this.isLoading = loading;
        const buttons = document.querySelectorAll('.btn');
        
        buttons.forEach(btn => {
            if (loading) {
                btn.classList.add('loading');
                btn.disabled = true;
            } else {
                btn.classList.remove('loading');
                btn.disabled = false;
            }
        });
    }

    handleLoginFailure(message) {
        this.failedAttempts++;
        
        if (this.failedAttempts >= this.maxFailedAttempts) {
            this.lockAccount();
            this.showNotification('登录失败次数过多，账户已被临时锁定', 'error');
        } else {
            this.showNotification(message, 'error');
        }
    }

    lockAccount() {
        const lockoutTime = Date.now() + this.lockoutDuration;
        localStorage.setItem('account_lockout', lockoutTime.toString());
    }

    isAccountLocked() {
        const lockoutTime = localStorage.getItem('account_lockout');
        if (!lockoutTime) return false;
        
        const now = Date.now();
        if (now < parseInt(lockoutTime)) {
            return true;
        } else {
            localStorage.removeItem('account_lockout');
            return false;
        }
    }

    checkAccountLockout() {
        if (this.isAccountLocked()) {
            const lockoutTime = parseInt(localStorage.getItem('account_lockout'));
            const remainingTime = Math.ceil((lockoutTime - Date.now()) / 60000);
            this.showNotification(`账户被锁定，请 ${remainingTime} 分钟后再试`, 'warning');
        }
    }

    showPasswordResetSuccess() {
        const stepEmail = document.getElementById('step-email');
        const stepConfirm = document.getElementById('step-confirm');
        
        if (stepEmail && stepConfirm) {
            stepEmail.style.display = 'none';
            stepConfirm.style.display = 'block';
            stepConfirm.classList.add('animate__animated', 'animate__fadeIn');
        }
    }

    // 退出登录
    async logout() {
        try {
            await window.uniCloudService.logout();
            this.showNotification('已安全退出', 'success');
            
            setTimeout(() => {
                window.location.href = 'index.html';
            }, 1000);
            
        } catch (error) {
            console.error('退出登录错误:', error);
            // 即使云端退出失败，也要清除本地存储
            localStorage.removeItem('unicloud_token');
            localStorage.removeItem('currentUser');
            window.location.href = 'index.html';
        }
    }
}

// 初始化认证系统
document.addEventListener('DOMContentLoaded', () => {
    window.authSystem = new AuthSystem();
});

// 导出供其他模块使用
if (typeof module !== 'undefined' && module.exports) {
    module.exports = AuthSystem;
} 