// 配置粒子动画效果
document.addEventListener('DOMContentLoaded', function() {
    // 页面加载动画
    setTimeout(function() {
        document.querySelector('.page-loader').classList.add('loaded');
    }, 1500);

    // 修复复选框功能
    const rememberCheckbox = document.getElementById('remember');
    const customCheckbox = document.querySelector('.custom-checkbox');

    if (rememberCheckbox && customCheckbox) {
        // 点击标签时切换复选框状态
        customCheckbox.addEventListener('click', function(e) {
            if (e.target !== rememberCheckbox) {
                e.preventDefault();
                rememberCheckbox.checked = !rememberCheckbox.checked;
                updateCheckboxAppearance();
            }
        });

        // 监听复选框状态变化
        rememberCheckbox.addEventListener('change', updateCheckboxAppearance);

        function updateCheckboxAppearance() {
            if (rememberCheckbox.checked) {
                customCheckbox.classList.add('checked');
            } else {
                customCheckbox.classList.remove('checked');
            }
        }

        // 初始化状态
        updateCheckboxAppearance();
    }
    
    // Particles.js配置
    if(document.getElementById('particles-js')) {
        particlesJS('particles-js', {
            "particles": {
                "number": {
                    "value": 80,
                    "density": {
                        "enable": true,
                        "value_area": 800
                    }
                },
                "color": {
                    "value": "#2c7be5"
                },
                "shape": {
                    "type": "circle",
                    "stroke": {
                        "width": 0,
                        "color": "#000000"
                    },
                    "polygon": {
                        "nb_sides": 5
                    }
                },
                "opacity": {
                    "value": 0.4,
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
                    "color": "#5499ff",
                    "opacity": 0.3,
                    "width": 1
                },
                "move": {
                    "enable": true,
                    "speed": 1,
                    "direction": "none",
                    "random": false,
                    "straight": false,
                    "out_mode": "out",
                    "bounce": false,
                    "attract": {
                        "enable": true,
                        "rotateX": 600,
                        "rotateY": 1200
                    }
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
                            "opacity": 0.8
                        }
                    },
                    "bubble": {
                        "distance": 400,
                        "size": 40,
                        "duration": 2,
                        "opacity": 8,
                        "speed": 3
                    },
                    "repulse": {
                        "distance": 200,
                        "duration": 0.4
                    },
                    "push": {
                        "particles_nb": 4
                    },
                    "remove": {
                        "particles_nb": 2
                    }
                }
            },
            "retina_detect": true
        });
    }

    // 表单验证
    const loginForm = document.getElementById('loginForm');
    const usernameInput = document.getElementById('username');
    const passwordInput = document.getElementById('password');
    const passwordToggle = document.querySelector('.password-toggle');

    // 禁用浏览器默认验证
    if (loginForm) {
        loginForm.setAttribute('novalidate', 'true');
    }

    // 为所有输入框添加自定义验证
    [usernameInput, passwordInput].forEach(input => {
        if (input) {
            input.addEventListener('invalid', function(e) {
                e.preventDefault(); // 阻止默认验证提示
                showCustomValidation(this);
            });

            input.addEventListener('input', function() {
                if (this.validity.valid || this.value.trim() !== '') {
                    clearError(this);
                }
            });
        }
    });

    // 自定义验证提示函数
    function showCustomValidation(input) {
        const inputGroup = input.closest('.input-group');
        const validationMessage = inputGroup.querySelector('.validation-message');

        let message = '';
        if (input.validity.valueMissing || input.value.trim() === '') {
            message = input.type === 'password' ? '请输入密码' : '请输入用户名';
        } else if (input.validity.tooShort) {
            message = '输入内容太短';
        } else if (input.validity.patternMismatch) {
            message = '输入格式不正确';
        }

        if (message && validationMessage) {
            validationMessage.textContent = message;
            showError(input, message);
        }
    }
    const passwordStrength = document.querySelector('.password-strength');
    const passwordStrengthBar = document.querySelector('.password-strength-bar');
    const passwordStrengthText = document.querySelector('.password-strength-text');

    // 密码强度检测
    if(passwordInput) {
        passwordInput.addEventListener('input', function() {
            const password = this.value;
            if(password.length > 0) {
                passwordStrength.classList.add('show');
                passwordStrengthText.classList.add('show');

                // 简单的密码强度检测
                let strength = 0;
                if(password.length > 6) strength += 25;
                if(password.match(/[a-z]+/)) strength += 25;
                if(password.match(/[A-Z]+/)) strength += 25;
                if(password.match(/[0-9]+/)) strength += 25;
                if(password.match(/[^a-zA-Z0-9]+/)) strength += 25;

                // 设置强度等级
                passwordStrength.className = 'password-strength show';
                passwordStrengthText.className = 'password-strength-text show';
                
                if(strength <= 25) {
                    passwordStrength.classList.add('weak');
                    passwordStrengthText.classList.add('weak');
                    passwordStrengthText.textContent = '密码强度：弱';
                } else if(strength <= 50) {
                    passwordStrength.classList.add('medium');
                    passwordStrengthText.classList.add('medium');
                    passwordStrengthText.textContent = '密码强度：中';
                } else if(strength <= 75) {
                    passwordStrength.classList.add('good');
                    passwordStrengthText.classList.add('good');
                    passwordStrengthText.textContent = '密码强度：好';
                } else {
                    passwordStrength.classList.add('strong');
                    passwordStrengthText.classList.add('strong');
                    passwordStrengthText.textContent = '密码强度：强';
                }
            } else {
                passwordStrength.classList.remove('show');
                passwordStrengthText.classList.remove('show');
            }
        });
    }

    // 密码显示/隐藏切换（当前页面没有此功能，暂时注释）
    /*
    if(passwordToggle) {
        passwordToggle.addEventListener('click', function() {
            const type = passwordInput.getAttribute('type') === 'password' ? 'text' : 'password';
            passwordInput.setAttribute('type', type);

            // 更新图标和标题
            const eyeIcon = this.querySelector('.iconfont');
            if(type === 'text') {
                eyeIcon.classList.remove('icon-eye');
                eyeIcon.classList.add('icon-eye-close');
                this.setAttribute('title', '隐藏密码');
            } else {
                eyeIcon.classList.remove('icon-eye-close');
                eyeIcon.classList.add('icon-eye');
                this.setAttribute('title', '显示密码');
            }
        });
    }
    */

    // 表单输入字段聚焦效果
    const inputGroups = document.querySelectorAll('.input-group');
    inputGroups.forEach(group => {
        const input = group.querySelector('input');
        
        input.addEventListener('focus', function() {
            group.classList.add('focused');
        });
        
        input.addEventListener('blur', function() {
            group.classList.remove('focused');
            if(this.value.length > 0) {
                group.classList.add('filled');
                validateInput(this);
            } else {
                group.classList.remove('filled');
                group.classList.remove('input-validated');
            }
        });
        
        // 初始化状态
        if(input.value.length > 0) {
            group.classList.add('filled');
        }
    });

    // 简单输入验证
    function validateInput(input) {
        const inputGroup = input.closest('.input-group');
        const value = input.value.trim();

        // 清除之前的错误状态
        clearError(input);

        // 根据输入类型进行验证
        if (input.type === 'text' && input.id === 'username') {
            if (value.length === 0) {
                return; // 空值不显示错误，等待提交时验证
            } else if (value.length < 3) {
                showError(input, '用户名至少需要3个字符');
                return;
            } else {
                showSuccess(input);
            }
        } else if (input.type === 'password') {
            if (value.length === 0) {
                return; // 空值不显示错误，等待提交时验证
            } else if (value.length < 6) {
                showError(input, '密码至少需要6个字符');
                return;
            } else {
                showSuccess(input);
                updatePasswordStrength(value);
            }
        }
    }

    // 更新密码强度指示器
    function updatePasswordStrength(password) {
        const strengthBar = document.querySelector('.password-strength-bar');
        const strengthText = document.querySelector('.password-strength-text');

        if (!strengthBar || !strengthText) return;

        let strength = 0;
        let strengthLabel = '弱';
        let strengthColor = '#e63757';

        // 计算密码强度
        if (password.length >= 6) strength += 1;
        if (password.length >= 8) strength += 1;
        if (/[A-Z]/.test(password)) strength += 1;
        if (/[0-9]/.test(password)) strength += 1;
        if (/[^A-Za-z0-9]/.test(password)) strength += 1;

        // 设置强度等级
        if (strength >= 4) {
            strengthLabel = '强';
            strengthColor = '#00d97e';
        } else if (strength >= 2) {
            strengthLabel = '中';
            strengthColor = '#f6c343';
        }

        // 更新UI
        strengthBar.style.width = `${(strength / 5) * 100}%`;
        strengthBar.style.backgroundColor = strengthColor;
        strengthText.textContent = `密码强度：${strengthLabel}`;
        strengthText.style.color = strengthColor;
    }

    // 表单提交处理
    if(loginForm) {
        console.log('登录表单事件监听器已绑定');
        loginForm.addEventListener('submit', function(e) {
            console.log('登录表单提交事件触发');
            e.preventDefault();

            let valid = true;
            
            // 验证用户名
            if(usernameInput.value.trim() === '') {
                showError(usernameInput, '请输入用户名');
                valid = false;
            } else {
                usernameInput.closest('.input-group').classList.remove('input-error');
            }
            
            // 验证密码
            if(passwordInput.value.trim() === '') {
                showError(passwordInput, '请输入密码');
                valid = false;
            } else {
                passwordInput.closest('.input-group').classList.remove('input-error');
            }
            
            if(valid) {
                // 显示加载状态
                const loginBtn = this.querySelector('.login-btn');
                loginBtn.classList.add('btn-loading');
                
                // 模拟登录请求
                setTimeout(function() {
                    loginBtn.classList.remove('btn-loading');

                    // 模拟登录验证 - 允许任意用户名和密码登录
                    const username = usernameInput.value.trim();
                    const password = passwordInput.value.trim();

                    // 只要用户名和密码不为空就允许登录
                    if (username.length > 0 && password.length > 0) {
                        console.log('登录验证通过，用户名:', username);

                        // 显示成功状态
                        showSuccess(usernameInput);
                        showSuccess(passwordInput);
                        showMessage('登录成功，正在跳转到数据分析平台...', 'success');

                        // 保存用户信息到本地存储
                        localStorage.setItem('currentUser', username);
                        localStorage.setItem('loginTime', new Date().toISOString());
                        console.log('用户信息已保存到localStorage');

                        // 跳转到数据分析页面
                        setTimeout(function() {
                            console.log('正在跳转到data.html...');
                            window.location.replace('data.html');
                        }, 1500);
                    } else {
                        console.log('登录验证失败，用户名或密码为空');
                        // 显示错误信息
                        showLoginError('请输入用户名和密码');
                    }
                }, 1500);
            }
        });
    }

    // 显示错误消息
    function showError(input, message) {
        const inputGroup = input.closest('.input-group');
        const validationMessage = inputGroup.querySelector('.validation-message');
        const validationIcon = inputGroup.querySelector('.validation-icon.error');

        inputGroup.classList.add('input-error');
        inputGroup.classList.remove('success');

        if (validationMessage) {
            validationMessage.textContent = message;
            validationMessage.style.display = 'block';
            setTimeout(() => {
                validationMessage.style.opacity = '1';
                validationMessage.style.transform = 'translateY(0)';
            }, 10);
        }

        // 不对密码输入框显示验证图标
        if (validationIcon && input.type !== 'password' && input.id !== 'password') {
            validationIcon.classList.add('show');
        }

        // 添加震动效果
        input.classList.add('shake-animation');
        setTimeout(() => {
            input.classList.remove('shake-animation');
        }, 600);
    }

    // 清除错误状态
    function clearError(input) {
        const inputGroup = input.closest('.input-group');
        const validationMessage = inputGroup.querySelector('.validation-message');
        const validationIcon = inputGroup.querySelector('.validation-icon');

        inputGroup.classList.remove('input-error');

        if (validationMessage) {
            validationMessage.style.opacity = '0';
            validationMessage.style.transform = 'translateY(-10px)';
            setTimeout(() => {
                validationMessage.style.display = 'none';
            }, 300);
        }

        if (validationIcon) {
            validationIcon.classList.remove('show');
        }
    }

    // 显示成功状态
    function showSuccess(input) {
        const inputGroup = input.closest('.input-group');
        const validationIcon = inputGroup.querySelector('.validation-icon.success');

        inputGroup.classList.add('success');
        inputGroup.classList.remove('input-error');

        // 不对密码输入框显示验证图标
        if (validationIcon && input.type !== 'password' && input.id !== 'password') {
            validationIcon.classList.add('show');
        }
    }

    // 显示消息提示框
    function showMessage(message, type = 'info') {
        // 创建通知元素
        const notification = document.createElement('div');
        notification.className = `notification notification-${type} notification-top-center`;

        // 设置图标
        let iconClass = '';
        switch(type) {
            case 'success':
                iconClass = 'icon-check';
                break;
            case 'error':
                iconClass = 'icon-close';
                break;
            case 'warning':
                iconClass = 'icon-warning';
                break;
            default:
                iconClass = 'icon-info';
        }

        notification.innerHTML = `
            <div class="notification-icon">
                <span class="iconfont ${iconClass}">
                    <span class="icon ${iconClass}"></span>
                </span>
            </div>
            <div class="notification-content">
                <div class="notification-title">${getNotificationTitle(type)}</div>
                <div class="notification-message">${message}</div>
            </div>
            <button class="notification-close" onclick="this.parentElement.remove()">
                <span class="iconfont icon-close">
                    <span class="icon icon-close"></span>
                </span>
            </button>
        `;

        // 添加到页面
        document.body.appendChild(notification);

        // 自动移除
        setTimeout(() => {
            if (notification.parentElement) {
                notification.style.animation = 'fadeOut 0.5s';
                setTimeout(() => {
                    notification.remove();
                }, 500);
            }
        }, 4000);
    }

    // 获取通知标题
    function getNotificationTitle(type) {
        switch(type) {
            case 'success': return '成功';
            case 'error': return '错误';
            case 'warning': return '警告';
            default: return '提示';
        }
    }

    // 显示登录错误
    function showLoginError(message) {
        showMessage(message, 'error');

        // 同时显示表单错误
        if (message.includes('用户名')) {
            showError(usernameInput, '用户名错误');
        }
        if (message.includes('密码')) {
            showError(passwordInput, '密码错误');
        }
    }


    // 图标字体加载检测和备用方案
    function checkIconFonts() {
        // 检测iconfont字体是否加载成功
        const testElement = document.createElement('span');
        testElement.className = 'iconfont icon-user';
        testElement.style.position = 'absolute';
        testElement.style.left = '-9999px';
        testElement.style.fontSize = '24px';
        document.body.appendChild(testElement);

        // 检查字体是否正确渲染
        setTimeout(() => {
            const computedStyle = window.getComputedStyle(testElement);
            const fontFamily = computedStyle.fontFamily;

            // 如果字体未正确加载，启用备用方案
            if (!fontFamily.includes('iconfont')) {
                document.querySelectorAll('.iconfont').forEach(icon => {
                    icon.setAttribute('data-fallback', 'true');
                });
            }

            document.body.removeChild(testElement);
        }, 100);
    }

    // 页面加载完成后检查图标字体
    checkIconFonts();
});
