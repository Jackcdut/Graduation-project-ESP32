'use strict';

/**
 * OneNet设备验证云函数
 * 代理调用 OneNet OTA API 验证设备
 * API: http://ota.heclouds.com/ota/devInfo
 */

const https = require('https');
const http = require('http');

exports.main = async (event, context) => {
    const { pid, authInfo } = event;
    
    // 参数验证
    if (!pid || !authInfo) {
        return {
            errno: -1,
            error: '缺少必要参数: pid 或 authInfo',
            data: null
        };
    }
    
    // 固定产品ID验证
    if (pid !== 'FCwDzD6VU0') {
        return {
            errno: -2,
            error: '产品ID无效',
            data: null
        };
    }
    
    try {
        const result = await callOneNetAPI(pid, authInfo);
        return result;
    } catch (error) {
        console.error('OneNet API调用失败:', error);
        return {
            errno: -3,
            error: '网络请求失败: ' + error.message,
            data: null
        };
    }
};

function callOneNetAPI(pid, authInfo) {
    return new Promise((resolve, reject) => {
        const postData = JSON.stringify({ pid, authInfo });
        
        const options = {
            hostname: 'ota.heclouds.com',
            port: 80,
            path: '/ota/devInfo',
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Content-Length': Buffer.byteLength(postData)
            }
        };
        
        const req = http.request(options, (res) => {
            let data = '';
            res.on('data', chunk => data += chunk);
            res.on('end', () => {
                try {
                    resolve(JSON.parse(data));
                } catch (e) {
                    reject(new Error('响应解析失败'));
                }
            });
        });
        
        req.on('error', reject);
        req.write(postData);
        req.end();
    });
}
