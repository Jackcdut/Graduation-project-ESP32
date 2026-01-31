/**
 * 资源加载器 - 提供CDN失败时的本地fallback
 */
class ResourceLoader {
    constructor() {
        this.retryCount = 3;
        this.timeout = 10000; // 10秒超时
    }

    // 加载CSS文件
    loadCSS(urls, fallbackUrl = null) {
        return new Promise((resolve, reject) => {
            const link = document.createElement('link');
            link.rel = 'stylesheet';
            link.type = 'text/css';
            
            let currentIndex = 0;
            const tryLoad = () => {
                if (currentIndex >= urls.length) {
                    if (fallbackUrl) {
                        link.href = fallbackUrl;
                        document.head.appendChild(link);
                        resolve();
                        return;
                    }
                    reject(new Error('所有CSS资源加载失败'));
                    return;
                }
                
                link.href = urls[currentIndex];
                
                const timeout = setTimeout(() => {
                    currentIndex++;
                    tryLoad();
                }, this.timeout);
                
                link.onload = () => {
                    clearTimeout(timeout);
                    resolve();
                };
                
                link.onerror = () => {
                    clearTimeout(timeout);
                    currentIndex++;
                    tryLoad();
                };
                
                document.head.appendChild(link);
            };
            
            tryLoad();
        });
    }

    // 加载JavaScript文件
    loadJS(urls, fallbackUrl = null) {
        return new Promise((resolve, reject) => {
            let currentIndex = 0;
            const tryLoad = () => {
                if (currentIndex >= urls.length) {
                    if (fallbackUrl) {
                        this.loadSingleJS(fallbackUrl).then(resolve).catch(reject);
                        return;
                    }
                    reject(new Error('所有JS资源加载失败'));
                    return;
                }
                
                this.loadSingleJS(urls[currentIndex])
                    .then(resolve)
                    .catch(() => {
                        currentIndex++;
                        tryLoad();
                    });
            };
            
            tryLoad();
        });
    }
    
    loadSingleJS(url) {
        return new Promise((resolve, reject) => {
            const script = document.createElement('script');
            script.src = url;
            script.type = 'text/javascript';
            
            const timeout = setTimeout(() => {
                reject(new Error(`JS加载超时: ${url}`));
            }, this.timeout);
            
            script.onload = () => {
                clearTimeout(timeout);
                resolve();
            };
            
            script.onerror = () => {
                clearTimeout(timeout);
                reject(new Error(`JS加载失败: ${url}`));
            };
            
            document.head.appendChild(script);
        });
    }
}

// 全局实例
window.resourceLoader = new ResourceLoader();

// 加载核心资源
document.addEventListener('DOMContentLoaded', async () => {
    try {
        // 加载CSS资源
        await window.resourceLoader.loadCSS([
            'https://cdn.bootcdn.net/ajax/libs/font-awesome/6.0.0/css/all.min.css',
            'https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css'
        ], 'libs/css/fontawesome/all.min.css');
        
        await window.resourceLoader.loadCSS([
            'https://cdnjs.cloudflare.com/ajax/libs/animate.css/4.1.1/animate.min.css'
        ], 'libs/css/animate/animate.min.css');
        
        // 加载JavaScript资源
        await window.resourceLoader.loadJS([
            'https://cdn.jsdelivr.net/npm/chart.js'
        ], 'libs/js/chart/chart.min.js');
        
        await window.resourceLoader.loadJS([
            'https://cdn.jsdelivr.net/npm/chartjs-plugin-zoom@2.0.1/dist/chartjs-plugin-zoom.min.js'
        ], 'libs/js/chart/chartjs-plugin-zoom.min.js');
        
        await window.resourceLoader.loadJS([
            'https://cdn.jsdelivr.net/npm/xlsx@0.18.5/dist/xlsx.full.min.js'
        ], 'libs/js/xlsx/xlsx.full.min.js');
        
        console.log('所有核心资源加载完成');
        
        // 触发自定义事件表示资源加载完成
        document.dispatchEvent(new CustomEvent('resourcesLoaded'));
        
    } catch (error) {
        console.error('资源加载失败:', error);
        // 显示友好的错误提示
        document.body.insertAdjacentHTML('beforeend', `
            <div style="position: fixed; top: 20px; right: 20px; background: #f56565; color: white; 
                        padding: 16px; border-radius: 8px; z-index: 10000; max-width: 300px;">
                <strong>资源加载失败</strong><br>
                部分功能可能无法正常使用。请检查网络连接或联系管理员。
            </div>
        `);
    }
});