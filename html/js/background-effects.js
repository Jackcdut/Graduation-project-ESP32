/**
 * 背景动态效果脚本
 * 包含粒子效果、动态背景、页面加载动画等
 */

document.addEventListener('DOMContentLoaded', function() {
    initBackgroundEffects();
    initPageLoader();
});

/**
 * 初始化背景效果
 */
function initBackgroundEffects() {
    createParticles();
    initFloatingShapes();
}

/**
 * 创建粒子效果
 */
function createParticles() {
    const particlesContainer = document.getElementById('particles');
    if (!particlesContainer) return;
    
    // 清除现有粒子
    particlesContainer.innerHTML = '';
    
    const particleCount = 50;
    
    for (let i = 0; i < particleCount; i++) {
        const particle = document.createElement('div');
        particle.className = 'particle';
        
        // 随机位置
        particle.style.left = Math.random() * 100 + '%';
        particle.style.top = Math.random() * 100 + '%';
        
        // 随机动画延迟和持续时间
        particle.style.animationDelay = Math.random() * 15 + 's';
        particle.style.animationDuration = (15 + Math.random() * 10) + 's';
        
        // 随机大小
        const size = 2 + Math.random() * 4;
        particle.style.width = size + 'px';
        particle.style.height = size + 'px';
        
        // 随机透明度
        particle.style.opacity = 0.3 + Math.random() * 0.7;
        
        particlesContainer.appendChild(particle);
    }
}

/**
 * 初始化浮动形状
 */
function initFloatingShapes() {
    const shapes = document.querySelectorAll('.animated-shape');
    
    shapes.forEach((shape, index) => {
        // 为每个形状添加鼠标交互
        shape.addEventListener('mouseenter', function() {
            this.style.animationPlayState = 'paused';
            this.style.transform = 'scale(1.1)';
        });
        
        shape.addEventListener('mouseleave', function() {
            this.style.animationPlayState = 'running';
            this.style.transform = 'scale(1)';
        });
        
        // 随机化动画参数
        const duration = 20 + Math.random() * 15;
        const delay = Math.random() * 5;
        
        shape.style.animationDuration = duration + 's';
        shape.style.animationDelay = delay + 's';
    });
}

/**
 * 初始化页面加载器
 */
function initPageLoader() {
    const pageLoader = document.getElementById('pageLoader');
    if (!pageLoader) return;
    
    // 模拟加载进度
    const loadingBar = pageLoader.querySelector('.loading-bar');
    if (loadingBar) {
        let progress = 0;
        const interval = setInterval(() => {
            progress += Math.random() * 15;
            if (progress >= 100) {
                progress = 100;
                clearInterval(interval);
                
                // 延迟隐藏加载器
                setTimeout(() => {
                    hidePageLoader();
                }, 500);
            }
            loadingBar.style.width = progress + '%';
        }, 100);
    } else {
        // 如果没有进度条，直接延迟隐藏
        setTimeout(() => {
            hidePageLoader();
        }, 1500);
    }
}

/**
 * 隐藏页面加载器
 */
function hidePageLoader() {
    const pageLoader = document.getElementById('pageLoader');
    if (pageLoader) {
        pageLoader.classList.add('loaded');
        
        // 完全移除元素
        setTimeout(() => {
            if (pageLoader.parentNode) {
                pageLoader.parentNode.removeChild(pageLoader);
            }
        }, 600);
    }
}

/**
 * 创建动态光效
 */
function createLightBeams() {
    const lightContainer = document.querySelector('.light-effects');
    if (!lightContainer) return;
    
    const beamCount = 4;
    
    for (let i = 0; i < beamCount; i++) {
        const beam = document.createElement('div');
        beam.className = 'light-beam';
        
        // 随机位置
        beam.style.left = (20 + Math.random() * 60) + '%';
        
        // 随机动画延迟
        beam.style.animationDelay = Math.random() * 12 + 's';
        
        // 随机动画持续时间
        beam.style.animationDuration = (8 + Math.random() * 8) + 's';
        
        lightContainer.appendChild(beam);
    }
}

/**
 * 鼠标跟随效果
 */
function initMouseFollowEffect() {
    let mouseX = 0;
    let mouseY = 0;
    
    document.addEventListener('mousemove', function(e) {
        mouseX = e.clientX;
        mouseY = e.clientY;
        
        // 更新背景渐变位置
        const xPercent = (mouseX / window.innerWidth) * 100;
        const yPercent = (mouseY / window.innerHeight) * 100;
        
        document.body.style.backgroundPosition = 
            `${xPercent}% ${yPercent}%, ${100-xPercent}% ${100-yPercent}%, 50% 50%, 25% 75%, 90% 60%, 0% 0%`;
    });
}

/**
 * 滚动视差效果
 */
function initScrollParallax() {
    window.addEventListener('scroll', function() {
        const scrolled = window.pageYOffset;
        const rate = scrolled * -0.5;
        
        // 移动背景形状
        const shapes = document.querySelectorAll('.animated-shape');
        shapes.forEach((shape, index) => {
            const speed = 0.2 + (index * 0.1);
            shape.style.transform = `translateY(${rate * speed}px)`;
        });
        
        // 移动粒子容器
        const particles = document.getElementById('particles');
        if (particles) {
            particles.style.transform = `translateY(${rate * 0.3}px)`;
        }
    });
}

/**
 * 窗口大小变化处理
 */
function initResizeHandler() {
    window.addEventListener('resize', function() {
        // 重新创建粒子以适应新的窗口大小
        setTimeout(() => {
            createParticles();
        }, 100);
    });
}

/**
 * 页面可见性变化处理
 */
function initVisibilityHandler() {
    document.addEventListener('visibilitychange', function() {
        const shapes = document.querySelectorAll('.animated-shape');
        const particles = document.querySelectorAll('.particle');
        
        if (document.hidden) {
            // 页面隐藏时暂停动画
            shapes.forEach(shape => {
                shape.style.animationPlayState = 'paused';
            });
            particles.forEach(particle => {
                particle.style.animationPlayState = 'paused';
            });
        } else {
            // 页面显示时恢复动画
            shapes.forEach(shape => {
                shape.style.animationPlayState = 'running';
            });
            particles.forEach(particle => {
                particle.style.animationPlayState = 'running';
            });
        }
    });
}

/**
 * 性能优化：减少动画在低性能设备上的影响
 */
function initPerformanceOptimization() {
    // 检测设备性能
    const isLowPerformance = navigator.hardwareConcurrency < 4 || 
                            /Android|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent);
    
    if (isLowPerformance) {
        // 减少粒子数量
        const particlesContainer = document.getElementById('particles');
        if (particlesContainer) {
            const particles = particlesContainer.querySelectorAll('.particle');
            particles.forEach((particle, index) => {
                if (index % 2 === 0) {
                    particle.remove();
                }
            });
        }
        
        // 简化动画
        const shapes = document.querySelectorAll('.animated-shape');
        shapes.forEach(shape => {
            shape.style.animationDuration = '30s'; // 减慢动画速度
        });
    }
}

// 初始化所有效果
document.addEventListener('DOMContentLoaded', function() {
    initMouseFollowEffect();
    initScrollParallax();
    initResizeHandler();
    initVisibilityHandler();
    initPerformanceOptimization();
    createLightBeams();
});
