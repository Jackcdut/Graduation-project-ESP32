// 增强版忘记密码页面脚本
class ForgotPasswordEnhanced {
    constructor() {
        this.init();
    }

    init() {
        this.bindEventListeners();
        this.initializeParticles();
        this.removePageLoader();
    }

    bindEventListeners() {
        // 重新发送链接按钮
        const resendLink = document.getElementById('resendLink');
        if (resendLink) {
            resendLink.addEventListener('click', (e) => {
                e.preventDefault();
                this.resendPasswordReset();
            });
        }

        // 返回登录按钮
        const backToLoginBtn = document.getElementById('backToLogin');
        if (backToLoginBtn) {
            backToLoginBtn.addEventListener('click', () => {
                window.location.href = 'index.html';
            });
        }

        // 邮箱输入框实时验证
        const resetEmailInput = document.getElementById('reset_email');
        if (resetEmailInput) {
            resetEmailInput.addEventListener('blur', () => {
                this.validateEmailField(resetEmailInput.value);
            });
            
            resetEmailInput.addEventListener('input', () => {
                this.clearEmailError();
            });
        }
    }

    initializeParticles() {
        // 初始化粒子效果
        if (window.particlesJS && document.getElementById('particles-js')) {
            particlesJS('particles-js', {
                "particles": {
                    "number": {
                        "value": 50,
                        "density": {
                            "enable": true,
                            "value_area": 800
                        }
                    },
                    "color": {
                        "value": "#667eea"
                    },
                    "shape": {
                        "type": "circle"
                    },
                    "opacity": {
                        "value": 0.3,
                        "random": true,
                        "anim": {
                            "enable": true,
                            "speed": 1,
                            "opacity_min": 0.1,
                            "sync": false
                        }
                    },
                    "size": {
                        "value": 3,
                        "random": true,
                        "anim": {
                            "enable": true,
                            "speed": 2,
                            "size_min": 0.1,
                            "sync": false
                        }
                    },
                    "line_linked": {
                        "enable": true,
                        "distance": 150,
                        "color": "#667eea",
                        "opacity": 0.2,
                        "width": 1
                    },
                    "move": {
                        "enable": true,
                        "speed": 1,
                        "direction": "none",
                        "random": false,
                        "straight": false,
                        "out_mode": "out",
                        "bounce": false
                    }
                },
                "interactivity": {
                    "detect_on": "canvas",
                    "events": {
                        "onhover": {
                            "enable": true,
                            "mode": "grab"
                        },
                        "onclick": {
                            "enable": true,
                            "mode": "push"
                        },
                        "resize": true
                    },
                    "modes": {
                        "grab": {
                            "distance": 140,
                            "line_linked": {
                                "opacity": 0.5
                            }
                        },
                        "push": {
                            "particles_nb": 4
                        }
                    }
                },
                "retina_detect": true
            });
        }
    }

    removePageLoader() {
        // 移除页面加载器
        setTimeout(() => {
            const loader = document.querySelector('.page-loader');
            if (loader) {
                loader.style.opacity = '0';
                loader.style.visibility = 'hidden';
                setTimeout(() => {
                    loader.style.display = 'none';
                }, 500);
            }
        }, 800);
    }

    async resendPasswordReset() {
        const emailInput = document.getElementById('reset_email');
        if (!emailInput) return;

        const email = emailInput.value.trim();
        
        if (!this.validateEmail(email)) {
            this.showNotification('请输入有效的邮箱地址', 'error');
            return;
        }

        try {
            // 更新按钮状态
            const resendLink = document.getElementById('resendLink');
            const originalText = resendLink.textContent;
            resendLink.textContent = '发送中...';
            resendLink.style.pointerEvents = 'none';

            // 调用认证系统重置密码
            if (window.authSystem) {
                const result = await window.uniCloudService.resetPassword(email);
                
                if (result.result?.success) {
                    this.showNotification('重置邮件已重新发送', 'success');
                    this.startResendCooldown(resendLink);
                } else {
                    throw new Error(result.result?.message || '发送失败');
                }
            } else {
                // 备用方案
                await this.simulatePasswordReset();
                this.showNotification('重置邮件已重新发送', 'success');
                this.startResendCooldown(resendLink);
            }

        } catch (error) {
            console.error('重新发送失败:', error);
            this.showNotification('重新发送失败：' + error.message, 'error');
            
            // 恢复按钮状态
            const resendLink = document.getElementById('resendLink');
            resendLink.textContent = originalText;
            resendLink.style.pointerEvents = 'auto';
        }
    }

    startResendCooldown(element, seconds = 60) {
        let countdown = seconds;
        
        const updateCountdown = () => {
            element.textContent = `重新发送 (${countdown}s)`;
            
            if (countdown > 0) {
                countdown--;
                setTimeout(updateCountdown, 1000);
            } else {
                element.textContent = '点击此处';
                element.style.pointerEvents = 'auto';
            }
        };
        
        updateCountdown();
    }

    async simulatePasswordReset() {
        // 模拟网络延迟
        return new Promise(resolve => setTimeout(resolve, 1500));
    }

    validateEmailField(email) {
        if (!email) return;
        
        if (!this.validateEmail(email)) {
            this.showEmailError('请输入有效的邮箱地址');
            return false;
        } else {
            this.clearEmailError();
            return true;
        }
    }

    validateEmail(email) {
        const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
        return emailRegex.test(email);
    }

    showEmailError(message) {
        const emailInput = document.getElementById('reset_email');
        if (!emailInput) return;

        const inputGroup = emailInput.closest('.input-group');
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

    clearEmailError() {
        const emailInput = document.getElementById('reset_email');
        if (!emailInput) return;

        const inputGroup = emailInput.closest('.input-group');
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

    // 添加页面切换效果
    navigateWithTransition(url) {
        document.body.style.opacity = '0';
        setTimeout(() => {
            window.location.href = url;
        }, 300);
    }
}

// 页面加载完成后初始化
document.addEventListener('DOMContentLoaded', () => {
    // 等待一小段时间确保其他脚本加载完成
    setTimeout(() => {
        window.forgotPasswordEnhanced = new ForgotPasswordEnhanced();
    }, 100);
});

// 导出供其他模块使用
if (typeof module !== 'undefined' && module.exports) {
    module.exports = ForgotPasswordEnhanced;
} 