const http = require('http');

// 阿里云函数计算事件函数入口
exports.handler = (req, resp, context) => {
    // 设置 CORS 头
    resp.setHeader('Access-Control-Allow-Origin', '*');
    resp.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    resp.setHeader('Access-Control-Allow-Headers', 'Content-Type');
    resp.setHeader('Content-Type', 'application/json');

    // 处理 OPTIONS 预检请求
    if (req.method === 'OPTIONS') {
        resp.setStatusCode(200);
        resp.send('');
        return;
    }

    if (req.method !== 'POST') {
        resp.setStatusCode(200);
        resp.send(JSON.stringify({ errno: -1, error: 'Method not allowed' }));
        return;
    }

    // 获取请求体
    let body = '';
    if (req.body) {
        body = req.body.toString();
    }

    try {
        const { pid, authInfo } = JSON.parse(body);

        if (!pid || !authInfo) {
            resp.setStatusCode(200);
            resp.send(JSON.stringify({ errno: -1, error: '缺少参数' }));
            return;
        }

        // 调用 OneNet API
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

        const apiReq = http.request(options, (apiRes) => {
            let data = '';
            apiRes.on('data', chunk => { data += chunk; });
            apiRes.on('end', () => {
                resp.setStatusCode(200);
                resp.send(data);
            });
        });

        apiReq.on('error', (e) => {
            resp.setStatusCode(200);
            resp.send(JSON.stringify({ errno: -1, error: e.message }));
        });

        apiReq.setTimeout(10000, () => {
            apiReq.destroy();
            resp.setStatusCode(200);
            resp.send(JSON.stringify({ errno: -1, error: '请求超时' }));
        });

        apiReq.write(postData);
        apiReq.end();

    } catch (e) {
        resp.setStatusCode(200);
        resp.send(JSON.stringify({ errno: -1, error: e.message }));
    }
};
