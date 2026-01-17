// 发送邮箱验证码云函数
const crypto = require('crypto');

const db = uniCloud.database();
const verificationCollection = db.collection('email_verification');

// 生成验证码
function generateVerificationCode(length = 6) {
    const chars = '0123456789';
    let result = '';
    for (let i = 0; i < length; i++) {
        result += chars.charAt(Math.floor(Math.random() * chars.length));
    }
    return result;
}

// 邮件模板
const emailTemplates = {
    register: {
        subject: '【数据分析平台】邮箱验证码',
        template: (code) => `
            <div style="max-width: 600px; margin: 0 auto; padding: 20px; font-family: 'Helvetica Neue', sans-serif;">
                <div style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); padding: 30px; text-align: center; border-radius: 10px 10px 0 0;">
                    <h1 style="color: white; margin: 0; font-size: 24px;">数据分析平台</h1>
                    <p style="color: rgba(255,255,255,0.9); margin: 10px 0 0 0;">专业的信号处理与数据分析工具</p>
                </div>
                <div style="background: white; padding: 40px; border-radius: 0 0 10px 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1);">
                    <h2 style="color: #333; margin-bottom: 20px;">验证您的邮箱地址</h2>
                    <p style="color: #666; line-height: 1.6; margin-bottom: 30px;">
                        感谢您注册数据分析平台！请使用以下验证码完成邮箱验证：
                    </p>
                    <div style="background: #f8f9fa; border: 2px dashed #667eea; padding: 20px; text-align: center; border-radius: 8px; margin: 30px 0;">
                        <span style="font-size: 32px; font-weight: bold; color: #667eea; letter-spacing: 6px;">${code}</span>
                    </div>
                    <p style="color: #999; font-size: 14px; margin-top: 30px;">
                        验证码有效期为10分钟，请尽快使用。如果您没有申请注册，请忽略此邮件。
                    </p>
                </div>
                <div style="text-align: center; padding: 20px; color: #999; font-size: 12px;">
                    <p>© 2025 数据分析平台 | 专业的信号处理与数据分析工具</p>
                </div>
            </div>
        `
    },
    reset: {
        subject: '【数据分析平台】密码重置验证码',
        template: (code) => `
            <div style="max-width: 600px; margin: 0 auto; padding: 20px; font-family: 'Helvetica Neue', sans-serif;">
                <div style="background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); padding: 30px; text-align: center; border-radius: 10px 10px 0 0;">
                    <h1 style="color: white; margin: 0; font-size: 24px;">数据分析平台</h1>
                    <p style="color: rgba(255,255,255,0.9); margin: 10px 0 0 0;">密码重置请求</p>
                </div>
                <div style="background: white; padding: 40px; border-radius: 0 0 10px 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1);">
                    <h2 style="color: #333; margin-bottom: 20px;">重置您的密码</h2>
                    <p style="color: #666; line-height: 1.6; margin-bottom: 30px;">
                        我们收到了您的密码重置请求。请使用以下验证码重置您的密码：
                    </p>
                    <div style="background: #fff3cd; border: 2px dashed #f0ad4e; padding: 20px; text-align: center; border-radius: 8px; margin: 30px 0;">
                        <span style="font-size: 32px; font-weight: bold; color: #f0ad4e; letter-spacing: 6px;">${code}</span>
                    </div>
                    <p style="color: #999; font-size: 14px; margin-top: 30px;">
                        验证码有效期为10分钟。如果您没有申请密码重置，请忽略此邮件，您的账户仍然安全。
                    </p>
                </div>
                <div style="text-align: center; padding: 20px; color: #999; font-size: 12px;">
                    <p>© 2025 数据分析平台 | 专业的信号处理与数据分析工具</p>
                </div>
            </div>
        `
    }
};

// 发送邮件函数（需要配置实际的邮件服务）
async function sendEmail(to, subject, htmlContent) {
    // 这里应该集成真实的邮件服务，例如：
    // 1. 阿里云邮件推送服务
    // 2. 腾讯云邮件服务
    // 3. SendGrid
    // 4. 其他邮件服务商
    
    try {
        // 示例：使用阿里云邮件推送
        // const result = await alicloudEmailService.send({
        //     to: to,
        //     subject: subject,
        //     html: htmlContent
        // });
        
        // 目前为演示用途，仅记录日志
        console.log(`发送邮件到 ${to}`);
        console.log(`主题: ${subject}`);
        console.log('内容:', htmlContent);
        
        return { success: true };
    } catch (error) {
        console.error('邮件发送失败:', error);
        throw error;
    }
}

exports.main = async (event, context) => {
    try {
        const { email, type = 'register' } = event;
        
        // 验证邮箱格式
        const emailRegex = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
        if (!emailRegex.test(email)) {
            return {
                success: false,
                message: '邮箱格式不正确'
            };
        }

        // 检查发送频率限制（防止滥用）
        const now = new Date();
        const oneMinuteAgo = new Date(now.getTime() - 60 * 1000);
        
        const recentSends = await verificationCollection
            .where({
                email: email,
                created_at: db.command.gte(oneMinuteAgo)
            })
            .get();

        if (recentSends.data.length > 0) {
            return {
                success: false,
                message: '发送过于频繁，请1分钟后再试'
            };
        }

        // 生成验证码
        const code = generateVerificationCode(6);
        const expiresAt = new Date(now.getTime() + 10 * 60 * 1000); // 10分钟后过期

        // 保存验证码记录
        const verificationData = {
            email: email,
            code: code,
            type: type,
            used: false,
            created_at: now,
            expires_at: expiresAt,
            ip: context.CLIENTIP || 'unknown'
        };

        await verificationCollection.add(verificationData);

        // 发送邮件
        const template = emailTemplates[type] || emailTemplates.register;
        const htmlContent = template.template(code);
        
        await sendEmail(email, template.subject, htmlContent);

        return {
            success: true,
            message: '验证码发送成功',
            expires_in: 600 // 10分钟
        };

    } catch (error) {
        console.error('发送验证码失败:', error);
        return {
            success: false,
            message: '发送失败，请稍后重试',
            error: error.message
        };
    }
}; 