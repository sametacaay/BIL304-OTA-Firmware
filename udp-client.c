#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/node-id.h"
#include "sys/log.h"
#include "sys/etimer.h"
#include "firmware_data.h"
#include <stdint.h>
#include <string.h>

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678
#define SEND_INTERVAL   (2 * CLOCK_SECOND)
#define BLOCK_SIZE      64

static struct simple_udp_connection udp_conn;


typedef struct {
  uint16_t block_no;
  uint16_t total_blocks;
  uint8_t  checksum;
  uint8_t  data[BLOCK_SIZE];
} ota_packet_t;

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

  LOG_INFO("ACK alindi, %d byte\n", datalen);
}

PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);

PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static uint16_t current_block = 0;
  static uint16_t total_blocks;
  uip_ipaddr_t dest_ipaddr;
  ota_packet_t pkt;

  PROCESS_BEGIN();

  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  total_blocks = (new_firmware_z1_len + BLOCK_SIZE - 1) / BLOCK_SIZE;

  LOG_INFO("Firmware boyutu: %u byte, toplam blok: %u\n",
           (unsigned)new_firmware_z1_len, total_blocks);

  etimer_set(&periodic_timer, 5 * CLOCK_SECOND);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if(node_id != 2) {
      etimer_set(&periodic_timer, SEND_INTERVAL);
      continue;
    }

    if(current_block >= total_blocks) {
      LOG_INFO("Tum bloklar gonderildi! (%u blok)\n", total_blocks);
      etimer_set(&periodic_timer, 60 * CLOCK_SECOND);
      continue;
    }

    if(NETSTACK_ROUTING.node_is_reachable() &&
       NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {

      uint32_t offset = (uint32_t)current_block * BLOCK_SIZE;
      uint16_t len = BLOCK_SIZE;
      if(offset + len > new_firmware_z1_len) {
        len = new_firmware_z1_len - offset;
      }

      pkt.block_no     = current_block;
      pkt.total_blocks = total_blocks;
      memcpy(pkt.data, new_firmware_z1 + offset, len);

      if(len < BLOCK_SIZE) {
        memset(pkt.data + len, 0, BLOCK_SIZE - len);
      }
      pkt.checksum = calc_checksum(pkt.data, BLOCK_SIZE);

      simple_udp_sendto(&udp_conn, &pkt, sizeof(ota_packet_t), &dest_ipaddr);

      LOG_INFO("Blok %u/%u gonderildi (offset=%lu, len=%u, cs=0x%02x)\n",
               current_block, total_blocks - 1,
               (unsigned long)offset, len, pkt.checksum);

      current_block++;
    } else {
      LOG_INFO("Henuz ulasılamıyor...\n");
    }

    etimer_set(&periodic_timer, SEND_INTERVAL);
  }

  PROCESS_END();
}
