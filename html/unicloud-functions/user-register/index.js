// 用户注册云函数
const crypto = require('crypto');

const db = uniCloud.database();
const usersCollection = db.collection('users');
const verificationCollection = db.collection('email_verification');

// 邮件服务配置（需要根据实际服务商配置）
const emailService = {
    // 这里应该配置真实的邮件服务
    sendVerificationCode: async (email, code) => {
        // 示例：使用阿里云邮件推送服务
        // 需要配置相应的服务密钥和模板
        console.log(`向 ${email} 发送验证码: ${code}`);
        return { success: true };
    }
};

// 生成随机验证码
function generateVerificationCode(length = 6) {
    const chars = '0123456789';
    let result = '';
    for (let i = 0; i < length; i++) {
        result += chars.charAt(Math.floor(Math.random() * chars.length));
    }
    return result;
}

// 密码加密
function hashPassword(password, salt = null) {
    if (!salt) {
        salt = crypto.randomBytes(16).toString('hex');
    }
    const hash = crypto.pbkdf2Sync(password, salt, 1000, 64, 'sha512').toString('hex');
    return { hash, salt };
}

// 验证密码强度
function validatePassword(password) {
    if (password.length < 6) {
        return { valid: false, message: '密码长度至少6位' };
    }
    
    let strength = 0;
    if (password.length >= 8) strength++;
    if (/[A-Z]/.test(password)) strength++;
    if (/[a-z]/.test(password)) strength++;
    if (/\d/.test(password)) strength++;
    if (/[^A-Za-z0-9]/.test(password)) strength++;
    
    return { 
        valid: true, 
        strength: strength,
        strengthText: strength < 2 ? '弱' : strength < 4 ? '中' : '强'
    };
}

// 验证邮箱格式
function validateEmail(email) {
    const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    return emailRegex.test(email);
}

exports.main = async (event, context) => {
    try {
        const { username, email, password } = event;
        
        // 输入验证
        if (!username || !email || !password) {
            return {
                success: false,
                message: '用户名、邮箱和密码不能为空'
            };
        }

        if (username.length < 3) {
            return {
                success: false,
                message: '用户名长度至少3位'
            };
        }

        if (!validateEmail(email)) {
            return {
                success: false,
                message: '邮箱格式不正确'
            };
        }

        const passwordValidation = validatePassword(password);
        if (!passwordValidation.valid) {
            return {
                success: false,
                message: passwordValidation.message
            };
        }

        // 检查用户名是否已存在
        const existingUser = await usersCollection.where({
            username: username
        }).get();

        if (existingUser.data.length > 0) {
            return {
                success: false,
                message: '用户名已被使用'
            };
        }

        // 检查邮箱是否已注册
        const existingEmail = await usersCollection.where({
            email: email
        }).get();

        if (existingEmail.data.length > 0) {
            return {
                success: false,
                message: '该邮箱已被注册'
            };
        }

        // 密码加密
        const { hash, salt } = hashPassword(password);

        // 创建用户记录
        const userData = {
            username: username,
            email: email,
            password_hash: hash,
            password_salt: salt,
            email_verified: false,
            created_at: new Date(),
            updated_at: new Date(),
            status: 'pending_verification'
        };

        const createResult = await usersCollection.add(userData);

        if (createResult.id) {
            return {
                success: true,
                message: '注册成功',
                user_id: createResult.id,
                password_strength: passwordValidation.strengthText
            };
        } else {
            throw new Error('用户创建失败');
        }

    } catch (error) {
        console.error('注册失败:', error);
        return {
            success: false,
            message: '系统错误，请稍后重试',
            error: error.message
        };
    }
}; 