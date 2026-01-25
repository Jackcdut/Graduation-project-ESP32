# 本地依赖库目录

本目录包含项目所需的所有第三方库文件，用于替代CDN依赖，确保项目在unicloud等云服务环境中稳定运行。

## 目录结构

```
libs/
├── css/
│   ├── fontawesome/          # Font Awesome 图标库
│   ├── animate/               # Animate.css 动画库
│   └── fonts/                 # Google Fonts 字体文件
├── js/
│   ├── chart/                 # Chart.js 及相关插件
│   ├── xlsx/                  # SheetJS Excel处理库
│   ├── particles/             # Particles.js 粒子效果
│   └── feather/               # Feather Icons 图标库
└── fonts/                     # 字体文件
    ├── inter/                 # Inter 字体系列
    └── nunito/                # Nunito 字体系列
```

## 版本信息

- Font Awesome: 6.0.0
- Chart.js: latest stable
- Chart.js Zoom Plugin: 2.0.1
- Chart.js Annotation Plugin: 2.2.1
- SheetJS: 0.18.5
- Animate.css: 4.1.1
- Particles.js: 2.0.0
- Feather Icons: latest

## 使用说明

所有HTML文件中的CDN引用已替换为本地路径引用，并实现了CDN失败时的本地fallback机制。 