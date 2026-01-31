// 注册页面主要功能 - 简化版本
let currentCaptcha = '';

// Toast通知系统
function showToast(type, title, message, duration = 3000) {
    console.log(`显示Toast: ${type} - ${title}: ${message}`);
    
    const toastContainer = document.getElementById('toastContainer');
    if (!toastContainer) {
        console.error('Toast容器未找到');
        return;
    }
    
    // 创建toast元素
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    
    toast.innerHTML = `
        <div class="toast-content">
            <div class="toast-title">${title}</div>
            <div class="toast-message">${message}</div>
        </div>
        <div class="toast-progress">
            <div class="toast-progress-bar"></div>
        </div>
    `;
    
    // 添加到容器
    toastContainer.appendChild(toast);
    
    // 显示动画
    setTimeout(() => {
        toast.classList.add('show');
        
        // 进度条动画
        const progressBar = toast.querySelector('.toast-progress-bar');
        if (progressBar) {
            progressBar.style.width = '100%';
            setTimeout(() => {
                progressBar.style.transitionDuration = `${duration}ms`;
                progressBar.style.width = '0%';
            }, 50);
        }
        
        // 自动移除
        setTimeout(() => {
            toast.classList.remove('show');
            setTimeout(() => {
                if (toastContainer.contains(toast)) {
                    toastContainer.removeChild(toast);
                }
            }, 300);
        }, duration);
    }, 100);
}

// 生成验证码
function generateCaptcha() {
    console.log('生成验证码');
    const canvas = document.getElementById('captcha-canvas');
    if (!canvas) {
        console.error('验证码画布未找到');
        return;
    }
    
    const ctx = canvas.getContext('2d');
    const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
    
    // 清空画布
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    
    // 设置背景
    ctx.fillStyle = '#f8fafc';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    
    // 生成4位验证码
    currentCaptcha = '';
    for (let i = 0; i < 4; i++) {
        currentCaptcha += chars.charAt(Math.floor(Math.random() * chars.length));
    }
    
    console.log('当前验证码:', currentCaptcha);
    
    // 绘制验证码
    ctx.font = 'bold 16px Arial';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    
    for (let i = 0; i < currentCaptcha.length; i++) {
        const x = 15 + i * 18;
        const y = 18;
        
        // 随机颜色
        const colors = ['#2c7be5', '#00d97e', '#f6c343', '#e63757'];
        ctx.fillStyle = colors[Math.floor(Math.random() * colors.length)];
        
        ctx.fillText(currentCaptcha[i], x, y);
    }
}

// 验证函数
function validateEmail(email) {
    const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    return emailRegex.test(email);
}

function validatePassword(password) {
    return password.length >= 6;
}

// 表单验证
function validateForm() {
    console.log('开始表单验证');
    
    const username = document.getElementById('username')?.value.trim() || '';
    const email = document.getElementById('email')?.value.trim() || '';
    const password = document.getElementById('password')?.value || '';
    const confirmPassword = document.getElementById('confirm_password')?.value || '';
    const captcha = document.getElementById('captcha')?.value.trim().toUpperCase() || '';
    const agreement = document.getElementById('agreement')?.checked || false;
    
    console.log('表单数据:', { username, email, password: '***', confirmPassword: '***', captcha, agreement });
    
    if (!username) {
        showToast('error', '输入错误', '请输入用户名');
        return false;
    }
    
    if (username.length < 3) {
        showToast('error', '用户名错误', '用户名至少需要3个字符');
        return false;
    }
    
    if (!email) {
        showToast('error', '输入错误', '请输入电子邮箱');
        return false;
    }
    
    if (!validateEmail(email)) {
        showToast('error', '邮箱错误', '请输入有效的电子邮箱地址');
        return false;
    }
    
    if (!password) {
        showToast('error', '输入错误', '请设置密码');
        return false;
    }
    
    if (!validatePassword(password)) {
        showToast('error', '密码错误', '密码至少需要6个字符');
        return false;
    }
    
    if (!confirmPassword) {
        showToast('error', '输入错误', '请确认密码');
        return false;
    }
    
    if (password !== confirmPassword) {
        showToast('error', '密码不匹配', '两次输入的密码不一致');
        return false;
    }
    
    if (!captcha) {
        showToast('error', '输入错误', '请输入验证码');
        return false;
    }
    
    if (captcha !== currentCaptcha) {
        showToast('error', '验证码错误', '验证码输入错误，请重新输入');
        return false;
    }
    
    if (!agreement) {
        showToast('error', '条款确认', '请阅读并同意服务条款');
        return false;
    }
    
    console.log('表单验证通过');
    return true;
}

// 模拟注册成功
function simulateRegister(username) {
    console.log('开始模拟注册:', username);
    showToast('success', '注册成功', `恭喜 ${username}！您的账号已创建，即将跳转到登录页面`, 3000);
    
    setTimeout(() => {
        console.log('跳转到登录页面');
        window.location.href = 'index.html';
    }, 3000);
}

// 页面初始化
document.addEventListener('DOMContentLoaded', function() {
    console.log('=== 注册页面开始初始化 ===');
    
    // 初始化Feather Icons
    if (typeof feather !== 'undefined') {
        feather.replace();
        console.log('✓ Feather Icons 初始化完成');
    } else {
        console.warn('⚠ Feather Icons 未加载');
    }
    
    // 页面加载器已完全移除
    
    // 生成验证码
    generateCaptcha();
    console.log('✓ 验证码已生成');
    
    // 显示欢迎消息
    setTimeout(() => {
        showToast('info', '欢迎注册', '请填写表单完成账号创建');
    }, 1000);
    
    // 绑定验证码刷新按钮
    const refreshBtn = document.querySelector('.captcha-refresh');
    if (refreshBtn) {
        refreshBtn.addEventListener('click', function(e) {
            e.preventDefault();
            console.log('验证码刷新按钮被点击');
            generateCaptcha();
            const captchaInput = document.getElementById('captcha');
            if (captchaInput) {
                captchaInput.value = '';
            }
        });
        console.log('✓ 验证码刷新按钮事件已绑定');
    } else {
        console.error('✗ 验证码刷新按钮未找到');
    }
    
    // 绑定验证码画布点击
    const canvas = document.getElementById('captcha-canvas');
    if (canvas) {
        canvas.addEventListener('click', function() {
            console.log('验证码画布被点击');
            generateCaptcha();
        });
        console.log('✓ 验证码画布点击事件已绑定');
    }
    
    // 绑定表单提交
    const registerForm = document.getElementById('registerForm');
    if (registerForm) {
        registerForm.addEventListener('submit', function(e) {
            e.preventDefault();
            console.log('表单提交事件触发');
            
            if (validateForm()) {
                const username = document.getElementById('username').value.trim();
                
                // 显示加载状态
                const registerBtn = document.querySelector('.login-btn');
                if (registerBtn) {
                    registerBtn.classList.add('btn-loading');
                    registerBtn.disabled = true;
                }
                
                showToast('info', '处理中', '正在创建您的账号，请稍候...');
                
                setTimeout(() => {
                    // 重置按钮状态
                    if (registerBtn) {
                        registerBtn.classList.remove('btn-loading');
                        registerBtn.disabled = false;
                    }
                    
                    simulateRegister(username);
                }, 2000);
            }
        });
        console.log('✓ 表单提交事件已绑定');
    } else {
        console.error('✗ 注册表单未找到');
    }
    
    // 服务条款功能已移除
    console.log('✓ 服务条款功能已移除');
    
    // 初始化鼠标跟随效果
    initializeMouseFollower();

    // 初始化手写艺术字效果
    initializeHandwritingEffect();

    console.log('=== 注册页面初始化完成 ===');
});

// 鼠标跟随光效
function initializeMouseFollower() {
    const mouseFollower = document.getElementById('mouseFollower');
    if (!mouseFollower) return;

    let mouseX = 0;
    let mouseY = 0;
    let isMoving = false;
    let moveTimeout;

    document.addEventListener('mousemove', function(e) {
        mouseX = e.clientX;
        mouseY = e.clientY;

        mouseFollower.style.left = mouseX + 'px';
        mouseFollower.style.top = mouseY + 'px';

        if (!isMoving) {
            mouseFollower.classList.add('active');
            isMoving = true;
        }

        clearTimeout(moveTimeout);
        moveTimeout = setTimeout(() => {
            mouseFollower.classList.remove('active');
            isMoving = false;
        }, 1000);
    });

    // 鼠标离开页面时隐藏效果
    document.addEventListener('mouseleave', function() {
        mouseFollower.classList.remove('active');
        isMoving = false;
    });

    console.log('✓ 鼠标跟随效果初始化完成');
}

// 手写艺术字效果
function initializeHandwritingEffect() {
    const agreementCheckbox = document.getElementById('agreement');
    const handwritingContainer = document.querySelector('.handwriting-container');

    if (!agreementCheckbox || !handwritingContainer) {
        console.warn('手写艺术字元素未找到');
        return;
    }

    // 监听复选框状态变化
    agreementCheckbox.addEventListener('change', function() {
        if (this.checked) {
            // 勾选时显示并开始手写动画
            showHandwritingAnimation();
        } else {
            // 取消勾选时隐藏
            hideHandwritingAnimation();
        }
    });

    console.log('✓ 手写艺术字效果初始化完成');
}

// 显示手写动画
function showHandwritingAnimation() {
    const handwritingContainer = document.querySelector('.handwriting-container');

    // 首先显示容器
    handwritingContainer.classList.add('show');

    // 延迟一点开始手写动画
    setTimeout(() => {
        handwritingContainer.classList.add('animate');

        // 播放手写音效（如果需要）
        playHandwriteSound();

        // 动画完成后添加发光效果
        setTimeout(() => {
            handwritingContainer.classList.add('complete');
            showToast('success', '欢迎加入！', 'We are on the way! 让我们一起开启数据分析之旅！');
        }, 4500); // 等待所有字母写完（包括箭头和心形）

    }, 300);
}

// 隐藏手写动画
function hideHandwritingAnimation() {
    const handwritingContainer = document.querySelector('.handwriting-container');

    // 移除所有动画类
    handwritingContainer.classList.remove('show', 'animate', 'complete');

    // 重置所有路径的动画状态
    const paths = handwritingContainer.querySelectorAll('path');
    paths.forEach(path => {
        path.style.animation = 'none';
        // 强制重绘
        path.offsetHeight;
        path.style.animation = null;
    });
}

// 播放手写音效（可选）
function playHandwriteSound() {
    // 这里可以添加音效播放代码
    // 例如：new Audio('sounds/handwrite.mp3').play();
    console.log('🎵 播放手写音效');
}
