// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#include "../touchpanel_common.h"
#include "synaptics_common.h"
#include <linux/crc32.h>
#include <linux/syscalls.h>

/*******Part0:LOG TAG Declear********************/

#define TPD_DEVICE "synaptics_common"
#define TPD_INFO(a, arg...)  pr_err("[TP]"TPD_DEVICE ": " a, ##arg)
#define TPD_DEBUG(a, arg...)\
    do{\
        if (LEVEL_DEBUG == tp_debug)\
            pr_err("[TP]"TPD_DEVICE ": " a, ##arg);\
    }while(0)

#define TPD_DETAIL(a, arg...)\
    do{\
        if (LEVEL_BASIC != tp_debug)\
            pr_err("[TP]"TPD_DEVICE ": " a, ##arg);\
    }while(0)


/*******Part1:Call Back Function implement*******/

unsigned int extract_uint_le(const unsigned char *ptr)
{
    return (unsigned int)ptr[0] +
        (unsigned int)ptr[1] * 0x100 +
        (unsigned int)ptr[2] * 0x10000 +
        (unsigned int)ptr[3] * 0x1000000;
}

/*******************Limit File With "limit_block" Format*************************************/
int synaptics_get_limit_data(char *type, const unsigned char *fw_image)
{
    int i = 0;
    unsigned int offset, count;
    struct limit_info *limit_info;
    struct limit_block *limit_block;

    limit_info = (struct limit_info *)fw_image;
    count = limit_info->count;

    offset = sizeof(struct limit_info);
    for (i = 0; i < count; i++) {
        limit_block = (struct limit_block *)(fw_image + offset);
        pr_info("name: %s, size: %d, offset %d\n", limit_block->name, limit_block->size, offset);
        if (strncmp(limit_block->name, type, MAX_LIMIT_NAME_SIZE) == 0) {
            break;
        }

        offset += (sizeof(struct limit_block) - 4 + 2*limit_block->size); /*minus 4, because byte align*/
    }

    if (i == count) {
        return 0;
    }

    return offset;
}

/*************************************TCM Firmware Parse Funtion**************************************/
int synaptics_parse_header_v2(struct image_info *image_info, const unsigned char *fw_image)
{
    struct image_header_v2 *header;
    unsigned int magic_value;
    unsigned int number_of_areas;
    unsigned int i = 0;
    unsigned int addr;
    unsigned int length;
    unsigned int checksum;
    unsigned int flash_addr;
    const unsigned char *content;
    struct area_descriptor *descriptor;
    int offset = sizeof(struct image_header_v2);

    header = (struct image_header_v2 *)fw_image;
    magic_value = le4_to_uint(header->magic_value);

    if (magic_value != IMAGE_FILE_MAGIC_VALUE) {
        pr_err("invalid magic number %d\n", magic_value);
        return -EINVAL;
    }

    number_of_areas = le4_to_uint(header->num_of_areas);

    for (i = 0; i < number_of_areas; i++) {
        addr = le4_to_uint(fw_image + offset);
        descriptor = (struct area_descriptor *)(fw_image + addr);
        offset += 4;

        magic_value =  le4_to_uint(descriptor->magic_value);
        if (magic_value != FLASH_AREA_MAGIC_VALUE)
            continue;

        length = le4_to_uint(descriptor->length);
        content = (unsigned char *)descriptor + sizeof(*descriptor);
        flash_addr = le4_to_uint(descriptor->flash_addr_words) * 2;
        checksum = le4_to_uint(descriptor->checksum);

        if (0 == strncmp((char *)descriptor->id_string,
                BOOT_CONFIG_ID,
                strlen(BOOT_CONFIG_ID))) {
            if (checksum != (crc32(~0, content, length) ^ ~0)) {
                pr_err("Boot config checksum error\n");
                return -EINVAL;
            }
            image_info->boot_config.size = length;
            image_info->boot_config.data = content;
            image_info->boot_config.flash_addr = flash_addr;
            pr_info("Boot config size = %d, address = 0x%08x\n", length, flash_addr);
        } else if (0 == strncmp((char *)descriptor->id_string,
                APP_CODE_ID,
                strlen(APP_CODE_ID))) {
            if (checksum != (crc32(~0, content, length) ^ ~0)) {
                pr_err("Application firmware checksum error\n");
                return -EINVAL;
            }
            image_info->app_firmware.size = length;
            image_info->app_firmware.data = content;
            image_info->app_firmware.flash_addr = flash_addr;
            pr_info("Application firmware size = %d address = 0x%08x\n", length, flash_addr);
        } else if (0 == strncmp((char *)descriptor->id_string,
                APP_CONFIG_ID,
                strlen(APP_CONFIG_ID))) {
            if (checksum != (crc32(~0, content, length) ^ ~0)) {
                pr_err("Application config checksum error\n");
                return -EINVAL;
            }
            image_info->app_config.size = length;
            image_info->app_config.data = content;
            image_info->app_config.flash_addr = flash_addr;
            pr_info("Application config size = %d address = 0x%08x\n",length, flash_addr);
        } else if (0 == strncmp((char *)descriptor->id_string,
                DISP_CONFIG_ID,
                strlen(DISP_CONFIG_ID))) {
            if (checksum != (crc32(~0, content, length) ^ ~0)) {
                pr_err("Display config checksum error\n");
                return -EINVAL;
            }
            image_info->disp_config.size = length;
            image_info->disp_config.data = content;
            image_info->disp_config.flash_addr = flash_addr;
            pr_info("Display config size = %d address = 0x%08x\n", length, flash_addr);
        }
    }
    return 0;
}

/**********************************RMI Firmware Parse Funtion*****************************************/
void synaptics_parse_header(struct image_header_data *header, const unsigned char *fw_image)
{
    struct image_header *data = (struct image_header *)fw_image;

    header->checksum = extract_uint_le(data->checksum);
    TPD_DEBUG(" checksume is %x", header->checksum);

    header->bootloader_version = data->bootloader_version;
    TPD_DEBUG(" bootloader_version is %d\n", header->bootloader_version);

    header->firmware_size = extract_uint_le(data->firmware_size);
    TPD_DEBUG(" firmware_size is %x\n", header->firmware_size);

    header->config_size = extract_uint_le(data->config_size);
    TPD_DEBUG(" header->config_size is %x\n", header->config_size);

    /* only available in s4322 , reserved in other, begin*/
    header->bootloader_offset = extract_uint_le(data->bootloader_addr );
    header->bootloader_size = extract_uint_le(data->bootloader_size);
    TPD_DEBUG(" header->bootloader_offset is %x\n", header->bootloader_offset);
    TPD_DEBUG(" header->bootloader_size is %x\n", header->bootloader_size);

    header->disp_config_offset = extract_uint_le(data->dsp_cfg_addr);
    header->disp_config_size = extract_uint_le(data->dsp_cfg_size);
    TPD_DEBUG(" header->disp_config_offset is %x\n", header->disp_config_offset);
    TPD_DEBUG(" header->disp_config_size is %x\n", header->disp_config_size);
    /* only available in s4322 , reserved in other ,  end*/

    memcpy(header->product_id, data->product_id, sizeof(data->product_id));
    header->product_id[sizeof(data->product_id)] = 0;

    memcpy(header->product_info, data->product_info, sizeof(data->product_info));

    header->contains_firmware_id = data->options_firmware_id;
    TPD_DEBUG(" header->contains_firmware_id is %x\n", header->contains_firmware_id);
    if (header->contains_firmware_id)
        header->firmware_id = extract_uint_le(data->firmware_id);

    return;
}

void synaptics_print_limit_v2(struct seq_file *s, struct touchpanel_data *ts, const struct firmware *fw)
{
    int i = 0, index = 0;
    int row, col;
    int rows, cols;
    unsigned int offset, count;
    int16_t *data_16;
    struct limit_info *limit_info;
    struct limit_block *limit_block;
    const char *data = fw->data;

    limit_info = (struct limit_info*)data;
    count = limit_info->count;

    offset = sizeof(struct limit_info);
    for (i = 0; i < count; i++) {
        limit_block = (struct limit_block *)(data + offset);
        pr_info("name: %s, size: %d, offset %d\n", limit_block->name, limit_block->size, offset);

        seq_printf(s, "%s\n", limit_block->name);

        data_16 = &limit_block->data;
        offset += (sizeof(struct limit_block) - 4 + 2*limit_block->size); /*minus 4, because byte align*/

        if ((ts->hw_res.TX_NUM*ts->hw_res.RX_NUM) != limit_block->size/2) {
            continue;
        }

        cols = ts->hw_res.TX_NUM;
        rows = ts->hw_res.RX_NUM;

        index = 0;
        for (row = 0; row < rows; row++){
            seq_printf(s, "[%02d]:", row);
            for(col = 0; col < cols; col++) {
                seq_printf(s, "%4d %4d,", *(data_16 + 2*index), *(data_16 + 2*index + 1));
                index++;
            }
            seq_printf(s, "\n");
        }

    }
    return;
}

void synaptics_print_limit_v1(struct seq_file *s, struct touchpanel_data *ts, const struct firmware *fw)
{
    uint16_t *prow = NULL;
    uint16_t *prowcbc = NULL;
    int i = 0;
    struct test_header *ph = NULL;

    ph = (struct test_header *)(fw->data);
    prow = (uint16_t *)(fw->data + ph->array_limit_offset);
    prowcbc = (uint16_t *)(fw->data + ph->array_limitcbc_offset);
    TPD_INFO("synaptics_test_limit_show:array_limit_offset = %x array_limitcbc_offset = %x \n",
            ph->array_limit_offset, ph->array_limitcbc_offset);
    TPD_DEBUG("test begin:\n");
    seq_printf(s, "Without cbc:");

    for (i = 0 ; i < (ph->array_limit_size / 2); i++) {
        if (i % (2 * ts->hw_res.RX_NUM) == 0)
            seq_printf(s, "\n[%2d] ", (i / ts->hw_res.RX_NUM) / 2);
        seq_printf(s, "%4d, ", prow[i]);
        TPD_DEBUG("%d, ", prow[i]);
    }
    if (ph->withCBC == 1) {
        seq_printf(s, "\nWith cbc:");
        for (i = 0 ; i < (ph->array_limitcbc_size / 2); i++) {
            if (i % (2 * ts->hw_res.RX_NUM) == 0)
                seq_printf(s, "\n[%2d] ", (i / ts->hw_res.RX_NUM) / 2);
            seq_printf(s, "%4d, ", prowcbc[i]);
            TPD_DEBUG("%d, ", prowcbc[i]);
        }
    }

    seq_printf(s, "\n");
    return;
}

void synaptics_print_limit_v3(struct seq_file *s, struct touchpanel_data *ts, const struct firmware *fw)
{
    int32_t *p_data32 = NULL;
    uint32_t i = 0, j = 0, m = 0, item_cnt = 0, array_index = 0;
    uint32_t *p_item_offset = NULL;
    struct test_header_new *ph = NULL;
    struct syna_test_item_header *item_head = NULL;

    ph = (struct test_header_new *)(fw->data);
    p_item_offset = (uint32_t *)(fw->data + sizeof(struct test_header_new));
    for (i = 0; i < 8*sizeof(ph->test_item); i++) {
        if ((ph->test_item >> i) & 0x01) {
            item_cnt++;
        }
    }

    for (m = 0; m < item_cnt; m++) {
        item_head = (struct syna_test_item_header *)(fw->data + p_item_offset[m]);
        if (item_head->item_magic != Limit_ItemMagic) {
            seq_printf(s, "item: %d limit data has some problem\n", item_head->item_bit);
            continue;
        }

        seq_printf(s, "item %d[size %d, limit type %d, para num %d] :\n", item_head->item_bit, item_head->item_size, item_head->item_limit_type, item_head->para_num);
        if (item_head->item_limit_type == LIMIT_TYPE_NO_DATA) {
            seq_printf(s, "no limit data\n");
        } else if (item_head->item_limit_type == LIMIT_TYPE_CERTAIN_DATA) {
            p_data32 = (int32_t *)(fw->data + item_head->top_limit_offset);
            seq_printf(s, "top limit data: %d\n", *p_data32);
            p_data32 = (int32_t *)(fw->data + item_head->floor_limit_offset);
            seq_printf(s, "floor limit data: %d\n", *p_data32);
        } else if (item_head->item_limit_type == LIMIT_TYPE_EACH_NODE_DATA) {
            if ((item_head->item_bit == TYPE_FULLRAW_CAP) || (item_head->item_bit == TYPE_DELTA_NOISE) || (item_head->item_bit == TYPE_RAW_CAP)) {
                p_data32 = (int32_t *)(fw->data + item_head->top_limit_offset);
                seq_printf(s, "top data: \n");
                for (i = 0; i < ts->hw_res.TX_NUM; i++) {
                    seq_printf(s, "[%02d] ", i);
                    for (j = 0; j < ts->hw_res.RX_NUM; j++) {
                        array_index = i * ts->hw_res.RX_NUM + j;
                        seq_printf(s, "%4d, ", p_data32[array_index]);
                    }
                    seq_printf(s, "\n");
                }

                p_data32 = (int32_t *)(fw->data + item_head->floor_limit_offset);
                seq_printf(s, "floor data: \n");
                for (i = 0; i < ts->hw_res.TX_NUM; i++) {
                    seq_printf(s, "[%02d] ", i);
                    for (j = 0; j < ts->hw_res.RX_NUM; j++) {
                        array_index = i * ts->hw_res.RX_NUM + j;
                        seq_printf(s, "%4d, ", p_data32[array_index]);
                    }
                    seq_printf(s, "\n");
                }
            } else if((item_head->item_bit == TYPE_HYBRIDRAW_CAP) || (item_head->item_bit == TYPE_HYBRIDABS_DIFF_CBC) || (item_head->item_bit == TYPE_HYBRIDABS_NOSIE)){
                p_data32 = (int32_t *)(fw->data + item_head->top_limit_offset);
                seq_printf(s, "top data: \n");
                for (i = 0; i < ts->hw_res.TX_NUM + ts->hw_res.RX_NUM; i++) {
                    seq_printf(s, "%4d, ", p_data32[i]);
                }
                seq_printf(s, "\n");

                p_data32 = (int32_t *)(fw->data + item_head->floor_limit_offset);
                seq_printf(s, "floor data: \n");
                for (i = 0; i < ts->hw_res.TX_NUM + ts->hw_res.RX_NUM; i++) {
                    seq_printf(s, "%4d, ", p_data32[i]);
                }
                seq_printf(s, "\n");
            } else if (item_head->item_bit == TYPE_TREXSHORT_CUSTOM) {
                p_data32 = (int32_t *)(fw->data + item_head->top_limit_offset);
                seq_printf(s, "top data: \n");
                for (i = 0; i < ts->hw_res.RX_NUM; i++) {
                    seq_printf(s, "%4d, ", p_data32[i]);
                }
                seq_printf(s, "\n");

                p_data32 = (int32_t *)(fw->data + item_head->floor_limit_offset);
                seq_printf(s, "floor data: \n");
                for (i = 0; i < ts->hw_res.RX_NUM; i++) {
                    seq_printf(s, "%4d, ", p_data32[i]);
                }
                seq_printf(s, "\n");
            }
        }

        p_data32 = (int32_t *)(fw->data + p_item_offset[m] + sizeof(struct syna_test_item_header));
        if (item_head->para_num) {
            seq_printf(s, "parameter:");
            for (j = 0; j < item_head->para_num; j++) {
                seq_printf(s, "%d, ", p_data32[j]);
            }
            seq_printf(s, "\n");
        }
        seq_printf(s, "\n");
    }
}

void synaptics_limit_read(struct seq_file *s, struct touchpanel_data *ts)
{
    int ret = 0;
    const struct firmware *fw = NULL;
    struct test_header *ph = NULL;
    uint32_t *p_firstitem_offset = NULL, *p_firstitem = NULL;

    ret = request_firmware(&fw, ts->panel_data.test_limit_name, ts->dev);
    if (ret < 0) {
        TPD_INFO("Request firmware failed - %s (%d)\n", ts->panel_data.test_limit_name, ret);
        seq_printf(s, "Request failed, Check the path %s\n",ts->panel_data.test_limit_name);
        return;
    }

    ph = (struct test_header *)(fw->data);
    p_firstitem_offset = (uint32_t *)(fw->data + sizeof(struct test_header_new));
    if (ph->magic1 == Limit_MagicNum1 && ph->magic2 == Limit_MagicNum2 && (fw->size >= *p_firstitem_offset+sizeof(uint32_t))) {
        p_firstitem = (uint32_t *)(fw->data + *p_firstitem_offset);
        if (*p_firstitem == Limit_ItemMagic) {
            synaptics_print_limit_v3(s, ts, fw);
            release_firmware(fw);
            return;
        }
    }
    if (ph->magic1 == Limit_MagicNum1 && ph->magic2 == Limit_MagicNum2_V2) {
        synaptics_print_limit_v2(s, ts, fw);
    } else {
        synaptics_print_limit_v1(s, ts, fw);
    }
    release_firmware(fw);
}
