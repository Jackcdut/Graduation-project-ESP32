/**
 * 数据分析平台 - 性能优化器
 * 负责资源预加载、缓存管理、性能监控等功能
 */

class PerformanceOptimizer {
    constructor() {
        this.loadStartTime = performance.now();
        this.resourceCache = new Map();
        this.loadedResources = new Set();
        this.criticalResources = ['chart.js', 'fontawesome', 'xlsx'];
        this.performanceMetrics = {};
        
        this.init();
    }

    init() {
        this.setupServiceWorker();
        this.preloadCriticalResources();
        this.setupPerformanceMonitoring();
        this.optimizeImages();
        this.setupResourceHints();
    }

    /**
     * 设置Service Worker进行缓存管理
     */
    setupServiceWorker() {
        if ('serviceWorker' in navigator) {
            navigator.serviceWorker.register('/sw.js')
                .then(registration => {
                    console.log('✓ Service Worker 注册成功:', registration.scope);
                })
                .catch(error => {
                    console.log('⚠ Service Worker 注册失败:', error);
                });
        }
    }

    /**
     * 预加载关键资源
     */
    preloadCriticalResources() {
        const criticalResources = [
            { href: 'libs/css/fontawesome/all.min.css', as: 'style' },
            { href: 'libs/js/chart/chart.min.js', as: 'script' },
            { href: 'libs/js/xlsx/xlsx.full.min.js', as: 'script' },
            { href: 'libs/fonts/inter/inter.css', as: 'style' }
        ];

        criticalResources.forEach(resource => {
            const link = document.createElement('link');
            link.rel = 'preload';
            link.href = resource.href;
            link.as = resource.as;
            link.crossOrigin = 'anonymous';
            
            // 检查资源是否存在
            link.onerror = () => {
                console.warn(`⚠ 预加载失败: ${resource.href}`);
            };
            
            link.onload = () => {
                console.log(`✓ 预加载成功: ${resource.href}`);
                this.loadedResources.add(resource.href);
            };
            
            document.head.appendChild(link);
        });
    }

    /**
     * 设置性能监控
     */
    setupPerformanceMonitoring() {
        // 监控页面加载性能
        window.addEventListener('load', () => {
            setTimeout(() => {
                this.collectPerformanceMetrics();
                this.reportPerformance();
            }, 1000);
        });

        // 监控资源加载错误
        window.addEventListener('error', (e) => {
            if (e.target.tagName === 'SCRIPT' || e.target.tagName === 'LINK') {
                console.error('资源加载失败:', e.target.src || e.target.href);
                this.handleResourceError(e.target);
            }
        }, true);
    }

    /**
     * 收集性能指标
     */
    collectPerformanceMetrics() {
        const navigation = performance.getEntriesByType('navigation')[0];
        const paint = performance.getEntriesByType('paint');
        
        this.performanceMetrics = {
            // 页面加载指标
            domContentLoaded: navigation.domContentLoadedEventEnd - navigation.domContentLoadedEventStart,
            loadComplete: navigation.loadEventEnd - navigation.loadEventStart,
            
            // 渲染指标
            firstPaint: paint.find(entry => entry.name === 'first-paint')?.startTime || 0,
            firstContentfulPaint: paint.find(entry => entry.name === 'first-contentful-paint')?.startTime || 0,
            
            // 网络指标
            dnsLookup: navigation.domainLookupEnd - navigation.domainLookupStart,
            tcpConnect: navigation.connectEnd - navigation.connectStart,
            
            // 自定义指标
            totalLoadTime: performance.now() - this.loadStartTime,
            resourcesLoaded: this.loadedResources.size,
            criticalResourcesStatus: this.checkCriticalResources()
        };
    }

    /**
     * 检查关键资源状态
     */
    checkCriticalResources() {
        return {
            chart: typeof Chart !== 'undefined',
            xlsx: typeof XLSX !== 'undefined',
            fontawesome: this.checkFontAwesome(),
            particles: typeof particlesJS !== 'undefined'
        };
    }

    /**
     * 检查Font Awesome是否加载
     */
    checkFontAwesome() {
        const testElement = document.createElement('i');
        testElement.className = 'fas fa-check';
        testElement.style.position = 'absolute';
        testElement.style.left = '-9999px';
        document.body.appendChild(testElement);
        
        const computed = window.getComputedStyle(testElement);
        const isLoaded = computed.fontFamily.includes('Font Awesome');
        
        document.body.removeChild(testElement);
        return isLoaded;
    }

    /**
     * 报告性能数据
     */
    reportPerformance() {
        console.group('📊 性能报告');
        console.log('页面加载时间:', this.performanceMetrics.totalLoadTime.toFixed(2) + 'ms');
        console.log('DOM内容加载:', this.performanceMetrics.domContentLoaded.toFixed(2) + 'ms');
        console.log('首次绘制:', this.performanceMetrics.firstPaint.toFixed(2) + 'ms');
        console.log('首次内容绘制:', this.performanceMetrics.firstContentfulPaint.toFixed(2) + 'ms');
        console.log('已加载资源数:', this.performanceMetrics.resourcesLoaded);
        console.log('关键资源状态:', this.performanceMetrics.criticalResourcesStatus);
        console.groupEnd();

        // 性能建议
        this.providePerformanceAdvice();
    }

    /**
     * 提供性能建议
     */
    providePerformanceAdvice() {
        const advice = [];
        
        if (this.performanceMetrics.totalLoadTime > 3000) {
            advice.push('页面加载时间较长，建议优化资源大小');
        }
        
        if (this.performanceMetrics.firstContentfulPaint > 2000) {
            advice.push('首次内容绘制较慢，建议优化关键渲染路径');
        }
        
        if (!this.performanceMetrics.criticalResourcesStatus.chart) {
            advice.push('Chart.js 未正确加载，图表功能可能受影响');
        }
        
        if (advice.length > 0) {
            console.group('💡 性能建议');
            advice.forEach(tip => console.warn(tip));
            console.groupEnd();
        }
    }

    /**
     * 处理资源加载错误
     */
    handleResourceError(element) {
        const src = element.src || element.href;
        const resourceType = element.tagName.toLowerCase();
        
        // 尝试从备用源加载
        if (src.includes('cdn.bootcdn.net')) {
            element.src = src.replace('cdn.bootcdn.net', 'cdnjs.cloudflare.com');
        } else if (src.includes('cdnjs.cloudflare.com')) {
            element.src = src.replace('cdnjs.cloudflare.com', 'unpkg.com');
        }
    }

    /**
     * 优化图片加载
     */
    optimizeImages() {
        // 懒加载图片
        if ('IntersectionObserver' in window) {
            const imageObserver = new IntersectionObserver((entries) => {
                entries.forEach(entry => {
                    if (entry.isIntersecting) {
                        const img = entry.target;
                        if (img.dataset.src) {
                            img.src = img.dataset.src;
                            img.removeAttribute('data-src');
                            imageObserver.unobserve(img);
                        }
                    }
                });
            });

            document.querySelectorAll('img[data-src]').forEach(img => {
                imageObserver.observe(img);
            });
        }

        // WebP支持检测
        this.checkWebPSupport();
    }

    /**
     * 检查WebP支持
     */
    checkWebPSupport() {
        const webP = new Image();
        webP.onload = webP.onerror = () => {
            const support = webP.height === 2;
            document.documentElement.classList.toggle('webp', support);
            console.log(support ? '✓ WebP 支持' : '⚠ WebP 不支持');
        };
        webP.src = 'data:image/webp;base64,UklGRjoAAABXRUJQVlA4IC4AAACyAgCdASoCAAIALmk0mk0iIiIiIgBoSygABc6WWgAA/veff/0PP8bA//LwYAAA';
    }

    /**
     * 设置资源提示
     */
    setupResourceHints() {
        // DNS预解析
        const dnsPrefetch = [
            'fonts.googleapis.com',
            'fonts.gstatic.com',
            'cdn.jsdelivr.net',
            'cdnjs.cloudflare.com'
        ];

        dnsPrefetch.forEach(domain => {
            const link = document.createElement('link');
            link.rel = 'dns-prefetch';
            link.href = `//${domain}`;
            document.head.appendChild(link);
        });

        // 预连接关键资源
        const preconnect = [
            'https://fonts.googleapis.com',
            'https://fonts.gstatic.com'
        ];

        preconnect.forEach(url => {
            const link = document.createElement('link');
            link.rel = 'preconnect';
            link.href = url;
            link.crossOrigin = 'anonymous';
            document.head.appendChild(link);
        });
    }

    /**
     * 压缩和合并CSS
     */
    static compressCSS(css) {
        return css
            .replace(/\/\*[\s\S]*?\*\//g, '') // 移除注释
            .replace(/\s+/g, ' ') // 压缩空白
            .replace(/;\s*}/g, '}') // 移除最后的分号
            .replace(/\s*{\s*/g, '{') // 压缩大括号
            .replace(/;\s*/g, ';') // 压缩分号
            .trim();
    }

    /**
     * 获取性能指标
     */
    getMetrics() {
        return this.performanceMetrics;
    }
}

// 创建全局实例
window.performanceOptimizer = new PerformanceOptimizer();

// 导出性能数据到控制台（开发模式）
if (window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1') {
    window.perfReport = () => {
        console.table(window.performanceOptimizer.getMetrics());
    };
} 