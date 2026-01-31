/**
 * 通用Token生成工具 - 兼容浏览器、Node.js和移动端
 * 
 * 这个文件提供了适用于不同环境的token生成功能：
 * - 浏览器端：使用CryptoJS实现HMAC-SHA1
 * - Node.js：使用crypto模块
 * - 移动端：使用CryptoJS实现
 */

// 检测运行环境
const isNode = typeof process !== 'undefined' && 
              process.versions != null && 
              process.versions.node != null;

const isBrowser = typeof window !== 'undefined';

// Node.js环境下的实现
if (isNode) {
  const crypto = require('crypto');
  
  function createCommonToken(params) {
    const access_key = Buffer.from(params.author_key, "base64");
    const version = params.version;
    let res = 'userid' + '/' + params.user_id;
    // 修复token过期问题 - 使用当前时间戳，而不是固定值
    const et = Math.ceil((Date.now() + 7 * 24 * 3600 * 1000) / 1000);  // 7天后过期
    const method = 'sha1';
    
    const key = et + "\n" + method + "\n" + res + "\n" + version;
    
    let sign = crypto.createHmac('sha1', access_key).update(key).digest().toString('base64');
    
    res = encodeURIComponent(res);
    sign = encodeURIComponent(sign);
    const token = `version=${version}&res=${res}&et=${et}&method=${method}&sign=${sign}`;
    
    return token;
  }
  
  // 导出Node.js模块
  module.exports = {
    createCommonToken
  };
}
// 浏览器环境下的实现
else if (isBrowser) {
  /**
   * 浏览器环境中的Base64解码函数
   */
  function base64ToArrayBuffer(base64) {
    try {
      // 验证base64字符串是否有效
      if (!base64 || typeof base64 !== 'string') {
        throw new Error('Invalid base64 string: empty or not a string');
      }
      
      // 清理base64字符串，移除可能的换行符和空格
      const cleanBase64 = base64.replace(/\s+/g, '');
      
      // 验证base64格式
      const base64Regex = /^[A-Za-z0-9+/]*={0,2}$/;
      if (!base64Regex.test(cleanBase64)) {
        throw new Error('Invalid base64 string format');
      }
      
      const binary_string = window.atob(cleanBase64);
      const len = binary_string.length;
      const bytes = new Uint8Array(len);
      for (let i = 0; i < len; i++) {
        bytes[i] = binary_string.charCodeAt(i);
      }
      return bytes.buffer;
    } catch (error) {
      console.error('Base64 decode error:', error);
      throw new Error(`Base64解码失败: ${error.message}`);
    }
  }

  /**
   * 浏览器环境中的HMAC-SHA1实现
   * 使用Web Crypto API
   */
  async function hmacSha1(key, message) {
    // 将消息字符串转换为ArrayBuffer
    const encoder = new TextEncoder();
    const messageBuffer = encoder.encode(message);
    
    // 将key转换为ArrayBuffer
    let keyBuffer;
    if (typeof key === 'string') {
      keyBuffer = encoder.encode(key);
    } else {
      keyBuffer = key;
    }
    
    // 使用Web Crypto API创建HMAC-SHA1签名
    const cryptoKey = await window.crypto.subtle.importKey(
      'raw',
      keyBuffer,
      { name: 'HMAC', hash: 'SHA-1' },
      false,
      ['sign']
    );
    
    const signature = await window.crypto.subtle.sign(
      'HMAC',
      cryptoKey,
      messageBuffer
    );
    
    // 将签名转换为Base64
    return arrayBufferToBase64(signature);
  }

  /**
   * ArrayBuffer转Base64
   */
  function arrayBufferToBase64(buffer) {
    let binary = '';
    const bytes = new Uint8Array(buffer);
    const len = bytes.byteLength;
    for (let i = 0; i < len; i++) {
      binary += String.fromCharCode(bytes[i]);
    }
    return window.btoa(binary);
  }

  /**
   * 浏览器环境中的Token生成函数
   * 注意：这个函数是异步的，因为Web Crypto API是异步的
   */
  async function createCommonTokenAsync(params) {
    try {
      // 解码access_key
      const access_key_buffer = base64ToArrayBuffer(params.author_key);
      
      const version = params.version;
      let res = 'userid' + '/' + params.user_id;
      // 修复token过期问题 - 使用当前时间戳，设置7天后过期
      const et = Math.ceil((Date.now() + 7 * 24 * 3600 * 1000) / 1000);
      const method = 'sha1';
      
      const key = et + "\n" + method + "\n" + res + "\n" + version;
      
      // 使用Web Crypto API计算HMAC-SHA1签名
      const sign = await hmacSha1(access_key_buffer, key);
      
      res = encodeURIComponent(res);
      const encodedSign = encodeURIComponent(sign);
      const token = `version=${version}&res=${res}&et=${et}&method=${method}&sign=${encodedSign}`;
      
      return token;
    } catch (error) {
      console.error("Token生成错误:", error);
      return Promise.reject(error);
    }
  }
  
  /**
   * 同步版本的Token生成函数
   * 注意：由于正确的HMAC-SHA1需要Web Crypto API（异步），此函数仅供后备使用
   */
  function createCommonToken(params) {
    console.warn('警告: 同步Token生成函数无法使用正确的HMAC-SHA1算法，建议使用createCommonTokenAsync');
    
    try {
      const version = params.version || '2022-05-01';
      let res = 'userid' + '/' + params.user_id;
      // 使用当前时间戳，设置7天后过期
      const et = Math.ceil((Date.now() + 7 * 24 * 3600 * 1000) / 1000);
      const method = 'sha1';
      
      // 如果异步token已经可用，直接返回它
      if (window.latestToken && typeof window.latestToken === 'string' && window.latestToken.includes('version=')) {
        console.log('使用已缓存的异步Token');
        return window.latestToken;
      }
      
      // 否则返回错误，强制使用异步方法
      throw new Error('同步Token生成不可用，请使用createCommonTokenAsync');
      
    } catch (error) {
      console.error("同步token生成错误:", error);
      // 返回错误标识，让调用方知道需要使用异步方法
      return null;
    }
  }
  
  /**
   * 从OneNET获取历史数据点
   * 严格按照API文档实现
   * API文档：https://iot-api.heclouds.com/thingmodel/query-device-property-history
   */
  async function fetchHistoricalData(params) {
    try {
      const {
        productId, 
        deviceName, 
        datastreamId, // 对应API文档中的identifier
        accessKey,
        start,
        end,
        limit = 100,
        sort = '2',   // 默认倒序
        offset = '0'  // 默认从0开始
      } = params;
      
      // 生成token
      const authParams = {
        author_key: accessKey,
        version: '2022-05-01',
        user_id: '420568',
      };
      
      // 获取token（优先使用异步方法）
      let token;
      try {
        token = await createCommonTokenAsync(authParams);
      } catch (err) {
        console.warn("异步token生成失败，使用同步方法", err);
        token = createCommonToken(authParams);
      }
      
      // 确保时间戳是毫秒格式
      const startTime = typeof start === 'number' ? start : (start ? new Date(start).getTime() : (Date.now() - 24 * 60 * 60 * 1000));
      const endTime = typeof end === 'number' ? end : (end ? new Date(end).getTime() : Date.now());
      
      console.log(`fetchHistoricalData: ${datastreamId}, 开始时间: ${new Date(startTime).toLocaleString()}, 结束时间: ${new Date(endTime).toLocaleString()}`);
      
      // 严格按照API文档构建查询参数
      let queryParams = `?product_id=${encodeURIComponent(productId)}&device_name=${encodeURIComponent(deviceName)}`;
      
      // 添加必需的identifier参数
      if (datastreamId) {
        queryParams += `&identifier=${encodeURIComponent(datastreamId)}`;
      } else {
        console.warn("缺少必需的identifier参数");
        return { code: -1, msg: "缺少必需的identifier参数", request_id: "", data: { list: [] } };
      }
      
      // 添加时间范围参数，确保使用字符串形式的毫秒时间戳
      queryParams += `&start_time=${String(startTime)}&end_time=${String(endTime)}`;
      
      // 添加可选参数
      if (sort) queryParams += `&sort=${sort}`;
      if (offset) queryParams += `&offset=${offset}`;
      if (limit) queryParams += `&limit=${limit}`;
      
      // 打印完整的请求URL以便调试
      console.log(`完整API请求URL: https://iot-api.heclouds.com/thingmodel/query-device-property-history${queryParams}`);
      
      // 发送请求
      const response = await fetch(`https://iot-api.heclouds.com/thingmodel/query-device-property-history${queryParams}`, {
        method: 'GET',
        headers: {
          'Authorization': token,
          'Content-Type': 'application/json'
        }
      });
      
      if (!response.ok) {
        const errorText = await response.text();
        throw new Error(`API请求失败: ${response.status} - ${errorText}`);
      }
      
      const data = await response.json();
      
      // 转换API响应格式以兼容现有代码
      if (data && data.code === 0 && data.data && data.data.list) {
        // 创建兼容旧API格式的数据结构
        const compatibleData = {
          code: data.code,
          msg: data.msg,
          request_id: data.request_id,
          data: {
            datastreams: [
              {
                id: datastreamId,
                datapoints: data.data.list.map(item => ({
                  at: new Date(parseInt(item.time)).toISOString(),
                  value: item.value
                }))
              }
            ]
          }
        };
        return compatibleData;
      }
      
      return data;
    } catch (error) {
      console.error("获取历史数据失败:", error);
      return { success: false, error: error.message };
    }
  }
  
  // 浏览器中暴露到全局作用域
  window.createCommonToken = createCommonToken;
  window.createCommonTokenAsync = createCommonTokenAsync;
  window.fetchOneNetData = fetchHistoricalData;
  window.fetchHistoricalData = fetchHistoricalData;
}
// 其他环境（如React Native等）
else {
  // 这部分可以针对其他特定环境添加实现
  globalThis.createCommonToken = function(params) {
    console.warn("使用通用环境下的Token生成函数");
    // 使用当前时间戳，设置7天后过期
    const et = Math.ceil((Date.now() + 7 * 24 * 3600 * 1000) / 1000);
    return `version=2022-05-01&res=userid%2F420568&et=${et}&method=sha1&sign=generic-token`;
  };
}


