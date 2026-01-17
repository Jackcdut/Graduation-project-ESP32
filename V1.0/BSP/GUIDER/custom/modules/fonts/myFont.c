#include "esp_partition.h"
#include "lvgl.h"
#include "esp_log.h"

#define TAG "myFont.c"

typedef struct{
    uint16_t min;
    uint16_t max;
    uint8_t  bpp;
    uint8_t  reserved[3];
}x_header_t;

typedef struct{
    uint32_t pos;
}x_table_t;

typedef struct{
    uint8_t adv_w;
    uint8_t box_w;
    uint8_t box_h;
    int8_t  ofs_x;
    int8_t  ofs_y;
    uint8_t r;
}glyph_dsc_t;


static x_header_t __g_xbf_hd = {
    .min = 0x0020,
    .max = 0x9fa0,
    .bpp = 2,
};

static uint8_t __g_font_buf[144];//如bin文件存在SPI FLASH可使用此buff

static esp_partition_t* partition_res=NULL;

// 前向声明
static const uint8_t * __user_font_get_bitmap(const lv_font_t * font, uint32_t unicode_letter);
static bool __user_font_get_glyph_dsc(const lv_font_t * font, lv_font_glyph_dsc_t * dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next);

static uint8_t *__user_font_getdata(int offset, int size){
    //如字模保存在SPI FLASH, SPIFLASH_Read(__g_font_buf,offset,size);
    //如字模已加载到SDRAM,直接返回偏移地址即可如:return (uint8_t*)(sdram_fontddr+offset);
    static uint8_t first_in = 1;
    if(first_in==1)
    {
        partition_res=esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, NULL);//这个函数第一个参数是我们分区表的第四行的，第二列的参数，第二个是第三列的值
        first_in=0;
        if (partition_res == NULL)
        {
            ESP_LOGI(TAG,"[WARNING] Font partition not found! Please flash myFont.bin to partition.\n");
            return NULL;
        }else{
             ESP_LOGI(TAG,"Successfully found font partition\n");

             // 读取并验证字库文件头部
             x_header_t header;
             esp_err_t res = esp_partition_read(partition_res, 0, &header, sizeof(x_header_t));
             if (res == ESP_OK) {
                 ESP_LOGI(TAG,"Font header info:");
                 ESP_LOGI(TAG,"  min: 0x%04X", header.min);
                 ESP_LOGI(TAG,"  max: 0x%04X", header.max);
                 ESP_LOGI(TAG,"  bpp: %d", header.bpp);

                 // 更新全局头部信息
                 __g_xbf_hd.min = header.min;
                 __g_xbf_hd.max = header.max;
                 __g_xbf_hd.bpp = header.bpp;
             } else {
                 ESP_LOGI(TAG,"[ERROR] Failed to read font header from partition");
             }
        }
    }

    // 如果分区未找到，直接返回NULL，避免崩溃
    if (partition_res == NULL) {
        return NULL;
    }

    esp_err_t res=esp_partition_read(partition_res,offset,__g_font_buf,size);//读取数据
    if(res!=ESP_OK)
    {
        ESP_LOGI(TAG,"Failed to read font data from partition (offset=%d, size=%d)\n", offset, size);
        return NULL;
    }
    return __g_font_buf;
}

static const uint8_t * __user_font_get_bitmap(const lv_font_t * font, uint32_t unicode_letter) {
    if( unicode_letter>__g_xbf_hd.max || unicode_letter<__g_xbf_hd.min ) {
        ESP_LOGI(TAG, "Character 0x%04X out of range [0x%04X-0x%04X]", unicode_letter, __g_xbf_hd.min, __g_xbf_hd.max);
        return NULL;
    }
    uint32_t unicode_offset = sizeof(x_header_t)+(unicode_letter-__g_xbf_hd.min)*4;
    uint32_t *p_pos = (uint32_t *)__user_font_getdata(unicode_offset, 4);
    if (p_pos == NULL) {
        ESP_LOGI(TAG, "Failed to read position for character 0x%04X", unicode_letter);
        return NULL;
    }
    if( p_pos[0] != 0 ) {
        uint32_t pos = p_pos[0];
        glyph_dsc_t * gdsc = (glyph_dsc_t*)__user_font_getdata(pos, sizeof(glyph_dsc_t));
        if (gdsc == NULL) {
            ESP_LOGI(TAG, "Failed to read glyph descriptor for character 0x%04X", unicode_letter);
            return NULL;
        }
        uint32_t bitmap_size = gdsc->box_w*gdsc->box_h*__g_xbf_hd.bpp/8;
        return __user_font_getdata(pos+sizeof(glyph_dsc_t), bitmap_size);
    }
    ESP_LOGI(TAG, "Character 0x%04X has no bitmap data (pos=0)", unicode_letter);
    return NULL;
}

static bool __user_font_get_glyph_dsc(const lv_font_t * font, lv_font_glyph_dsc_t * dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next) {
    if( unicode_letter>__g_xbf_hd.max || unicode_letter<__g_xbf_hd.min ) {
        return false;
    }
    uint32_t unicode_offset = sizeof(x_header_t)+(unicode_letter-__g_xbf_hd.min)*4;
    uint32_t *p_pos = (uint32_t *)__user_font_getdata(unicode_offset, 4);
    if( p_pos[0] != 0 ) {
        glyph_dsc_t * gdsc = (glyph_dsc_t*)__user_font_getdata(p_pos[0], sizeof(glyph_dsc_t));
        dsc_out->adv_w = gdsc->adv_w;
        dsc_out->box_h = gdsc->box_h;
        dsc_out->box_w = gdsc->box_w;
        dsc_out->ofs_x = gdsc->ofs_x;
        dsc_out->ofs_y = gdsc->ofs_y;
        dsc_out->bpp   = __g_xbf_hd.bpp;
        return true;
    }
    return false;
}

//FangSong_GB2312,,-1
//字模高度：18
//XBF字体,外部bin文件
lv_font_t myFont = {
    .get_glyph_bitmap = __user_font_get_bitmap,
    .get_glyph_dsc = __user_font_get_glyph_dsc,
    .line_height = 18,
    .base_line = 0,
};

/**
 * Check if font partition is available
 * @return true if font partition is found, false otherwise
 */
bool myFont_is_available(void) {
    if (partition_res == NULL) {
        // Try to find partition
        partition_res = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, NULL);
    }
    return (partition_res != NULL);
}

