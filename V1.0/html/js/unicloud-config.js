// UniCloud 配置文件
class UniCloudService {
    constructor() {
        this.config = {
            spaceId: 'mp-f7bbae3b-f356-4047-8276-372de80efe6f',
            spaceName: 'flotation-monitor',
            clientSecret: '4X8roJ2KpEPoDuiD4+Bgfw==',
            requestDomain: 'https://api.next.bspapp.com',
            uploadDomain: 'https://file-unidynyous-mp-f7bbae3b-f356-4047-8276-372de80efe6f.oss-cn-zhangjiakou.aliyuncs.com',
            downloadDomain: 'https://mp-f7bbae3b-f356-4047-8276-372de80efe6f.cdn.bspapp.com'
        };
        
        this.isInitialized = false;
        this.init();
    }

    async init() {
        try {
            // 初始化云服务
            this.cloud = new uniCloud.Uniapp({
                spaceId: this.config.spaceId,
                clientSecret: this.config.clientSecret,
                endpoint: this.config.requestDomain
            });
            
            this.isInitialized = true;
            console.log('✓ UniCloud 初始化成功');
            return true;
        } catch (error) {
            console.error('✗ UniCloud 初始化失败:', error);
            this.isInitialized = false;
            return false;
        }
    }

    // 调用云函数
    async callFunction(name, data = {}) {
        if (!this.isInitialized) {
            throw new Error('UniCloud 未初始化');
        }

        try {
            console.log(`🚀 调用云函数: ${name}`, data);
            const result = await this.cloud.callFunction({
                name,
                data
            });
            
            console.log(`✓ 云函数 ${name} 调用成功:`, result);
            return result;
        } catch (error) {
            console.error(`✗ 云函数 ${name} 调用失败:`, error);
            throw error;
        }
    }

    // 数据库操作
    database() {
        if (!this.isInitialized) {
            throw new Error('UniCloud 未初始化');
        }
        return this.cloud.database();
    }

    // 用户认证相关API
    async register(userData) {
        return await this.callFunction('user-register', userData);
    }

    async login(credentials) {
        return await this.callFunction('user-login', credentials);
    }

    async resetPassword(email) {
        return await this.callFunction('password-reset', { email });
    }

    async sendVerificationCode(email, type = 'register') {
        return await this.callFunction('send-verification-code', { email, type });
    }

    async verifyEmail(email, code) {
        return await this.callFunction('verify-email', { email, code });
    }

    // 获取当前用户信息
    async getCurrentUser() {
        const token = localStorage.getItem('unicloud_token');
        if (!token) {
            return null;
        }

        try {
            return await this.callFunction('get-current-user', { token });
        } catch (error) {
            // Token可能过期，清除本地存储
            localStorage.removeItem('unicloud_token');
            localStorage.removeItem('currentUser');
            return null;
        }
    }

    // 退出登录
    async logout() {
        const token = localStorage.getItem('unicloud_token');
        if (token) {
            try {
                await this.callFunction('user-logout', { token });
            } catch (error) {
                console.warn('云端退出失败:', error);
            }
        }
        
        // 清除本地存储
        localStorage.removeItem('unicloud_token');
        localStorage.removeItem('currentUser');
        localStorage.removeItem('loginTime');
    }

    // 检查邮箱是否已注册
    async checkEmailExists(email) {
        return await this.callFunction('check-email-exists', { email });
    }

    // 检查用户名是否已存在
    async checkUsernameExists(username) {
        return await this.callFunction('check-username-exists', { username });
    }
}

// 创建全局实例
window.uniCloudService = new UniCloudService();

// 导出供其他模块使用
if (typeof module !== 'undefined' && module.exports) {
    module.exports = UniCloudService;
} 