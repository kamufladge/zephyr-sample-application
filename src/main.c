/* main.c - Synchronization demo */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "zephyr/drivers/gpio.h"
#include "zephyr/net/net_event.h"
#include "zephyr/net/net_ip.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample_application, LOG_LEVEL_DBG);

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/sys/printk.h>

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

/* delay between greetings (in ms) */
#define SLEEPTIME 5000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(kamu_led)

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void hello_loop() {
  const char *tname;
  struct k_thread *current_thread;
  current_thread = k_current_get();
  tname = k_thread_name_get(current_thread);

  if (!gpio_is_ready_dt(&led)) {
    return;
  }

  int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    return;
  }

  while (1) {
    gpio_pin_toggle_dt(&led);
    printk("%s: Hello World from %s!\n", tname, CONFIG_BOARD);
    k_msleep(SLEEPTIME);
  }
}

/* thread_b is a static thread spawned immediately */
void thread_b_entry_point(void *dummy1, void *dummy2, void *dummy3) {
  ARG_UNUSED(dummy1);
  ARG_UNUSED(dummy2);
  ARG_UNUSED(dummy3);

  hello_loop();
}
K_THREAD_DEFINE(thread_b, STACKSIZE, thread_b_entry_point, NULL, NULL, NULL,
                PRIORITY, 0, 0);
extern const k_tid_t thread_b;

static void dns_result_cb(enum dns_resolve_status status,
                          struct dns_addrinfo *info, void *user_data) {

  char str_addr[NET_IPV6_ADDR_LEN];
  char *addr_family = NULL;
  void *addr;

  switch (status) {
  case DNS_EAI_CANCELED:
    LOG_INF("DNS query was canceled");
    return;
  case DNS_EAI_FAIL:
    LOG_INF("DNS resolve failed");
    return;
  case DNS_EAI_NODATA:
    LOG_INF("Cannot resolve address");
    return;
  case DNS_EAI_ALLDONE:
    LOG_INF("DNS resolving finished");
    return;
  case DNS_EAI_INPROGRESS:
    break;
  default:
    LOG_INF("DNS resolving error (%d)", status);
    return;
  }

  if (!info) {
    return;
  }

  if (info->ai_family == AF_INET) {
    addr_family = "IPv4";
    addr = &net_sin(&info->ai_addr)->sin_addr;
  } else if (info->ai_family == AF_INET6) {
    addr_family = "IPv6";
    addr = &net_sin6(&info->ai_addr)->sin6_addr;
  } else {
    LOG_ERR("Invalid IP address family %d", info->ai_family);
    return;
  }

  net_addr_ntop(info->ai_family, addr, str_addr, sizeof(str_addr));

  LOG_INF("%s address: %s", addr_family, str_addr);
}

static uint16_t dns_id;

int main(void) {
  int ret;

  struct net_if *iface = net_if_get_default();
  LOG_INF("interface found %s", iface->config.name);

#ifdef CONFIG_NET_DHCPV4
  net_dhcpv4_start(iface);
#endif

  uint32_t raised_events;
  void const *info;
  size_t info_size;
  struct net_if *ifaaaa;
  ret = net_mgmt_event_wait(NET_EVENT_IPV4_ACD_SUCCEED, &raised_events, &ifaaaa,
                            &info, &info_size, K_SECONDS(30));
  if (ret < 0) {
    LOG_ERR("did not connect in time (%d)", ret);
    return -1;
  }

  ret = dns_get_addr_info("google.com", DNS_QUERY_TYPE_A, &dns_id,
                          dns_result_cb, NULL, 2000);

  if (ret < 0) {
    LOG_ERR("Cannot resolve (%d)", ret);
    return -1;
  }

  return 0;
}
