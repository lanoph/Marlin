/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2020 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "../../../../inc/MarlinConfigPre.h"

#if ENABLED(TFT_LITTLE_VGL_UI)

#include "../../../../MarlinCore.h"

#include "string.h"

#include "pic_manager.h"
#include "W25Qxx.h"
#include "../../../../sd/cardreader.h"
#include "draw_ready_print.h"
#include "mks_hardware_test.h"

extern uint16_t DeviceCode;
extern unsigned char bmp_public_buf[17 * 1024];

#if ENABLED(SDSUPPORT)
  extern char *createFilename(char * const buffer, const dir_t &p);
#endif

uint32_t lv_get_pic_addr(uint8_t *Pname) {
  uint8_t Pic_cnt;
  uint8_t i, j;
  PIC_MSG PIC;
  uint32_t tmp_cnt = 0;
  uint32_t addr = 0;

  #if ENABLED(MARLIN_DEV_MODE)
    SERIAL_ECHOLNPAIR("Getting picture SPI Flash Address: ", (const char*)Pname);
  #endif

  W25QXX.init(SPI_QUARTER_SPEED);

  W25QXX.SPI_FLASH_BufferRead(&Pic_cnt, PIC_COUNTER_ADDR, 1);
  if (Pic_cnt == 0xFF) Pic_cnt = 0;
  for (i = 0; i < Pic_cnt; i++) {
    j = 0;
    do {
      W25QXX.SPI_FLASH_BufferRead(&PIC.name[j], PIC_NAME_ADDR + tmp_cnt, 1);
      tmp_cnt++;
    } while (PIC.name[j++] != '\0');

    if ((strcmp((char*)Pname, (char*)PIC.name)) == 0) {
      if ((DeviceCode == 0x9488) || (DeviceCode == 0x5761))
        addr = PIC_DATA_ADDR_TFT35 + i * PER_PIC_MAX_SPACE_TFT35;
      else
        addr = PIC_DATA_ADDR_TFT32 + i * PER_PIC_MAX_SPACE_TFT32;
      return (addr + 4);//The purpose of adding 4 is to remove 4-byte picture header information.
    }
  }

  return addr;
}

const char *picPath = "mks_pic";
const char *bakPath = "bak_pic";

const char *fontPath = "mks_font";
const char *bakFont = "bak_font";

void spiFlashErase_PIC() {
  volatile uint32_t pic_sectorcnt = 0;
  for (pic_sectorcnt = 0; pic_sectorcnt < TERN(MKS_TEST, 2, PIC_SIZE_xM * 1024 / 64); pic_sectorcnt++)
    W25QXX.SPI_FLASH_BlockErase(PICINFOADDR + pic_sectorcnt * 64 * 1024);
}

#if HAS_SPI_FLASH_FONT
  void spiFlashErase_FONT() {
    volatile uint32_t Font_sectorcnt = 0;
    for (Font_sectorcnt = 0; Font_sectorcnt < FONT_SIZE_xM * 1024 / 64; Font_sectorcnt++)
      W25QXX.SPI_FLASH_BlockErase(FONTINFOADDR + Font_sectorcnt * 64 * 1024);
  }
#endif

uint32_t LogoWrite_Addroffset = 0;

uint8_t Pic_Logo_Write(uint8_t *LogoName, uint8_t *Logo_Wbuff, uint32_t LogoWriteSize) {
  if (LogoWriteSize <= 0) return 0;

  W25QXX.SPI_FLASH_BufferWrite(Logo_Wbuff, PIC_LOGO_ADDR + LogoWrite_Addroffset, LogoWriteSize);

  for (uint32_t i = 0; i < LogoWriteSize; i++) {
    uint8_t temp1;
    W25QXX.SPI_FLASH_BufferRead(&temp1, PIC_LOGO_ADDR + LogoWrite_Addroffset + i, 1);
    if (*(Logo_Wbuff + i) != temp1) return 0;
  }
  LogoWrite_Addroffset += LogoWriteSize;
  const uint32_t logo_maxsize = DeviceCode == 0x9488 || DeviceCode == 0x5761 ? LOGO_MAX_SIZE_TFT35 : LOGO_MAX_SIZE_TFT32;
  if (LogoWrite_Addroffset >= logo_maxsize) LogoWrite_Addroffset = 0;
  return 1;
}

uint32_t TitleLogoWrite_Addroffset = 0;
uint8_t Pic_TitleLogo_Write(uint8_t *TitleLogoName, uint8_t *TitleLogo_Wbuff, uint32_t TitleLogoWriteSize) {
  if (TitleLogoWriteSize <= 0)
    return 0;
  if ((DeviceCode == 0x9488) || (DeviceCode == 0x5761))
    W25QXX.SPI_FLASH_BufferWrite(TitleLogo_Wbuff, PIC_ICON_LOGO_ADDR_TFT35 + TitleLogoWrite_Addroffset, TitleLogoWriteSize);
  else
    W25QXX.SPI_FLASH_BufferWrite(TitleLogo_Wbuff, PIC_ICON_LOGO_ADDR_TFT32 + TitleLogoWrite_Addroffset, TitleLogoWriteSize);
  TitleLogoWrite_Addroffset += TitleLogoWriteSize;
  if (TitleLogoWrite_Addroffset >= TITLELOGO_MAX_SIZE)
    TitleLogoWrite_Addroffset = 0;
  return 1;
}

uint32_t default_view_addroffset_r = 0;
void default_view_Write(uint8_t *default_view__Rbuff, uint32_t default_view_Writesize) {
  W25QXX.SPI_FLASH_BufferWrite(default_view__Rbuff, DEFAULT_VIEW_ADDR_TFT35 + default_view_addroffset_r, default_view_Writesize);
  default_view_addroffset_r += default_view_Writesize;
  if (default_view_addroffset_r >= DEFAULT_VIEW_MAX_SIZE)
    default_view_addroffset_r = 0;
}

uint32_t Pic_Info_Write(uint8_t *P_name, uint32_t P_size) {
  uint8_t pic_counter = 0;
  uint32_t Pic_SaveAddr;
  uint32_t Pic_SizeSaveAddr;
  uint32_t Pic_NameSaveAddr;
  uint8_t Pname_temp;
  uint32_t i, j;
  uint32_t name_len = 0;
  uint32_t SaveName_len = 0;
  union union32 size_tmp;

  W25QXX.SPI_FLASH_BufferRead(&pic_counter, PIC_COUNTER_ADDR, 1);

  if (pic_counter == 0xFF)
    pic_counter = 0;

  if ((DeviceCode == 0x9488) || (DeviceCode == 0x5761))
    Pic_SaveAddr = PIC_DATA_ADDR_TFT35 + pic_counter * PER_PIC_MAX_SPACE_TFT35;
  else
    Pic_SaveAddr = PIC_DATA_ADDR_TFT32 + pic_counter * PER_PIC_MAX_SPACE_TFT32;

  for (j = 0; j < pic_counter; j++) {
    do {
      W25QXX.SPI_FLASH_BufferRead(&Pname_temp, PIC_NAME_ADDR + SaveName_len, 1);
      SaveName_len++;
    } while (Pname_temp != '\0');
  }
  i = 0;
  while ((*(P_name + i) != '\0')) {
    i++;
    name_len++;
  }

  Pic_NameSaveAddr = PIC_NAME_ADDR + SaveName_len;
  W25QXX.SPI_FLASH_BufferWrite(P_name, Pic_NameSaveAddr, name_len + 1);
  Pic_SizeSaveAddr = PIC_SIZE_ADDR + 4 * pic_counter;
  size_tmp.dwords = P_size;
  W25QXX.SPI_FLASH_BufferWrite(size_tmp.bytes, Pic_SizeSaveAddr, 4);

  pic_counter++;
  W25QXX.SPI_FLASH_SectorErase(PIC_COUNTER_ADDR);
  W25QXX.SPI_FLASH_BufferWrite(&pic_counter, PIC_COUNTER_ADDR, 1);

  return Pic_SaveAddr;
}

uint8_t public_buf[512];

#if ENABLED(SDSUPPORT)

  void UpdatePic() {
    char *fn;
    unsigned char logoFlag;
    uint16_t pbr;
    uint32_t pfileSize;
    uint32_t Pic_Write_Addr;

    SdFile dir, root = card.getroot();
    if (dir.open(&root, picPath, O_RDONLY)) {

      disp_pic_update();
      spiFlashErase_PIC();

      dir_t d;
      while (dir.readDir(&d, card.longFilename) > 0) {
        if (card.longFilename[0] == 0)
          continue;
        if (card.longFilename[0] == '.')
          continue;

        fn = card.longFilename;

        if (strstr(fn, ".bin")) {
          if (strstr(fn, "_logo"))
            logoFlag = 1;
          else if (strstr(fn, "_titlelogo"))
            logoFlag = 2;
          else if (strstr(fn, "_preview"))
            logoFlag = 3;
          else
            logoFlag = 0;

          char dosFilename[FILENAME_LENGTH];
          createFilename(dosFilename, d);

          SdFile file;
          if (file.open(&dir, dosFilename, O_READ)) {
            if (logoFlag == 1) {
              while (1) {
                pbr = file.read(public_buf, BMP_WRITE_BUF_LEN);
                Pic_Logo_Write((uint8_t *)fn, public_buf, pbr); //
                if (pbr < BMP_WRITE_BUF_LEN) break;
              }
            }
            else if (logoFlag == 2) {
              while (1) {
                pbr = file.read(public_buf, BMP_WRITE_BUF_LEN);
                Pic_TitleLogo_Write((uint8_t *)fn, public_buf, pbr); //
                if (pbr < BMP_WRITE_BUF_LEN) break;
              }
            }
            else if (logoFlag == 3) {
              while (1) {
                pbr = file.read(public_buf, BMP_WRITE_BUF_LEN);
                default_view_Write(public_buf, pbr); //
                if (pbr < BMP_WRITE_BUF_LEN) break;
              }
            }
            else {
              pfileSize = file.fileSize();
              Pic_Write_Addr = Pic_Info_Write((uint8_t *)fn, pfileSize);
              while (1) {
                pbr = file.read(public_buf, BMP_WRITE_BUF_LEN);
                W25QXX.SPI_FLASH_BufferWrite(public_buf, Pic_Write_Addr, pbr);
                Pic_Write_Addr += pbr;
                if (pbr < BMP_WRITE_BUF_LEN) break;
              }
            }
            file.close();
          }

        }

      }
      dir.rename(&root, bakPath);
    }
    dir.close();
  }

  #if HAS_SPI_FLASH_FONT

    void spi_flash_read_test() { W25QXX.SPI_FLASH_BufferRead(public_buf, UNIGBK_FLASH_ADDR, BMP_WRITE_BUF_LEN); }

    void UpdateFont() {
      char *fn;
      uint16_t pbr;
      uint32_t flashaddr = 0;

      SdFile dir, root = card.getroot();
      if (dir.open(&root, fontPath, O_RDONLY)) {

        disp_font_update();
        spiFlashErase_FONT();

        dir_t d;
        while (dir.readDir(&d, card.longFilename) > 0) {
          if (card.longFilename[0] == 0)
            break;

          if (card.longFilename[0] == '.')
            continue;

          fn = card.longFilename;

          if (strstr(fn, ".bin")) {
            char dosFilename[FILENAME_LENGTH];
            createFilename(dosFilename, d);
            //strcat(public_buf, dosFilename);

            SdFile file;
            if (file.open(&dir, dosFilename, O_READ)) {

              flashaddr = UNIGBK_FLASH_ADDR;
              pbr = 0;
              while (1) {
                flashaddr += pbr;
                pbr = file.read(public_buf, BMP_WRITE_BUF_LEN);
                W25QXX.SPI_FLASH_BufferWrite(public_buf, flashaddr, pbr);
                if (pbr < BMP_WRITE_BUF_LEN) break;
              }
              file.close();
            }

          }
        }

        dir.rename(&root, bakFont);
        dir.close();
      }
    }

  #endif // HAS_SPI_FLASH_FONT

#endif // SDSUPPORT

void Pic_Read(uint8_t *Pname, uint8_t *P_Rbuff) {
  uint8_t i, j;
  uint8_t Pic_cnt;
  uint32_t tmp_cnt = 0;
  PIC_MSG PIC;

  W25QXX.SPI_FLASH_BufferRead(&Pic_cnt, PIC_COUNTER_ADDR, 1);
  if (Pic_cnt == 0xff)
    Pic_cnt = 0;

  for (i = 0; i < Pic_cnt; i++) {
    j = 0;
    do {
      W25QXX.SPI_FLASH_BufferRead(&PIC.name[j], PIC_NAME_ADDR + tmp_cnt, 1);
      tmp_cnt++;
    } while (PIC.name[j++] != '\0');
    //pic size
    W25QXX.SPI_FLASH_BufferRead(PIC.size.bytes, PIC_SIZE_ADDR + i * 4, 4);

    if ((strcmp((char*)Pname, (char*)PIC.name)) == 0) {
      W25QXX.SPI_FLASH_BufferRead((uint8_t *)P_Rbuff, PIC_DATA_ADDR_TFT35 + i * PER_PIC_MAX_SPACE_TFT35, PIC.size.dwords);
      break;
    }
  }
}

void lv_pic_test(uint8_t *P_Rbuff, uint32_t addr, uint32_t size) {
  W25QXX.init(SPI_QUARTER_SPEED);
  W25QXX.SPI_FLASH_BufferRead((uint8_t *)P_Rbuff, addr, size);
}

#if HAS_SPI_FLASH_FONT
  void get_spi_flash_data(const char *rec_buf, int addr, int size) {
    W25QXX.init(SPI_QUARTER_SPEED);
    W25QXX.SPI_FLASH_BufferRead((uint8_t *)rec_buf, UNIGBK_FLASH_ADDR + addr, size);
  }
#endif

uint32_t logo_addroffset = 0;
void Pic_Logo_Read(uint8_t *LogoName, uint8_t *Logo_Rbuff, uint32_t LogoReadsize) {
  W25QXX.SPI_FLASH_BufferRead(Logo_Rbuff, PIC_LOGO_ADDR + logo_addroffset, LogoReadsize);
  logo_addroffset += LogoReadsize;
  if (logo_addroffset >= LOGO_MAX_SIZE_TFT35)
    logo_addroffset = 0;
}

uint32_t default_view_addroffset = 0;
void default_view_Read(uint8_t *default_view_Rbuff, uint32_t default_view_Readsize) {
  W25QXX.init(SPI_QUARTER_SPEED);

  W25QXX.SPI_FLASH_BufferRead(default_view_Rbuff, DEFAULT_VIEW_ADDR_TFT35 + default_view_addroffset + 4, default_view_Readsize);
  default_view_addroffset += default_view_Readsize;
  if (default_view_addroffset >= DEFAULT_VIEW_MAX_SIZE)
    default_view_addroffset = 0;
}

uint32_t flash_view_addroffset = 0;
void flash_view_Read(uint8_t *flash_view_Rbuff, uint32_t flash_view_Readsize) {
  W25QXX.init(SPI_QUARTER_SPEED);

  W25QXX.SPI_FLASH_BufferRead(flash_view_Rbuff, BAK_VIEW_ADDR_TFT35 + flash_view_addroffset, flash_view_Readsize);
  flash_view_addroffset += flash_view_Readsize;
  if (flash_view_addroffset >= FLASH_VIEW_MAX_SIZE)
    flash_view_addroffset = 0;
}

#endif // TFT_LITTLE_VGL_UI