/* main.c - Synchronization demo */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "zephyr/net/net_ip.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample_application, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/net_if.h>
#include <zephyr/sys/printk.h>

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

/* delay between greetings (in ms) */
#define SLEEPTIME 500

/*
 * @param my_name      thread identification string
 * @param my_sem       thread's own semaphore
 * @param other_sem    other thread's semaphore
 */
void hello_loop() {
  const char *tname;
  struct k_thread *current_thread;
  current_thread = k_current_get();
  tname = k_thread_name_get(current_thread);

  while (1) {
    printk("%s: Hello World from %s!\n", tname, CONFIG_BOARD);
    k_busy_wait(100000);
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
  struct net_if *iface = net_if_get_default();
  LOG_INF("interface found %s", iface->config.name);

  int ret = dns_get_addr_info("www.zephyrproject.org", DNS_QUERY_TYPE_A,
                              &dns_id, dns_result_cb, NULL, 2000);

  if (ret < 0) {
    LOG_ERR("Cannot resolve (%d)", ret);
    return -1;
  }

  return 0;
}
