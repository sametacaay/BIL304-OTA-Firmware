# BIL304 - OTA Firmware Transfer Sistemi

## Proje Hakkında
Bu proje, Contiki-NG işletim sistemi üzerinde Cooja simülatörü kullanılarak
geliştirilmiş bir OTA (Over-The-Air) firmware transfer sistemidir.

## Sistem Mimarisi
- **Düğüm 2 (Gönderici):** new-firmware.z1 dosyasını bloklara bölerek gönderir
- **Düğüm 3 (Aracı):** Paketleri iletir
- **Düğüm 1 (Alıcı):** Blokları alır, doğrular ve CFS'e kaydeder

## Gerçeklenen Yöntemler

### Paket Yapısı
Her paket şu alanları içerir:
- `block_no` (2 byte): Blok numarası
- `total_blocks` (2 byte): Toplam blok sayısı
- `checksum` (1 byte): XOR tabanlı hata kontrolü
- `data` (64 byte): Firmware verisi

### Paket Uzunluğu
- Blok boyutu: 64 byte
- Toplam paket boyutu: 69 byte

### Güvenilir Transfer Stratejisi
- **Stop-and-Wait:** Gönderici her blok için ACK bekler
- **Checksum:** XOR tabanlı blok bütünlük kontrolü
- **Sıralı gönderim:** Her blok numarasıyla birlikte gönderilir

### Alınan Önlemler
- Bozuk bloklar checksum ile tespit edilir, ACK gönderilmez
- Alıcı eksik blokları takip eder
- Transfer tamamlanınca CFS dosya sistemine kaydedilir

## Çalışma Videosu
https://www.youtube.com/watch?v=NxrdeFBZfXE

## Sonuç
Transfer tamamlandığında alıcı düğüm şu mesajı yazdırır:
```
Yuklenmeye hazir yeni firmware alimi tamamlandi.
```
