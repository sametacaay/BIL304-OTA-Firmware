#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "cfs/cfs.h"
#include "sys/log.h"
#include <stdint.h>
#include <string.h>

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678
#define BLOCK_SIZE      64

static struct simple_udp_connection udp_conn;

typedef struct {
  uint16_t block_no;
  uint16_t total_blocks;
  uint8_t  checksum;
  uint8_t  data[BLOCK_SIZE];
} ota_packet_t;

static uint16_t received_blocks = 0;
static uint16_t total_blocks    = 0;
static uint8_t  transfer_done   = 0;

static uint8_t calc_checksum(const uint8_t *data, uint16_t len) {
  uint8_t sum = 0;
  uint16_t i;
  for(i = 0; i < len; i++) sum ^= data[i];
  return sum;
}

static void udp_rx_callback(struct simple_udp_connection *c,
  const uip_ipaddr_t *sender_addr, uint16_t sender_port,
  const uip_ipaddr_t *receiver_addr, uint16_t receiver_port,
  const uint8_t *data, uint16_t datalen)
{
  ota_packet_t pkt;
  uint8_t expected_cs;
  int fd;

  if(transfer_done) return;
  if(datalen != sizeof(ota_packet_t)) {
    LOG_WARN("Yanlis paket boyutu: %u\n", datalen);
    return;
  }

  memcpy(&pkt, data, sizeof(ota_packet_t));


  expected_cs = calc_checksum(pkt.data, BLOCK_SIZE);
  if(expected_cs != pkt.checksum) {
    LOG_WARN("Blok %u checksum hatasi! beklenen=0x%02x gelen=0x%02x\n",
             pkt.block_no, expected_cs, pkt.checksum);

    return;
  }

  if(total_blocks == 0) {
    total_blocks = pkt.total_blocks;
    LOG_INFO("Transfer basladi, toplam blok: %u\n", total_blocks);
  }


  fd = cfs_open("firmware.bin", CFS_WRITE | CFS_APPEND);
  if(fd < 0) {
    LOG_ERR("CFS acilamadi!\n");
    return;
  }
  cfs_write(fd, pkt.data, BLOCK_SIZE);
  cfs_close(fd);

  received_blocks++;

  LOG_INFO("Blok %u/%u alindi, cs=0x%02x\n",
           pkt.block_no, total_blocks - 1, pkt.checksum);


  simple_udp_sendto(&udp_conn, "ACK", 3, sender_addr);


  if(received_blocks >= total_blocks) {
    transfer_done = 1;
    LOG_INFO("========================================\n");
    LOG_INFO("Yuklenmeye hazir yeni firmware alimi tamamlandi.\n");
    LOG_INFO("Toplam %u blok, %u byte CFS'e kaydedildi.\n",
             received_blocks, (unsigned)(received_blocks * BLOCK_SIZE));
    LOG_INFO("========================================\n");
  }
}

PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);

PROCESS_THREAD(udp_server_process, ev, data)
{
  PROCESS_BEGIN();

  NETSTACK_ROUTING.root_start();

  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);

  LOG_INFO("OTA alici hazir, firmware bekleniyor...\n");

  PROCESS_END();
}
