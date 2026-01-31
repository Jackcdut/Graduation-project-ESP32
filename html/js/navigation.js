/**
 * 页面导航和转场动画功能
 */
document.addEventListener('DOMContentLoaded', function() {
    // 初始化页面链接
    initPageTransitions();
    
    // 初始化活跃导航项高亮
    highlightActiveNavItem();
    
    // 初始化回到顶部按钮
    initBackToTopButton();
    
    // 初始化页面加载动画
    initPageLoadingAnimation();
    
    // 添加页面进入动画
    document.body.classList.add('page-loaded');
});

/**
 * 初始化页面间的平滑过渡
 */
function initPageTransitions() {
    // 获取所有内部链接
    const internalLinks = document.querySelectorAll('a[href^="./"], a[href^="/"], a[href^="index"], a[href^="dashboard"], a[href^="analysis"], a[href^="data"], a[href^="signal"], a[href^="statistical"]');
    
    internalLinks.forEach(link => {
        link.addEventListener('click', function(e) {
            // 排除具有特殊属性的链接
            if (this.getAttribute('data-no-transition') || this.getAttribute('target') === '_blank') {
                return;
            }

            // 排除程序化跳转（通过JavaScript触发的跳转）
            if (this.getAttribute('data-programmatic') === 'true') {
                return;
            }

            const href = this.getAttribute('href');
            // 如果是同一页面的锚点链接，则不应用转场效果
            if (href.startsWith('#')) {
                return;
            }

            e.preventDefault();

            // 开始离开页面的动画
            document.body.classList.add('page-leaving');

            // 延迟导航以便动画能够完成
            setTimeout(() => {
                window.location.href = href;
            }, 300);
        });
    });
}

/**
 * 高亮显示当前页面对应的导航项
 */
function highlightActiveNavItem() {
    const currentPath = window.location.pathname;
    const navLinks = document.querySelectorAll('.nav-link');
    
    navLinks.forEach(link => {
        const href = link.getAttribute('href');
        if (!href) return;
        
        // 提取文件名
        const linkPath = href.split('/').pop();
        const currentFile = currentPath.split('/').pop();
        
        if (currentFile === linkPath || 
            (currentFile === '' && linkPath === 'index.html') ||
            (currentPath.includes(linkPath) && linkPath !== 'index.html')) {
            link.parentElement.classList.add('active');
        }
    });
}

/**
 * 添加回到顶部按钮
 */
function initBackToTopButton() {
    // 创建回到顶部按钮
    const backToTopBtn = document.createElement('button');
    backToTopBtn.className = 'back-to-top';
    backToTopBtn.innerHTML = '<span class="iconfont icon-arrow-up"></span>';
    backToTopBtn.title = '回到顶部';
    document.body.appendChild(backToTopBtn);
    
    // 显示/隐藏按钮逻辑
    window.addEventListener('scroll', function() {
        if (window.pageYOffset > 300) {
            backToTopBtn.classList.add('show');
        } else {
            backToTopBtn.classList.remove('show');
        }
    });
    
    // 点击事件
    backToTopBtn.addEventListener('click', function() {
        window.scrollTo({
            top: 0,
            behavior: 'smooth'
        });
    });
}

/**
 * 页面加载动画
 */
function initPageLoadingAnimation() {
    // 检查页面是否已加载
    if (document.readyState === 'complete') {
        hidePageLoader();
    } else {
        window.addEventListener('load', hidePageLoader);
    }
    
    // 隐藏页面加载器
    function hidePageLoader() {
        const pageLoader = document.querySelector('.page-loader');
        if (pageLoader) {
            pageLoader.classList.add('loaded');
            setTimeout(() => {
                pageLoader.style.display = 'none';
            }, 500);
        }
    }
}

/**
 * 创建页面加载器
 */
function createPageLoader() {
    // 检查是否已存在加载器
    if (document.querySelector('.page-loader')) return;
    
    const loader = document.createElement('div');
    loader.className = 'page-loader';
    loader.innerHTML = `
        <div class="loader-content">
            <div class="spinner"></div>
            <p>加载中...</p>
        </div>
    `;
    
    document.body.appendChild(loader);
}

/**
 * 滚动到页面指定部分
 */
function scrollToSection(sectionId, offset = 0) {
    const section = document.getElementById(sectionId);
    if (!section) return;
    
    const targetY = section.getBoundingClientRect().top + window.pageYOffset - offset;
    window.scrollTo({
        top: targetY,
        behavior: 'smooth'
    });
}

/**
 * 添加导航面包屑
 */
function updateBreadcrumbs(items) {
    const breadcrumb = document.querySelector('.breadcrumb');
    if (!breadcrumb) return;
    
    // 清空现有面包屑
    breadcrumb.innerHTML = '';
    
    // 添加首页链接
    const homeLink = document.createElement('a');
    homeLink.href = 'dashboard.html';
    homeLink.textContent = '首页';
    breadcrumb.appendChild(homeLink);
    
    // 添加其他面包屑项目
    items.forEach((item, index) => {
        const separator = document.createElement('span');
        separator.className = 'separator';
        separator.textContent = '/';
        breadcrumb.appendChild(separator);
        
        if (index === items.length - 1) {
            // 最后一项为当前页面
            const current = document.createElement('span');
            current.className = 'current';
            current.textContent = item.text;
            breadcrumb.appendChild(current);
        } else {
            // 其他项为链接
            const link = document.createElement('a');
            link.href = item.url;
            link.textContent = item.text;
            breadcrumb.appendChild(link);
        }
    });
} 