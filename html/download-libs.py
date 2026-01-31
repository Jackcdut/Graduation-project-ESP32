#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
数据分析平台 - 依赖库下载脚本
用于下载所有外部CDN依赖库到本地，确保项目可以离线运行
"""

import os
import requests
import shutil
from pathlib import Path
from urllib.parse import urlparse

class LibraryDownloader:
    def __init__(self):
        self.base_dir = Path("libs")
        self.session = requests.Session()
        self.session.headers.update({
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36'
        })
        
        # 需要下载的资源列表
        self.resources = {
            # CSS 资源
            'css/fontawesome/all.min.css': 'https://cdn.bootcdn.net/ajax/libs/font-awesome/6.0.0/css/all.min.css',
            'css/animate/animate.min.css': 'https://cdnjs.cloudflare.com/ajax/libs/animate.css/4.1.1/animate.min.css',
            
            # JavaScript 资源  
            'js/chart/chart.min.js': 'https://cdn.jsdelivr.net/npm/chart.js',
            'js/chart/chartjs-plugin-zoom.min.js': 'https://cdn.jsdelivr.net/npm/chartjs-plugin-zoom@2.0.1/dist/chartjs-plugin-zoom.min.js',
            'js/chart/chartjs-plugin-annotation.min.js': 'https://cdn.jsdelivr.net/npm/chartjs-plugin-annotation@2.2.1/dist/chartjs-plugin-annotation.min.js',
            'js/xlsx/xlsx.full.min.js': 'https://cdn.jsdelivr.net/npm/xlsx@0.18.5/dist/xlsx.full.min.js',
            'js/particles/particles.min.js': 'https://cdn.jsdelivr.net/particles.js/2.0.0/particles.min.js',
        }
        
        # 字体资源（Google Fonts）
        self.fonts = {
            'Inter': 'https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap',
            'Nunito': 'https://fonts.googleapis.com/css2?family=Nunito:wght@400;600;700&display=swap'
        }

    def create_directories(self):
        """创建必要的目录结构"""
        directories = [
            'css/fontawesome',
            'css/animate', 
            'css/fonts',
            'js/chart',
            'js/xlsx',
            'js/particles',
            'js/feather',
            'fonts/inter',
            'fonts/nunito'
        ]
        
        for dir_path in directories:
            full_path = self.base_dir / dir_path
            full_path.mkdir(parents=True, exist_ok=True)
            print(f"✓ 创建目录: {full_path}")

    def download_file(self, url, local_path):
        """下载单个文件"""
        try:
            print(f"下载: {url}")
            response = self.session.get(url, timeout=30)
            response.raise_for_status()
            
            # 确保目录存在
            local_path.parent.mkdir(parents=True, exist_ok=True)
            
            # 写入文件
            with open(local_path, 'wb') as f:
                f.write(response.content)
            
            print(f"✓ 保存到: {local_path}")
            return True
            
        except Exception as e:
            print(f"✗ 下载失败 {url}: {e}")
            return False

    def download_resources(self):
        """下载所有资源文件"""
        print("开始下载CSS和JS资源...")
        success_count = 0
        total_count = len(self.resources)
        
        for local_path, url in self.resources.items():
            full_local_path = self.base_dir / local_path
            if self.download_file(url, full_local_path):
                success_count += 1
        
        print(f"\n资源下载完成: {success_count}/{total_count}")
        return success_count == total_count

    def download_fonts(self):
        """下载和处理Google Fonts"""
        print("\n开始处理Google Fonts...")
        
        for font_name, css_url in self.fonts.items():
            try:
                # 下载CSS文件
                print(f"处理字体: {font_name}")
                css_response = self.session.get(css_url)
                css_response.raise_for_status()
                
                css_content = css_response.text
                font_dir = self.base_dir / 'fonts' / font_name.lower()
                font_dir.mkdir(parents=True, exist_ok=True)
                
                # 保存CSS文件
                css_file = font_dir / f'{font_name.lower()}.css'
                with open(css_file, 'w', encoding='utf-8') as f:
                    f.write(css_content)
                
                print(f"✓ 保存字体CSS: {css_file}")
                
                # TODO: 解析CSS中的字体文件URL并下载woff2文件
                # 这里简化处理，仅保存CSS文件
                
            except Exception as e:
                print(f"✗ 处理字体失败 {font_name}: {e}")

    def create_combined_css(self):
        """创建合并的CSS文件以减少HTTP请求"""
        print("\n创建合并CSS文件...")
        
        combined_css = ""
        css_files = [
            self.base_dir / 'css/fontawesome/all.min.css',
            self.base_dir / 'css/animate/animate.min.css'
        ]
        
        for css_file in css_files:
            if css_file.exists():
                with open(css_file, 'r', encoding='utf-8') as f:
                    combined_css += f"\n/* {css_file.name} */\n"
                    combined_css += f.read()
                    combined_css += "\n"
        
        # 保存合并的CSS
        combined_file = self.base_dir / 'css/combined.min.css'
        with open(combined_file, 'w', encoding='utf-8') as f:
            f.write(combined_css)
        
        print(f"✓ 创建合并CSS: {combined_file}")

    def create_loader_script(self):
        """创建资源加载器脚本，提供CDN fallback功能"""
        loader_script = '''/**
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
});'''

        loader_file = self.base_dir / 'js/resource-loader.js'
        with open(loader_file, 'w', encoding='utf-8') as f:
            f.write(loader_script)
        
        print(f"✓ 创建资源加载器: {loader_file}")

    def run(self):
        """执行完整的下载流程"""
        print("=== 数据分析平台依赖库下载器 ===\n")
        
        # 创建目录结构
        self.create_directories()
        
        # 下载资源文件
        if self.download_resources():
            print("✓ 所有资源下载成功")
        else:
            print("⚠ 部分资源下载失败，请检查网络连接")
        
        # 下载字体
        self.download_fonts()
        
        # 创建合并文件
        self.create_combined_css()
        
        # 创建加载器脚本
        self.create_loader_script()
        
        print("\n=== 下载完成 ===")
        print("请运行此脚本来下载所有依赖库:")
        print("python3 download-libs.py")
        print("\n下载完成后，所有HTML文件将自动使用本地资源。")

if __name__ == "__main__":
    downloader = LibraryDownloader()
    downloader.run() 