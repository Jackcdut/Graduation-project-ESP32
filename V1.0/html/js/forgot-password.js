// 初始化忘记密码页面
document.addEventListener('DOMContentLoaded', function() {
    // 添加表单动画效果
    initializeFormEffects();
    
    // 初始化返回登录按钮
    const backToLoginBtn = document.getElementById('backToLogin');
    if (backToLoginBtn) {
        backToLoginBtn.addEventListener('click', function() {
            window.location.href = 'index.html';
        });
    }
});

// 初始化表单动画效果
function initializeFormEffects() {
    // 输入框焦点效果
    const inputs = document.querySelectorAll('.input-group input');
    inputs.forEach(input => {
        input.addEventListener('focus', function() {
            this.parentElement.classList.add('focused');
        });
        
        input.addEventListener('blur', function() {
            if (!this.value) {
                this.parentElement.classList.remove('focused');
            }
        });
        
        // 检查初始状态
        if (input.value) {
            input.parentElement.classList.add('focused');
        }
    });
}

// 重置密码表单处理
const resetForm = document.getElementById('resetForm');
if (resetForm) {
    resetForm.addEventListener('submit', function(e) {
        e.preventDefault();
        
        const email = document.getElementById('reset_email').value;
        
        // 验证邮箱
        const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
        if (!email || !emailRegex.test(email)) {
            showInputError('reset_email', '请输入有效的电子邮箱');
            return;
        }
        
        // 显示加载状态
        const resetBtn = resetForm.querySelector('.login-btn');
        resetBtn.classList.add('btn-loading');
        resetBtn.disabled = true;
        
        // 模拟发送重置邮件
        setTimeout(function() {
            // 这里应该发送真实的API请求
            showMessage('重置链接已发送到您的邮箱', 'success');
            
            // 显示确认步骤
            transitionToConfirmStep(email);
            
        }, 1500);
    });
}

// 显示输入错误
function showInputError(inputId, message) {
    const input = document.getElementById(inputId);
    input.classList.add('input-error');
    showMessage(message, 'error');
    
    setTimeout(() => {
        input.classList.remove('input-error');
    }, 1000);
}

// 转换到确认步骤
function transitionToConfirmStep(email) {
    const stepEmail = document.getElementById('step-email');
    const stepConfirm = document.getElementById('step-confirm');
    
    // 添加淡出动画
    stepEmail.classList.add('animate__animated', 'animate__fadeOut');
    
    setTimeout(() => {
        stepEmail.style.display = 'none';
        stepConfirm.style.display = 'block';
        
        // 添加淡入动画
        stepConfirm.classList.add('animate__animated', 'animate__fadeIn');
        
        // 触发成功图标动画
        const successIcon = document.querySelector('.success-icon');
        if (successIcon) {
            successIcon.classList.add('animate__animated', 'animate__zoomIn');
        }
    }, 500);
}

// 重新发送链接
const resendLink = document.getElementById('resendLink');
if (resendLink) {
    resendLink.addEventListener('click', function(e) {
        e.preventDefault();
        
        // 显示发送中状态
        const originalText = resendLink.textContent;
        resendLink.innerHTML = '<span class="iconfont icon-loading loading-spin"></span> 发送中...';
        resendLink.style.pointerEvents = 'none';
        
        // 模拟重新发送
        setTimeout(function() {
            showMessage('重置链接已重新发送', 'info');
            resendLink.textContent = originalText;
            resendLink.style.pointerEvents = 'auto';
            
            // 添加提示动画
            resendLink.classList.add('animate__animated', 'animate__pulse');
            setTimeout(() => {
                resendLink.classList.remove('animate__animated', 'animate__pulse');
            }, 1000);
        }, 1500);
    });
}
