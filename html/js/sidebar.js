/**
 * 统一侧边栏交互脚本
 * 用于处理所有页面中的侧边栏交互行为
 */

document.addEventListener('DOMContentLoaded', function() {
    // 初始化侧边栏展开/折叠
    initSidebarToggle();
    
    // 初始化子菜单展开/折叠
    initSubmenuToggle();
    
    // 高亮当前页面对应的菜单项
    highlightCurrentPage();
    
    // 为移动设备添加侧边栏遮罩层点击关闭
    initMobileOverlay();
    
    // 根据URL自动展开相关子菜单
    expandSubmenuBasedOnURL();
});

/**
 * 初始化侧边栏展开/折叠功能
 */
function initSidebarToggle() {
    const menuToggle = document.getElementById('menuToggle');
    const sidebar = document.getElementById('sidebar');
    const mainContainer = document.querySelector('.main-container');
    const sidebarOverlay = document.getElementById('sidebarOverlay');
    
    if (menuToggle && sidebar) {
        menuToggle.addEventListener('click', function() {
            // 在移动端，添加/移除active类来显示/隐藏侧边栏
            if (window.innerWidth <= 992) {
                sidebar.classList.toggle('active');
                if (sidebarOverlay) {
                    sidebarOverlay.classList.toggle('active');
                }
                document.body.classList.toggle('sidebar-open');
            } 
            // 在桌面端，添加/移除collapsed类来展开/折叠侧边栏
            else {
                sidebar.classList.toggle('collapsed');
                if (mainContainer) {
                    mainContainer.classList.toggle('sidebar-collapsed');
                }
                // 保存用户的偏好设置
                localStorage.setItem('sidebarCollapsed', sidebar.classList.contains('collapsed'));
            }
        });
        
        // 加载保存的侧边栏状态
        if (window.innerWidth > 992) {
            const isCollapsed = localStorage.getItem('sidebarCollapsed') === 'true';
            if (isCollapsed) {
                sidebar.classList.add('collapsed');
                if (mainContainer) {
                    mainContainer.classList.add('sidebar-collapsed');
                }
            }
        }
    }
}

/**
 * 初始化子菜单展开/折叠功能
 */
function initSubmenuToggle() {
    const navItems = document.querySelectorAll('.nav-item:has(.nav-submenu)');
    
    navItems.forEach(item => {
        const link = item.querySelector('.nav-link');
        if (link) {
            link.addEventListener('click', function(e) {
                // 阻止链接默认行为
                if (this.getAttribute('href') === '#' || this.closest('.nav-item').querySelector('.nav-submenu')) {
                    e.preventDefault();
                }
                
                // 切换当前项的展开状态
                const navItem = this.closest('.nav-item');
                navItem.classList.toggle('expanded');
                
                // 保存子菜单状态
                const menuId = navItem.dataset.menuId || navItem.querySelector('.nav-text').textContent.trim();
                localStorage.setItem(`submenu_${menuId}`, navItem.classList.contains('expanded'));
            });
        }
    });
}

/**
 * 高亮当前页面对应的菜单项
 */
function highlightCurrentPage() {
    const currentPath = window.location.pathname;
    const currentPage = currentPath.split('/').pop();
    
    // 高亮主菜单项
    const navLinks = document.querySelectorAll('.nav-link');
    
    navLinks.forEach(link => {
        const href = link.getAttribute('href');
        if (!href) return;
        
        const linkPage = href.split('/').pop();
        
        if (currentPage === linkPage) {
            // 将active类添加到最近的nav-item
            const navItem = link.closest('.nav-item');
            if (navItem) {
                navItem.classList.add('active');
                
                // 如果是子菜单项，确保父菜单展开
                const parentItem = navItem.closest('.nav-submenu')?.closest('.nav-item');
                if (parentItem) {
                    parentItem.classList.add('expanded');
                }
            }
        }
    });
    
    // 高亮子菜单项
    const submenuLinks = document.querySelectorAll('.nav-submenu-link');
    
    submenuLinks.forEach(link => {
        const href = link.getAttribute('href');
        if (!href) return;
        
        const linkPage = href.split('/').pop();
        
        if (currentPage === linkPage) {
            // 将active类添加到最近的nav-submenu-item
            const submenuItem = link.closest('.nav-submenu-item');
            if (submenuItem) {
                submenuItem.classList.add('active');
                
                // 确保父菜单展开
                const parentItem = submenuItem.closest('.nav-submenu').closest('.nav-item');
                if (parentItem) {
                    parentItem.classList.add('expanded');
                }
            }
        }
    });
}

/**
 * 为移动设备添加侧边栏遮罩层点击关闭
 */
function initMobileOverlay() {
    const overlay = document.getElementById('sidebarOverlay');
    const sidebar = document.getElementById('sidebar');
    
    if (overlay && sidebar) {
        overlay.addEventListener('click', function() {
            sidebar.classList.remove('active');
            overlay.classList.remove('active');
            document.body.classList.remove('sidebar-open');
        });
    }
    
    // 监听窗口大小变化
    window.addEventListener('resize', function() {
        if (window.innerWidth > 992) {
            // 恢复桌面端状态
            if (overlay) overlay.classList.remove('active');
            document.body.classList.remove('sidebar-open');
            
            const isCollapsed = localStorage.getItem('sidebarCollapsed') === 'true';
            if (sidebar) {
                sidebar.classList.remove('active');
                if (isCollapsed) {
                    sidebar.classList.add('collapsed');
                }
            }
        } else {
            // 切换到移动端状态
            if (sidebar) sidebar.classList.remove('collapsed');
        }
    });
}

/**
 * 根据URL自动展开相关子菜单
 */
function expandSubmenuBasedOnURL() {
    const currentPath = window.location.pathname;
    const currentPage = currentPath.split('/').pop();
    
    // 检查子菜单链接
    const submenuLinks = document.querySelectorAll('.nav-submenu-link');
    
    submenuLinks.forEach(link => {
        const href = link.getAttribute('href');
        if (!href) return;
        
        const linkPage = href.split('/').pop();
        
        if (currentPage === linkPage) {
            // 找到父菜单项并展开
            const parentItem = link.closest('.nav-submenu').closest('.nav-item');
            if (parentItem) {
                parentItem.classList.add('expanded');
            }
        }
    });
} 